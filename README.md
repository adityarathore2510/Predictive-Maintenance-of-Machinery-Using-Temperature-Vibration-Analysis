# Predictive Machine Failure Monitoring System

[![Platform](https://img.shields.io/badge/Platform-ESP32-blue)](https://www.espressif.com/)
[![Framework](https://img.shields.io/badge/Framework-ESP--IDF%20%2B%20FreeRTOS-green)](https://docs.espressif.com/projects/esp-idf/)
[![Language](https://img.shields.io/badge/Firmware-Embedded%20C-orange)](https://gcc.gnu.org/)
[![Dashboard](https://img.shields.io/badge/Dashboard-Python%20%2B%20Matplotlib-yellow)](https://matplotlib.org/)
[![Status](https://img.shields.io/badge/Status-Active-brightgreen)]()

---

## Overview

A professional-grade industrial **Predictive Maintenance System** built on the ESP32 microcontroller.  
The system continuously monitors machine vibration and temperature, applies real-time signal filtering, performs threshold-based anomaly detection, and streams health data to a Python monitoring dashboard.

This project demonstrates core competencies in:
- **Embedded C / ESP-IDF / FreeRTOS** multi-task firmware design
- **Analog signal conditioning** and digital filtering
- **Industrial anomaly detection** with hysteresis state machines
- **Real-time data visualization** with Python desktop applications

---

## System Architecture

```
[Machine]
   ├── ADXL335 Accelerometer ──► ESP32 GPIO36 (ADC1_CH0)
   └── LM35 Temperature Sensor ► ESP32 GPIO39 (ADC1_CH3)
                                       │
                                   FreeRTOS
                                       │
                        ┌──────────────┼──────────────┐
                        │              │              │
                  task_sensor    task_filter    task_anomaly
                  (20 Hz ADC)  (Moving Avg)  (Hysteresis FSM)
                        │              │              │
                        └──────────────┼──────────────┘
                                       │
                              ┌────────┴────────┐
                              │                 │
                         task_alert         task_comm
                       (LED + Buzzer)    (JSON / UART2)
                                               │
                                         USB-Serial
                                               │
                                    Python Dashboard
                                   (Live Graphs + Alerts)
```

---

## Features

| Feature | Implementation |
|---------|---------------|
| Vibration Monitoring | ADXL335 analog accelerometer → 12-bit ADC |
| Temperature Monitoring | LM35 linear temperature sensor → 12-bit ADC |
| Signal Filtering | Circular buffer moving average (N=10, O(1) update) |
| Anomaly Detection | Threshold comparison with hysteresis state machine |
| Health Classification | NORMAL / WARNING / CRITICAL / FAULT states |
| Alert System | PWM buzzer (LEDC) + 3× GPIO LEDs |
| Serial Telemetry | JSON packets over UART2 at 500ms intervals |
| Real-Time Dashboard | Python Tkinter + Matplotlib, dark industrial theme |
| Trend Detection | Linear regression slope on rolling vibration window |
| Health Score | Composite 0–100% metric from severity values |
| Demo Mode | Dashboard runs without hardware (`--demo` flag) |

---

## Folder Structure

```
PredictiveMaintenance/
├── firmware/
│   ├── CMakeLists.txt
│   └── main/
│       ├── config.h          ← Global configuration (pins, thresholds, priorities)
│       ├── sensors.h/c       ← ADC sensor interface (ADXL335 + LM35)
│       ├── filter.h/c        ← Moving average filter (circular buffer)
│       ├── anomaly.h/c       ← Anomaly detection (hysteresis FSM)
│       ├── alert.h/c         ← LED + Buzzer alert management (LEDC)
│       ├── comm.h/c          ← JSON serial communication (UART2)
│       └── main.c            ← FreeRTOS task orchestration
├── dashboard/
│   ├── dashboard.py          ← Main dashboard application
│   ├── serial_reader.py      ← Threaded UART reader
│   ├── data_processor.py     ← Health score + trend analytics
│   └── requirements.txt
├── simulation/
│   ├── block_diagram.md      ← Full system block diagram
│   ├── circuit_diagram.md    ← Pin connections + schematic guide
│   ├── proteus_guide.md      ← Simulation setup instructions
│   └── component_list.md     ← BOM with justifications
└── docs/
    ├── architecture.md       ← Technical architecture deep-dive
    ├── interview_prep.md     ← 20 Q&A for placement interviews
    ├── resume_description.md ← Ready-to-paste resume bullets
    └── deployment_guide.md   ← Step-by-step setup guide
```

---

## Quick Start

### Option 1: Dashboard Demo (No Hardware Required)

```bash
cd dashboard
pip install -r requirements.txt
python dashboard.py --demo
```

The demo mode cycles through all health states automatically,
demonstrating the full dashboard functionality including graphs,
health score, trend detection, and event logging.

### Option 2: With ESP32 Hardware

**Firmware Setup (ESP-IDF v5.x):**
```bash
# Set up ESP-IDF environment
. $IDF_PATH/export.sh        # Linux/Mac
# or use ESP-IDF PowerShell on Windows

cd firmware
idf.py set-target esp32
idf.py build
idf.py flash monitor
```

**Dashboard:**
```bash
cd dashboard
pip install -r requirements.txt
python dashboard.py --port COM3     # Windows
python dashboard.py --port /dev/ttyUSB0    # Linux
```

---

## Hardware Requirements

| Component | Purpose | GPIO |
|-----------|---------|------|
| ESP32 DevKit V1 | Main controller | — |
| ADXL335 Module | Vibration sensing | GPIO36 |
| LM35 (TO-92) | Temperature sensing | GPIO39 |
| Green LED + 220Ω | NORMAL state indicator | GPIO26 |
| Yellow LED + 220Ω | WARNING state indicator | GPIO27 |
| Red LED + 220Ω | CRITICAL state indicator | GPIO14 |
| Passive Buzzer + 100Ω | Audio alert | GPIO25 |
| CP2102 USB-Serial | Dashboard communication | GPIO17 |

**Total BOM Cost: ≈ ₹800**

---

## Anomaly Detection Thresholds

| Parameter | Normal | Warning | Critical | Source |
|-----------|--------|---------|----------|--------|
| Vibration | < 0.5g | 0.5–1.0g | > 2.0g | ISO 10816 (simplified) |
| Temperature | < 40°C | 40–65°C | > 80°C | Industrial motor specs |
| Hysteresis (Vib) | ±0.1g | — | — | Prevents alert flicker |
| Hysteresis (Temp) | ±2°C | — | — | Prevents alert flicker |

---

## Serial Communication Protocol

**Baud Rate:** 115200  
**Format:** Newline-delimited JSON, one packet per 500ms

```json
{
  "id":     "ESP32-PM-01",
  "ts":     1234,
  "vib":    0.3421,
  "temp":   42.10,
  "vs":     0.171,
  "ts_s":   0.526,
  "health": "NORMAL",
  "alerts": 0
}
```

| Field | Description |
|-------|-------------|
| `id` | Device identifier |
| `ts` | Seconds since boot |
| `vib` | Filtered vibration (g) |
| `temp` | Filtered temperature (°C) |
| `vs` | Vibration severity (0.0–1.0) |
| `ts_s` | Temperature severity (0.0–1.0) |
| `health` | Machine health state |
| `alerts` | Total alert count |

---

## Industrial Application

This system architecture is directly applicable to:

- **Motor bearing fault detection** — vibration anomalies precede failure by 2–8 weeks
- **Pump condition monitoring** — cavitation and misalignment detection
- **HVAC system monitoring** — fan and compressor health
- **CNC machine tool monitoring** — spindle bearing wear detection
- **Industrial conveyor systems** — belt tension and roller degradation

---

## Future Improvements

- [ ] MQTT over WiFi → Broker → Grafana dashboard
- [ ] SD card logging for offline data retention
- [ ] FFT-based frequency analysis (identifies bearing fault frequency)
- [ ] Multiple sensor nodes (ESP-MESH networking)
- [ ] OTA firmware update capability
- [ ] Battery-powered low-power mode (deep sleep with periodic wakeup)

---

## Author

**Aditya Singh Rathore**  
B.Tech Electronics & Communication Engineering  
Project: Industrial Predictive Maintenance System  
Stack: ESP32 · Embedded C · FreeRTOS · ESP-IDF · Python · Matplotlib

---

*Built as a core ECE placement project demonstrating real-world embedded systems engineering.*
