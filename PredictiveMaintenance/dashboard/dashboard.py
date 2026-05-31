"""
dashboard.py — Industrial Predictive Maintenance Dashboard
===========================================================
Real-time monitoring dashboard for ESP32-based machine health system.

ARCHITECTURE:
- Tkinter: Lightweight desktop GUI (no browser/server needed)
- Matplotlib: Embedded real-time graphs via FigureCanvasTkAgg
- SerialReader: Background thread reads ESP32 UART output
- DataProcessor: Computes health score, trends, statistics
- Refresh rate: 500ms (matches ESP32 transmission interval)

DESIGN DECISIONS:
- Dark industrial theme (#0d1117 background) for professional appearance
- Color-coded health indicators matching firmware LED behavior:
    GREEN  → NORMAL   (#00ff88)
    YELLOW → WARNING  (#ffcc00)
    RED    → CRITICAL (#ff4444)
- Separate graph panels for vibration and temperature
- Real-time statistics panel with trend indicator

USAGE:
    python dashboard.py --port COM3
    python dashboard.py --port COM3 --demo    # Run with simulated data
"""

import tkinter as tk
from tkinter import ttk, messagebox
import matplotlib
matplotlib.use("TkAgg")
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
import argparse
import time
import math
import random
import sys
import threading
import platform

from serial_reader  import SerialReader
from data_processor import DataProcessor
from alert_sounds   import AlertSound, _HAS_SOUND   # real WAV-based audio engine

# ─────────────────────────────────────────────
# COLOR PALETTE — Industrial Dark Theme
# ─────────────────────────────────────────────
BG_DARK      = "#0d1117"
BG_PANEL     = "#161b22"
BG_CARD      = "#1c2128"
ACCENT_BLUE  = "#58a6ff"
ACCENT_GREEN = "#00ff88"
ACCENT_YELL  = "#ffcc00"
ACCENT_RED   = "#ff4444"
ACCENT_GRAY  = "#8b949e"
TEXT_WHITE   = "#e6edf3"
TEXT_DIM     = "#6e7681"
BORDER_COLOR = "#30363d"

HEALTH_COLORS = {
    "NORMAL":   ACCENT_GREEN,
    "WARNING":  ACCENT_YELL,
    "CRITICAL": ACCENT_RED,
    "FAULT":    "#ff00ff",
}

plt.rcParams.update({
    "figure.facecolor":  BG_PANEL,
    "axes.facecolor":    BG_DARK,
    "axes.edgecolor":    BORDER_COLOR,
    "axes.labelcolor":   ACCENT_GRAY,
    "xtick.color":       ACCENT_GRAY,
    "ytick.color":       ACCENT_GRAY,
    "grid.color":        BORDER_COLOR,
    "grid.linestyle":    "--",
    "grid.alpha":        0.5,
    "text.color":        TEXT_WHITE,
    "font.family":       "monospace",
})


# ─────────────────────────────────────────────
# DEMO DATA GENERATOR (for simulation without hardware)
# ─────────────────────────────────────────────
# ─────────────────────────────────────────────
# DEMO DATA GENERATOR
# ─────────────────────────────────────────────
class DemoDataGenerator:
    """
    Generates realistic ESP32-style telemetry for dashboard testing.
    temp_offset: user-controlled temperature adjustment (°C) applied on top.
    """

    def __init__(self):
        self._t          = 0
        self._phase      = "normal"
        self._phase_t    = 0
        self.temp_offset = 0.0   # ← adjusted by +/- buttons

    def next_packet(self) -> dict:
        self._t      += 1
        self._phase_t += 1

        if self._phase_t > 60:
            phases = ["normal", "warning", "critical", "normal"]
            idx    = (phases.index(self._phase) + 1) % len(phases)
            self._phase   = phases[idx]
            self._phase_t = 0

        if self._phase == "normal":
            vib  = 0.2 + 0.15 * abs(math.sin(self._t * 0.1)) + random.gauss(0, 0.03)
            temp = 35.0 + 3.0 * math.sin(self._t * 0.05) + random.gauss(0, 0.5)
            health = "NORMAL"
        elif self._phase == "warning":
            vib  = 1.1 + 0.3 * abs(math.sin(self._t * 0.2)) + random.gauss(0, 0.05)
            temp = 68.0 + 2.0 * math.sin(self._t * 0.1) + random.gauss(0, 1.0)
            health = "WARNING"
        elif self._phase == "critical":
            vib  = 2.3 + 0.4 * abs(math.sin(self._t * 0.3)) + random.gauss(0, 0.1)
            temp = 82.0 + 3.0 * math.sin(self._t * 0.08) + random.gauss(0, 1.5)
            health = "CRITICAL"
        else:
            vib  = 0.25 + random.gauss(0, 0.03)
            temp = 38.0 + random.gauss(0, 0.5)
            health = "NORMAL"

        vib  = max(0.0, vib)
        # Apply user-controlled temperature offset
        temp = max(0.0, temp + self.temp_offset)

        # Recalculate health based on adjusted temperature
        if temp >= 80.0 or vib >= 2.0:
            health = "CRITICAL"
        elif temp >= 65.0 or vib >= 1.0:
            health = "WARNING"

        vs   = min(1.0, vib  / 2.0)
        ts_s = min(1.0, temp / 80.0)

        return {
            'id':      'ESP32-PM-01',
            'ts':       self._t,
            'vib':      round(vib,  4),
            'temp':     round(temp, 2),
            'vs':       round(vs,   3),
            'ts_s':     round(ts_s, 3),
            'health':   health,
            'alerts':   0,
            '_rx_time': time.time(),
        }


