"""
alert_sounds.py — Real Audio Alert Engine
==========================================
Generates and plays health-state-dependent WAV alert sounds
through the system's actual audio output (speakers/headphones).

APPROACH:
- WAV files are synthesized on first run using Python stdlib only
  (wave + struct + math) — ZERO external dependencies.
- Played asynchronously via winsound.PlaySound (SND_ASYNC flag).
- Each health state has a distinct, recognisable sound profile:

    NORMAL   → silent
    WARNING  → slow warbling tone (500 Hz ↔ 700 Hz, 0.8 s cycle)
    CRITICAL → rapid alternating siren (900 Hz ↔ 1700 Hz, fast sweep)
    FAULT    → continuous urgent triple-beep siren

SOUND FILES (auto-generated in same directory):
    alert_warning.wav
    alert_critical.wav
    alert_fault.wav
"""

import wave
import struct
import math
import os
import time
import threading
import platform

# Only import winsound on Windows
if platform.system() == "Windows":
    import winsound
    _HAS_SOUND = True
else:
    _HAS_SOUND = False

# Directory where WAV files are stored (same folder as this script)
_DIR = os.path.dirname(os.path.abspath(__file__))

SAMPLE_RATE = 44100   # Hz — CD quality
CHANNELS    = 1       # Mono
BIT_DEPTH   = 16      # 16-bit PCM


# ──────────────────────────────────────────────────────────
# WAV SYNTHESIS HELPERS
# ──────────────────────────────────────────────────────────

