# Resume Description — Predictive Maintenance System

## Project Title (for Resume Header)

**Predictive Machine Failure Monitoring System** | ESP32 · Embedded C · FreeRTOS · Python

---

## Short Version (1 Line — for Skills Section)

> ESP32-based industrial predictive maintenance system with FreeRTOS firmware, ADC sensor fusion, signal filtering, and real-time Python dashboard.

---

## Medium Version (3 Bullets — for Projects Section)

- Built a **real-time machine health monitoring system** on ESP32 using ADXL335 accelerometer and LM35 temperature sensor, with modular **ESP-IDF firmware** structured across 6 source files following industrial coding standards.
- Implemented a **moving average filter** (O(1) circular buffer) and **hysteresis-based anomaly detection FSM** classifying machine state as NORMAL / WARNING / CRITICAL, with GPIO LED and PWM buzzer alert outputs.
- Developed a **Python desktop dashboard** (Tkinter + Matplotlib) that reads JSON telemetry from UART, plots live vibration and temperature graphs, computes a composite health score, and detects rising trends via linear regression.

---

## Full Version (5 Bullets — for Detailed Project Section)

- Designed a **multi-task FreeRTOS firmware** on ESP32 (Xtensa LX6 dual-core) with five concurrent tasks — sensor acquisition (20Hz), moving average filter, hysteresis anomaly detector, alert manager, and serial communicator — interconnected via thread-safe queues and mutex semaphore.
- Implemented **analog sensor interfacing** using ESP32 12-bit ADC1 with 11dB attenuation; converted raw ADC counts to g-force (ADXL335) and Celsius (LM35) using datasheet-derived sensitivity formulas.
- Applied a **10-sample circular buffer moving average filter** with O(1) running-sum update to suppress ADC noise while preserving machine condition trends critical for fault detection.
- Designed a **state-machine anomaly detector** with configurable hysteresis bands (±0.1g, ±2°C) preventing alert flicker at state boundaries, and ISO 10816-inspired vibration thresholds (Warning: 1.0g, Critical: 2.0g).
- Built a dark-themed **industrial Python dashboard** with real-time Matplotlib graphs, health score display, vibration trend detection (linear regression slope), event log, and a built-in hardware-free demo mode.

---

## Technical Skills Demonstrated

| Skill Area | Evidence |
|---|---|
| Embedded C Programming | 6 modular source files, clean API design |
| FreeRTOS | 5 tasks, queues, mutex, core affinity |
| ADC & Sensor Interfacing | ADXL335 + LM35 with unit conversion |
| Signal Processing | Moving average filter, circular buffer |
| Digital Control Techniques | Hysteresis FSM, threshold detection |
| Serial Communication | UART2 JSON protocol, 115200 baud |
| Python Desktop Development | Tkinter + Matplotlib dashboard |
| Data Analytics | Health score, linear regression trend |
| Modular Architecture | Config-driven, single-responsibility modules |

---

## One-Liner for LinkedIn / Introduction

> "Built a full-stack embedded systems predictive maintenance project: ESP32 FreeRTOS firmware for vibration and temperature monitoring with hysteresis anomaly detection, integrated with a real-time Python dashboard for industrial machine health visualization."

---

## GitHub Repository Description (One Line)

> ESP32-based industrial predictive maintenance system — FreeRTOS firmware + Python dashboard for real-time machine vibration and temperature monitoring with anomaly detection.

---

## Tags / Keywords for Job Applications

`ESP32` `Embedded C` `FreeRTOS` `ESP-IDF` `ADC` `ADXL335` `LM35` `UART` `Signal Processing`
`Moving Average Filter` `Anomaly Detection` `Predictive Maintenance` `Real-Time Systems`
`Python` `Matplotlib` `Tkinter` `Industrial IoT` `Industry 4.0` `RTOS`