# ─────────────────────────────────────────────
# MAIN DASHBOARD APPLICATION
# ─────────────────────────────────────────────
class Dashboard:

    REFRESH_MS = 500  # Refresh interval in milliseconds

    def __init__(self, root: tk.Tk, reader=None, demo_gen=None):
        self.root      = root
        self.reader    = reader
        self.demo_gen  = demo_gen
        self.processor = DataProcessor()
        self._sound    = AlertSound()
        self._muted    = False

        self._setup_window()
        self._build_header()
        self._build_status_bar()
        self._build_temp_controls()   # ← NEW: +/- temp adjustment
        self._build_graphs()
        self._build_stats_panel()
        self._build_alert_log()
        self._start_refresh()

    # ── Window Setup ──────────────────────────
    def _setup_window(self):
        self.root.title("Predictive Maintenance System — ESP32 Dashboard v1.0")
        self.root.configure(bg=BG_DARK)
        self.root.minsize(1280, 780)
        self.root.geometry("1400x820")

    # ── Header Bar ────────────────────────────
    def _build_header(self):
        hdr = tk.Frame(self.root, bg=BG_PANEL, pady=10)
        hdr.pack(fill="x", side="top")

        tk.Label(hdr, text="⚙  PREDICTIVE MAINTENANCE SYSTEM",
                 font=("Courier New", 16, "bold"),
                 fg=ACCENT_BLUE, bg=BG_PANEL).pack(side="left", padx=20)

        self._lbl_time = tk.Label(hdr, text="",
                                   font=("Courier New", 11),
                                   fg=ACCENT_GRAY, bg=BG_PANEL)
        self._lbl_time.pack(side="right", padx=20)

        tk.Label(hdr, text=f"Device: ESP32-PM-01  |  FW: 1.0.0",
                 font=("Courier New", 10), fg=TEXT_DIM, bg=BG_PANEL
                 ).pack(side="right", padx=10)

    # ── Status Bar ────────────────────────────
    def _build_status_bar(self):
        bar = tk.Frame(self.root, bg=BG_CARD, pady=12)
        bar.pack(fill="x", padx=10, pady=(6, 0))

        # Machine Health State
        tk.Label(bar, text="MACHINE HEALTH",
                 font=("Courier New", 9), fg=TEXT_DIM, bg=BG_CARD
                 ).grid(row=0, column=0, padx=20)
        self._lbl_health = tk.Label(bar, text="● NORMAL",
                                     font=("Courier New", 14, "bold"),
                                     fg=ACCENT_GREEN, bg=BG_CARD)
        self._lbl_health.grid(row=1, column=0, padx=20)

        # Health Score %
        tk.Label(bar, text="HEALTH SCORE",
                 font=("Courier New", 9), fg=TEXT_DIM, bg=BG_CARD
                 ).grid(row=0, column=1, padx=20)
        self._lbl_score = tk.Label(bar, text="100.0%",
                                    font=("Courier New", 14, "bold"),
                                    fg=ACCENT_GREEN, bg=BG_CARD)
        self._lbl_score.grid(row=1, column=1, padx=20)

        # Vibration
        tk.Label(bar, text="VIBRATION (g)",
                 font=("Courier New", 9), fg=TEXT_DIM, bg=BG_CARD
                 ).grid(row=0, column=2, padx=20)
        self._lbl_vib = tk.Label(bar, text="0.000 g",
                                  font=("Courier New", 14, "bold"),
                                  fg=TEXT_WHITE, bg=BG_CARD)
        self._lbl_vib.grid(row=1, column=2, padx=20)

        # Temperature
        tk.Label(bar, text="TEMPERATURE (°C)",
                 font=("Courier New", 9), fg=TEXT_DIM, bg=BG_CARD
                 ).grid(row=0, column=3, padx=20)
        self._lbl_temp = tk.Label(bar, text="0.0 °C",
                                   font=("Courier New", 14, "bold"),
                                   fg=TEXT_WHITE, bg=BG_CARD)
        self._lbl_temp.grid(row=1, column=3, padx=20)

        # Alert Count
        tk.Label(bar, text="TOTAL ALERTS",
                 font=("Courier New", 9), fg=TEXT_DIM, bg=BG_CARD
                 ).grid(row=0, column=4, padx=20)
        self._lbl_alerts = tk.Label(bar, text="0",
                                     font=("Courier New", 14, "bold"),
                                     fg=ACCENT_GRAY, bg=BG_CARD)
        self._lbl_alerts.grid(row=1, column=4, padx=20)

        # Trend indicator
        tk.Label(bar, text="VIBRATION TREND",
                 font=("Courier New", 9), fg=TEXT_DIM, bg=BG_CARD
                 ).grid(row=0, column=5, padx=20)
        self._lbl_trend = tk.Label(bar, text="STABLE  →",
                                    font=("Courier New", 13, "bold"),
                                    fg=ACCENT_GREEN, bg=BG_CARD)
        self._lbl_trend.grid(row=1, column=5, padx=20)

        # Connection status
        self._lbl_conn = tk.Label(bar, text="◉ CONNECTED",
                                   font=("Courier New", 10),
                                   fg=ACCENT_GREEN, bg=BG_CARD)
        self._lbl_conn.grid(row=0, column=6, padx=30, rowspan=2)

        # Mute button
        self._btn_mute = tk.Button(
            bar, text="🔊 SOUND ON",
            font=("Courier New", 9, "bold"),
            bg="#1c2128", fg=ACCENT_GREEN,
            activebackground="#30363d", activeforeground=ACCENT_YELL,
            relief="flat", bd=0, padx=10, pady=4,
            cursor="hand2",
            command=self._toggle_mute
        )
        self._btn_mute.grid(row=0, column=7, padx=15, rowspan=2)

    # ── Temperature Manual Control ────────────
    def _build_temp_controls(self):
        """
        Interactive temperature adjustment panel.
        Allows user to manually inject a +/- offset into the demo data stream
        to trigger WARNING / CRITICAL states on demand — great for demo.
        Only active in demo mode.
        """
        ctrl = tk.Frame(self.root, bg=BG_PANEL, pady=6)
        ctrl.pack(fill="x", padx=10, pady=(4, 0))

        tk.Label(ctrl, text="MANUAL TEMP OVERRIDE (DEMO)",
                 font=("Courier New", 9, "bold"),
                 fg=ACCENT_BLUE, bg=BG_PANEL).pack(side="left", padx=12)

        # Current offset display
        self._temp_offset_var = tk.StringVar(value="Δ +0.0 °C")
        tk.Label(ctrl, textvariable=self._temp_offset_var,
                 font=("Courier New", 12, "bold"),
                 fg=ACCENT_YELL, bg=BG_PANEL, width=12).pack(side="left", padx=6)

        btn_style = dict(
            font=("Courier New", 14, "bold"),
            relief="flat", bd=0,
            bg=BG_CARD, activebackground="#30363d",
            cursor="hand2", width=3, pady=2
        )

        tk.Button(ctrl, text="＋", fg=ACCENT_GREEN,
                  activeforeground=ACCENT_GREEN,
                  command=lambda: self._adjust_temp(+5.0),
                  **btn_style).pack(side="left", padx=4)

        tk.Button(ctrl, text="－", fg=ACCENT_RED,
                  activeforeground=ACCENT_RED,
                  command=lambda: self._adjust_temp(-5.0),
                  **btn_style).pack(side="left", padx=4)

        tk.Button(ctrl, text="RESET",
                  font=("Courier New", 9, "bold"),
                  fg=TEXT_DIM, activeforeground=TEXT_WHITE,
                  bg=BG_CARD, activebackground="#30363d",
                  relief="flat", bd=0, padx=8, pady=4,
                  cursor="hand2",
                  command=lambda: self._adjust_temp(None)).pack(side="left", padx=6)

        # Threshold hints
        tk.Label(ctrl,
                 text="[ Warning >65°C  |  Critical >80°C ]  Each step: ±5°C",
                 font=("Courier New", 8), fg=TEXT_DIM, bg=BG_PANEL
                 ).pack(side="left", padx=16)

    def _adjust_temp(self, delta):
        """Apply delta to demo generator temp_offset, or reset to 0."""
        if self.demo_gen is None:
            return
        if delta is None:
            self.demo_gen.temp_offset = 0.0
        else:
            self.demo_gen.temp_offset = max(-30.0, min(60.0,
                self.demo_gen.temp_offset + delta))
        offset = self.demo_gen.temp_offset
        sign   = "+" if offset >= 0 else ""
        self._temp_offset_var.set(f"Δ {sign}{offset:.1f} °C")
        self._log_event(
            f"Manual temp override: Δ{sign}{offset:.1f}°C applied", "INFO")

    def _toggle_mute(self):
        self._muted = not self._muted
        self._sound.set_muted(self._muted)
        if self._muted:
            self._btn_mute.config(text="🔇 MUTED", fg=ACCENT_GRAY)
        else:
            self._btn_mute.config(text="🔊 SOUND ON", fg=ACCENT_GREEN)

    # ── Live Graphs ───────────────────────────
    def _build_graphs(self):
        graph_frame = tk.Frame(self.root, bg=BG_DARK)
        graph_frame.pack(fill="both", expand=True, padx=10, pady=8)

        self.fig, (self.ax_vib, self.ax_temp) = plt.subplots(
            2, 1, figsize=(10, 5), sharex=False)
        self.fig.tight_layout(pad=2.5)

        # Vibration axes
        self.ax_vib.set_ylabel("Vibration (g)", color=ACCENT_GRAY)
        self.ax_vib.set_title("Real-Time Vibration — ADXL335",
                               color=TEXT_WHITE, fontsize=10)
        self.ax_vib.axhline(y=1.0, color=ACCENT_YELL,
                             linestyle='--', alpha=0.6, label="Warning (1.0g)")
        self.ax_vib.axhline(y=2.0, color=ACCENT_RED,
                             linestyle='--', alpha=0.6, label="Critical (2.0g)")
        self.ax_vib.legend(loc='upper right', fontsize=8,
                            facecolor=BG_CARD, labelcolor=TEXT_WHITE)
        self.ax_vib.grid(True)
        self.ax_vib.set_ylim(0, 3.0)

        self.line_vib, = self.ax_vib.plot([], [], color=ACCENT_BLUE,
                                            linewidth=1.5, label="Vibration")

        # Temperature axes
        self.ax_temp.set_ylabel("Temperature (°C)", color=ACCENT_GRAY)
        self.ax_temp.set_xlabel("Time (s)", color=ACCENT_GRAY)
        self.ax_temp.set_title("Real-Time Temperature — LM35",
                                color=TEXT_WHITE, fontsize=10)
        self.ax_temp.axhline(y=65.0, color=ACCENT_YELL,
                              linestyle='--', alpha=0.6, label="Warning (65°C)")
        self.ax_temp.axhline(y=80.0, color=ACCENT_RED,
                              linestyle='--', alpha=0.6, label="Critical (80°C)")
        self.ax_temp.legend(loc='upper right', fontsize=8,
                             facecolor=BG_CARD, labelcolor=TEXT_WHITE)
        self.ax_temp.grid(True)
        self.ax_temp.set_ylim(0, 100.0)

        self.line_temp, = self.ax_temp.plot([], [], color="#ff9944",
                                              linewidth=1.5, label="Temperature")

        canvas = FigureCanvasTkAgg(self.fig, master=graph_frame)
        canvas.get_tk_widget().pack(fill="both", expand=True)
        self._canvas = canvas

    # ── Stats Panel ───────────────────────────
    def _build_stats_panel(self):
        stats_frame = tk.Frame(self.root, bg=BG_CARD, pady=8)
        stats_frame.pack(fill="x", padx=10, pady=4)

        labels = ["VIB MEAN", "VIB σ", "TEMP MEAN",
                  "TREND SLOPE", "SESSION TIME", "DEVICE ID"]
        self._stat_vars = {}
        for i, lbl in enumerate(labels):
            tk.Label(stats_frame, text=lbl, font=("Courier New", 8),
                     fg=TEXT_DIM, bg=BG_CARD).grid(row=0, column=i, padx=15)
            var = tk.StringVar(value="—")
            tk.Label(stats_frame, textvariable=var,
                     font=("Courier New", 11, "bold"),
                     fg=ACCENT_GRAY, bg=BG_CARD).grid(row=1, column=i, padx=15)
            self._stat_vars[lbl] = var

    # ── Alert Log ─────────────────────────────
    def _build_alert_log(self):
        log_frame = tk.Frame(self.root, bg=BG_PANEL)
        log_frame.pack(fill="x", padx=10, pady=(0, 8))

        tk.Label(log_frame, text="EVENT LOG",
                 font=("Courier New", 9, "bold"),
                 fg=ACCENT_BLUE, bg=BG_PANEL).pack(anchor="w", padx=8, pady=2)

        self._log_text = tk.Text(log_frame, height=4,
                                  bg=BG_DARK, fg=ACCENT_GRAY,
                                  font=("Courier New", 9),
                                  state="disabled", relief="flat",
                                  insertbackground=TEXT_WHITE)
        self._log_text.pack(fill="x", padx=8, pady=2)
        self._log_text.tag_config("WARNING",  foreground=ACCENT_YELL)
        self._log_text.tag_config("CRITICAL", foreground=ACCENT_RED)
        self._log_text.tag_config("NORMAL",   foreground=ACCENT_GREEN)
        self._log_text.tag_config("INFO",     foreground=ACCENT_GRAY)

        self._last_health = "NORMAL"

    def _log_event(self, msg: str, tag: str = "INFO"):
        ts = time.strftime("%H:%M:%S")
        self._log_text.configure(state="normal")
        self._log_text.insert("end", f"[{ts}] {msg}\n", tag)
        self._log_text.see("end")
        self._log_text.configure(state="disabled")

    # ── Refresh Loop ──────────────────────────
    def _start_refresh(self):
        sound_note = "(sound enabled)" if _HAS_SOUND else "(sound N/A — non-Windows)"
        self._log_event(f"Dashboard started. Waiting for telemetry... {sound_note}", "INFO")
        self._refresh()

    def _refresh(self):
        """Main update loop — called every REFRESH_MS milliseconds."""
        packet = None

        if self.demo_gen:
            packet = self.demo_gen.next_packet()
        elif self.reader:
            packet = self.reader.get_latest()

        if packet:
            analytics = self.processor.process(packet)
            self._update_status(analytics)
            self._update_graphs()
            self._update_stats(analytics)

            new_health = analytics['health']

            # Feed health state to audio engine EVERY tick
            self._sound.set_health(new_health)

            # Log health state changes
            if new_health != self._last_health:
                sound_desc = {
                    "NORMAL":   "🔇 silent",
                    "WARNING":  "🔔 slow beep (600 Hz)",
                    "CRITICAL": "🚨 rapid burst (1200 Hz)",
                    "FAULT":    "⚠ siren (2000 Hz)",
                }.get(new_health, "")
                self._log_event(
                    f"Health → {new_health}  "
                    f"(Vib={analytics['vib']:.3f}g, Temp={analytics['temp']:.1f}°C)  "
                    f"Audio: {sound_desc}",
                    tag=new_health
                )
                self._last_health = new_health

            if analytics.get('trend_alert') and new_health == "NORMAL":
                self._log_event(
                    f"⚠ Vibration trend rising (slope={analytics['trend_slope']:.4f} g/sample)",
                    tag="WARNING"
                )

        # Update clock
        self._lbl_time.config(text=time.strftime("  %Y-%m-%d  %H:%M:%S  "))
        self.root.after(self.REFRESH_MS, self._refresh)

    def _update_status(self, a: dict):
        health  = a['health']
        color   = HEALTH_COLORS.get(health, ACCENT_GRAY)
        score   = a['health_pct']

        self._lbl_health.config(text=f"● {health}", fg=color)
        self._lbl_score.config(
            text=f"{score:.1f}%",
            fg=(ACCENT_GREEN if score >= 70 else ACCENT_YELL if score >= 40 else ACCENT_RED)
        )
        self._lbl_vib.config(
            text=f"{a['vib']:.3f} g",
            fg=(ACCENT_RED if a['vib'] >= 2.0 else ACCENT_YELL if a['vib'] >= 1.0 else TEXT_WHITE)
        )
        self._lbl_temp.config(
            text=f"{a['temp']:.1f} °C",
            fg=(ACCENT_RED if a['temp'] >= 80 else ACCENT_YELL if a['temp'] >= 65 else TEXT_WHITE)
        )
        self._lbl_alerts.config(
            text=str(a['alert_count']),
            fg=(ACCENT_RED if a['alert_count'] > 5 else ACCENT_GRAY)
        )

        if a.get('trend_alert'):
            self._lbl_trend.config(text="RISING  ↑", fg=ACCENT_YELL)
        else:
            self._lbl_trend.config(text="STABLE  →", fg=ACCENT_GREEN)

        if self.reader and not self.reader.connected:
            self._lbl_conn.config(text="◯ DISCONNECTED", fg=ACCENT_RED)
        else:
            self._lbl_conn.config(text="◉ CONNECTED", fg=ACCENT_GREEN)

    def _update_graphs(self):
        plot_data = self.processor.get_plot_data()
        ts  = plot_data['timestamps']
        vib = plot_data['vibration']
        tmp = plot_data['temperature']

        if not ts:
            return

        self.line_vib.set_data(ts, vib)
        self.ax_vib.set_xlim(max(0, ts[-1] - 60), ts[-1] + 2)

        self.line_temp.set_data(ts, tmp)
        self.ax_temp.set_xlim(max(0, ts[-1] - 60), ts[-1] + 2)

        self._canvas.draw_idle()

    def _update_stats(self, a: dict):
        elapsed = a['elapsed']
        mm = int(elapsed // 60)
        ss = int(elapsed % 60)
        self._stat_vars["VIB MEAN"].set(f"{a['vib_mean']:.3f}g")
        self._stat_vars["VIB σ"].set(f"{a['vib_std']:.3f}g")
        self._stat_vars["TEMP MEAN"].set(f"{a['temp_mean']:.1f}°C")
        self._stat_vars["TREND SLOPE"].set(f"{a['trend_slope']:.4f}")
        self._stat_vars["SESSION TIME"].set(f"{mm:02d}:{ss:02d}")
        self._stat_vars["DEVICE ID"].set("ESP32-PM-01")


# ─────────────────────────────────────────────
# ENTRY POINT
# ─────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(
        description="Predictive Maintenance Dashboard — ESP32")
    parser.add_argument("--port",  type=str, default=None,
                        help="Serial port (e.g. COM3 or /dev/ttyUSB0)")
    parser.add_argument("--baud",  type=int, default=115200,
                        help="Baud rate (default: 115200)")
    parser.add_argument("--demo",  action="store_true",
                        help="Run with simulated data (no hardware needed)")
    args = parser.parse_args()

    root = tk.Tk()
    reader   = None
    demo_gen = None

    if args.demo:
        demo_gen = DemoDataGenerator()
        print("[INFO] Running in DEMO mode — no hardware required.")
    elif args.port:
        reader = SerialReader(port=args.port, baud=args.baud)
        if not reader.start():
            messagebox.showerror("Serial Error",
                f"Could not open {args.port}.\n{reader.error_msg}")
            sys.exit(1)
        print(f"[INFO] Connected to {args.port} at {args.baud} baud.")
    else:
        # Auto-select demo mode if no port given
        print("[INFO] No --port specified. Starting in DEMO mode.")
        demo_gen = DemoDataGenerator()

    app = Dashboard(root, reader=reader, demo_gen=demo_gen)

    def on_close():
        if reader:
            reader.stop()
        app._sound.stop()
        root.destroy()

    root.protocol("WM_DELETE_WINDOW", on_close)
    root.mainloop()


if __name__ == "__main__":
    main()