def _write_wav(filepath: str, samples: list[int]):
    """Write a list of 16-bit PCM samples to a WAV file."""
    with wave.open(filepath, 'w') as wf:
        wf.setnchannels(CHANNELS)
        wf.setsampwidth(BIT_DEPTH // 8)
        wf.setframerate(SAMPLE_RATE)
        packed = struct.pack(f"<{len(samples)}h", *samples)
        wf.writeframes(packed)


def _sine(freq: float, t: float, amplitude: float = 0.8) -> float:
    """Single sine sample at time t seconds."""
    return amplitude * math.sin(2.0 * math.pi * freq * t)


def _generate_warning_wav(filepath: str):
    """
    WARNING tone: slow warble between 500 Hz and 700 Hz over 0.8 s.
    Repeated twice for a ~1.6 s clip.
    Conveys: 'pay attention — something is off'.
    """
    duration   = 1.6     # seconds
    n_samples  = int(SAMPLE_RATE * duration)
    max_amp    = 32000
    warble_hz  = 1.25    # warble speed (cycles/second)

    samples = []
    for i in range(n_samples):
        t    = i / SAMPLE_RATE
        # Frequency sweeps 500–700 Hz sinusoidally
        freq = 600.0 + 100.0 * math.sin(2.0 * math.pi * warble_hz * t)
        # Soft envelope: fade in first 50ms, fade out last 50ms
        env  = 1.0
        fade = 0.05
        if t < fade:
            env = t / fade
        elif t > duration - fade:
            env = (duration - t) / fade
        s = int(max_amp * env * _sine(freq, t))
        samples.append(max(-32767, min(32767, s)))

    _write_wav(filepath, samples)


def _generate_critical_wav(filepath: str):
    """
    CRITICAL siren: fast sweep 900 Hz → 1700 Hz → 900 Hz, 0.4 s cycle.
    Three sweeps = 1.2 s clip.
    Conveys: 'urgent — immediate action required'.
    """
    duration  = 1.2
    n_samples = int(SAMPLE_RATE * duration)
    max_amp   = 32000
    sweep_hz  = 2.5   # sweeps per second

    samples = []
    for i in range(n_samples):
        t    = i / SAMPLE_RATE
        # Frequency sweeps 900–1700 Hz using triangle wave shape
        phase = (t * sweep_hz) % 1.0
        if phase < 0.5:
            freq = 900.0 + 1600.0 * (phase * 2.0)
        else:
            freq = 2500.0 - 1600.0 * (phase * 2.0)
        freq = max(900.0, min(1700.0, freq))

        env = 1.0
        fade = 0.02
        if t < fade:
            env = t / fade
        elif t > duration - fade:
            env = (duration - t) / fade

        s = int(max_amp * env * _sine(freq, t))
        samples.append(max(-32767, min(32767, s)))

    _write_wav(filepath, samples)


def _generate_fault_wav(filepath: str):
    """
    FAULT: three sharp 2000 Hz pulses with 80ms silence between.
    Total ~1.2 s. Conveys: 'system fault — stop the machine'.
    """
    pulse_dur   = 0.25   # seconds per pulse
    silence_dur = 0.10   # seconds between pulses
    n_pulses    = 3
    max_amp     = 32000

    samples = []
    for _ in range(n_pulses):
        # Pulse
        n = int(SAMPLE_RATE * pulse_dur)
        for i in range(n):
            t   = i / SAMPLE_RATE
            env = 1.0
            fade = 0.01
            if t < fade:
                env = t / fade
            elif t > pulse_dur - fade:
                env = (pulse_dur - t) / fade
            s = int(max_amp * env * _sine(2000.0, t))
            samples.append(max(-32767, min(32767, s)))
        # Silence
        n_sil = int(SAMPLE_RATE * silence_dur)
        samples.extend([0] * n_sil)

    _write_wav(filepath, samples)


# ──────────────────────────────────────────────────────────
# SOUND FILE REGISTRY
# ──────────────────────────────────────────────────────────

_SOUND_FILES = {
    "WARNING":  os.path.join(_DIR, "alert_warning.wav"),
    "CRITICAL": os.path.join(_DIR, "alert_critical.wav"),
    "FAULT":    os.path.join(_DIR, "alert_fault.wav"),
}

_GENERATORS = {
    "WARNING":  _generate_warning_wav,
    "CRITICAL": _generate_critical_wav,
    "FAULT":    _generate_fault_wav,
}


def ensure_sounds_exist():
    """
    Generate missing WAV files on first launch.
    Called once at startup — takes < 1 second total.
    """
    for key, path in _SOUND_FILES.items():
        if not os.path.exists(path):
            print(f"[AlertSound] Generating {os.path.basename(path)}...")
            _GENERATORS[key](path)
    print("[AlertSound] All WAV files ready.")


# ──────────────────────────────────────────────────────────
# ALERT SOUND ENGINE
# ──────────────────────────────────────────────────────────

# Repeat intervals per health state (seconds between plays)
_INTERVALS = {
    "NORMAL":   9999.0,
    "WARNING":  4.0,
    "CRITICAL": 2.0,
    "FAULT":    1.5,
}


class AlertSound:
    """
    Asynchronous health-state-dependent audio alert engine.

    Uses winsound.PlaySound with SND_ASYNC so audio never blocks the UI.
    A daemon thread watches health state and re-triggers at the right interval.

    Sound profiles:
        NORMAL   → silent
        WARNING  → slow warbling tone  (speakers)
        CRITICAL → fast siren sweep    (speakers)
        FAULT    → triple pulse alarm  (speakers)
    """

    def __init__(self):
        self._health     = "NORMAL"
        self._muted      = False
        self._running    = True
        self._lock       = threading.Lock()
        self._last_played = 0.0
        self._last_health = "NORMAL"

        if _HAS_SOUND:
            ensure_sounds_exist()

        self._thread = threading.Thread(
            target=self._loop, daemon=True, name="AlertSound")
        self._thread.start()

    def set_health(self, health: str):
        with self._lock:
            self._health = health

    def set_muted(self, muted: bool):
        with self._lock:
            self._muted = muted
            if muted and _HAS_SOUND:
                # Stop any currently playing sound immediately
                try:
                    winsound.PlaySound(None, winsound.SND_PURGE)
                except Exception:
                    pass

    def stop(self):
        self._running = False
        if _HAS_SOUND:
            try:
                winsound.PlaySound(None, winsound.SND_PURGE)
            except Exception:
                pass

    def _loop(self):
        while self._running:
            with self._lock:
                health = self._health
                muted  = self._muted

            now      = time.monotonic()
            interval = _INTERVALS.get(health, 9999.0)

            # Stop sound immediately when returning to NORMAL
            if health == "NORMAL" and self._last_health != "NORMAL":
                if _HAS_SOUND:
                    try:
                        winsound.PlaySound(None, winsound.SND_PURGE)
                    except Exception:
                        pass

            # Play alert sound if due
            if (not muted and _HAS_SOUND
                    and health != "NORMAL"
                    and now - self._last_played >= interval):

                wav_path = _SOUND_FILES.get(health)
                if wav_path and os.path.exists(wav_path):
                    try:
                        winsound.PlaySound(
                            wav_path,
                            winsound.SND_FILENAME | winsound.SND_ASYNC | winsound.SND_NODEFAULT
                        )
                        self._last_played = now
                    except Exception as e:
                        print(f"[AlertSound] Play error: {e}")

            self._last_health = health
            time.sleep(0.1)
