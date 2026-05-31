"""
serial_reader.py — Threaded Serial Port Reader
================================================
Reads newline-delimited JSON packets from the ESP32 UART2 output.

DESIGN DECISIONS:
- Runs in a daemon thread so it dies when the main app closes.
- Uses a thread-safe queue (collections.deque with maxlen) to buffer data.
- JSON parsing errors are silently skipped — corrupted packets happen
  in real serial communication and must not crash the main UI thread.

Usage:
    reader = SerialReader(port='COM3', baud=115200)
    reader.start()
    while True:
        packet = reader.get_latest()
        if packet:
            process(packet)
"""

import threading
import json
import time
import collections
import serial
import serial.tools.list_ports


class SerialReader:
    """
    Thread-safe serial port reader for ESP32 JSON telemetry.
    """

    def __init__(self, port: str, baud: int = 115200, buffer_size: int = 200):
        """
        :param port:        Serial port name (e.g., 'COM3' or '/dev/ttyUSB0')
        :param baud:        Baud rate — must match firmware config (115200)
        :param buffer_size: Max packets retained in rolling buffer
        """
        self.port        = port
        self.baud        = baud
        self._buffer     = collections.deque(maxlen=buffer_size)
        self._lock       = threading.Lock()
        self._running    = False
        self._thread     = None
        self._ser        = None
        self.error_msg   = None
        self.connected   = False

    def start(self) -> bool:
        """Open serial port and start reader thread. Returns True on success."""
        try:
            self._ser = serial.Serial(
                port=self.port,
                baudrate=self.baud,
                timeout=1.0,        # 1s read timeout — prevents blocking forever
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
            )
            self._running  = True
            self.connected = True
            self.error_msg = None
            self._thread   = threading.Thread(target=self._read_loop,
                                               daemon=True, name="SerialReader")
            self._thread.start()
            return True
        except serial.SerialException as e:
            self.error_msg = str(e)
            self.connected = False
            return False

    def stop(self):
        """Stop reader thread and close serial port gracefully."""
        self._running = False
        if self._thread and self._thread.is_alive():
            self._thread.join(timeout=2.0)
        if self._ser and self._ser.is_open:
            self._ser.close()
        self.connected = False

    def _read_loop(self):
        """Internal thread function — reads lines and parses JSON."""
        while self._running:
            try:
                if self._ser and self._ser.in_waiting > 0:
                    raw_line = self._ser.readline().decode('utf-8', errors='ignore').strip()
                    if raw_line.startswith('{'):
                        packet = json.loads(raw_line)
                        packet['_rx_time'] = time.time()    # Add local receive timestamp
                        with self._lock:
                            self._buffer.append(packet)
            except json.JSONDecodeError:
                pass    # Corrupted packet — skip silently
            except serial.SerialException as e:
                self.error_msg = str(e)
                self.connected = False
                self._running  = False
            except Exception:
                pass

    def get_latest(self) -> dict | None:
        """Return the most recent packet without removing it."""
        with self._lock:
            return dict(self._buffer[-1]) if self._buffer else None

    def get_all(self) -> list:
        """Return all buffered packets as a copy."""
        with self._lock:
            return list(self._buffer)

    def clear(self):
        """Clear the buffer."""
        with self._lock:
            self._buffer.clear()

    @staticmethod
    def list_ports() -> list[str]:
        """Return available serial port names."""
        return [p.device for p in serial.tools.list_ports.comports()]
