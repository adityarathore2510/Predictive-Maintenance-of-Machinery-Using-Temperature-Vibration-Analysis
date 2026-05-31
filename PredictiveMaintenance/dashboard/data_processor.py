"""
data_processor.py — Real-Time Data Processing & Health Analytics
================================================================
Processes incoming telemetry packets from the ESP32.

RESPONSIBILITIES:
- Maintains time-series data for plotting
- Computes rolling statistics (mean, std dev, trend slope)
- Calculates a composite Machine Health Score (0–100%)
- Detects upward trend in vibration (early warning indicator)

HEALTH SCORE FORMULA:
    health_score = 100 × (1 - max(vib_severity, temp_severity))
    
    This gives a linear, recruiter-explainable metric that maps
    directly to sensor severity values from the ESP32.

TREND DETECTION:
    Uses linear regression slope over the last N samples of vibration.
    A positive slope > threshold → trend alert ("Vibration Rising").
    This is predictive: we warn BEFORE the threshold is crossed.
"""

import time
import collections
import statistics


class DataProcessor:
    """
    Stateful processor for incoming telemetry packets.
    Maintains rolling windows and computes derived analytics.
    """

    MAX_SAMPLES = 300   # ~150 seconds of data at 500ms interval

    def __init__(self):
        # Time-series data (deque for O(1) append/pop)
        self.timestamps     = collections.deque(maxlen=self.MAX_SAMPLES)
        self.vibration      = collections.deque(maxlen=self.MAX_SAMPLES)
        self.temperature    = collections.deque(maxlen=self.MAX_SAMPLES)
        self.vib_severity   = collections.deque(maxlen=self.MAX_SAMPLES)
        self.temp_severity  = collections.deque(maxlen=self.MAX_SAMPLES)
        self.health_history = collections.deque(maxlen=self.MAX_SAMPLES)

        # Current machine state
        self.current_health     = "NORMAL"
        self.current_health_pct = 100.0
        self.alert_count        = 0
        self.session_start      = time.time()

        # Trend tracking
        self.trend_window       = 30   # samples for trend analysis
        self.vib_trend_slope    = 0.0
        self.trend_alert        = False

    def process(self, packet: dict) -> dict:
        """
        Ingest one telemetry packet and return an analytics result.

        :param packet: Parsed JSON packet from SerialReader
        :return: Dictionary with computed analytics
        """
        rx_time = packet.get('_rx_time', time.time())
        elapsed = rx_time - self.session_start

        vib  = float(packet.get('vib',  0.0))
        temp = float(packet.get('temp', 0.0))
        vs   = float(packet.get('vs',   0.0))
        ts_s = float(packet.get('ts_s', 0.0))
        health_str = packet.get('health', 'NORMAL')
        alerts     = int(packet.get('alerts', 0))

        # Append to time series
        self.timestamps.append(elapsed)
        self.vibration.append(vib)
        self.temperature.append(temp)
        self.vib_severity.append(vs)
        self.temp_severity.append(ts_s)
        self.health_history.append(health_str)

        # Update state
        self.current_health = health_str
        self.alert_count    = alerts

        # Compute health score: 0% = worst, 100% = perfect
        composite_severity    = max(vs, ts_s)
        self.current_health_pct = max(0.0, 100.0 * (1.0 - composite_severity))

        # Trend analysis: linear regression slope on vibration
        self.vib_trend_slope = 0.0
        self.trend_alert     = False
        if len(self.vibration) >= self.trend_window:
            recent_vib = list(self.vibration)[-self.trend_window:]
            recent_t   = list(range(self.trend_window))
            slope = self._linear_slope(recent_t, recent_vib)
            self.vib_trend_slope = slope
            # If rising at > 0.01g per sample → predictive warning
            if slope > 0.01:
                self.trend_alert = True

        # Current statistics
        vib_list  = list(self.vibration)
        temp_list = list(self.temperature)

        result = {
            'vib':           vib,
            'temp':          temp,
            'health':        health_str,
            'health_pct':    self.current_health_pct,
            'alert_count':   self.alert_count,
            'vib_mean':      statistics.mean(vib_list)  if vib_list  else 0.0,
            'temp_mean':     statistics.mean(temp_list) if temp_list else 0.0,
            'vib_std':       statistics.stdev(vib_list) if len(vib_list) > 1 else 0.0,
            'trend_slope':   self.vib_trend_slope,
            'trend_alert':   self.trend_alert,
            'elapsed':       elapsed,
        }
        return result

    def get_plot_data(self) -> dict:
        """Return time-series data for the plotter module."""
        return {
            'timestamps':   list(self.timestamps),
            'vibration':    list(self.vibration),
            'temperature':  list(self.temperature),
        }

    def reset(self):
        """Reset all collected data (session restart)."""
        self.__init__()

    @staticmethod
    def _linear_slope(x: list, y: list) -> float:
        """
        Compute linear regression slope using least-squares formula.
        slope = (N*Σxy - Σx*Σy) / (N*Σx² - (Σx)²)
        """
        n = len(x)
        if n < 2:
            return 0.0
        sum_x  = sum(x)
        sum_y  = sum(y)
        sum_xy = sum(xi * yi for xi, yi in zip(x, y))
        sum_x2 = sum(xi * xi for xi in x)
        denom  = n * sum_x2 - sum_x ** 2
        if denom == 0:
            return 0.0
        return (n * sum_xy - sum_x * sum_y) / denom
