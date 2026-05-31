# System Block Diagram — Predictive Maintenance System

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                     PHYSICAL MACHINE (Simulated)                    │
│                                                                     │
│   ┌──────────────┐          ┌──────────────────┐                   │
│   │  ADXL335     │          │  LM35 Temp Sensor│                   │
│   │  3-Axis      │          │  0°C–100°C        │                   │
│   │  Accelerometer│         │  10mV/°C linear   │                   │
│   │  Analog Out  │          │  Analog Out       │                   │
│   └──────┬───────┘          └────────┬──────────┘                   │
│          │                           │                              │
└──────────┼───────────────────────────┼──────────────────────────────┘
           │ Vout (0–3.3V)             │ Vout (0–1.0V)
           ▼ GPIO36 (ADC1_CH0)         ▼ GPIO39 (ADC1_CH3)
┌─────────────────────────────────────────────────────────────────────┐
│                       ESP32 (Xtensa LX6 Dual Core)                  │
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                     FreeRTOS Kernel                         │   │
│  │                                                             │   │
│  │  Core 1 (Real-Time):          Core 0 (Communication):      │   │
│  │  ┌─────────────────┐          ┌──────────────────────┐      │   │
│  │  │  task_sensor    │          │  task_alert          │      │   │
│  │  │  (Priority: 5)  │          │  (Priority: 3)       │      │   │
│  │  │  20Hz sampling  │          │  LED/Buzzer control  │      │   │
│  │  └────────┬────────┘          └──────────────────────┘      │   │
│  │           │ Queue             ┌──────────────────────┐      │   │
│  │  ┌────────▼────────┐          │  task_comm           │      │   │
│  │  │  task_filter    │          │  (Priority: 2)       │      │   │
│  │  │  (Priority: 4)  │          │  JSON via UART2      │      │   │
│  │  │  Moving Avg     │          └──────────────────────┘      │   │
│  │  └────────┬────────┘                                        │   │
│  │           │ Queue                                           │   │
│  │  ┌────────▼────────┐                                        │   │
│  │  │  task_anomaly   │──Shared Result──► task_alert           │   │
│  │  │  (Priority: 3)  │                ► task_comm             │   │
│  │  │  Hysteresis FSM │                                        │   │
│  │  └─────────────────┘                                        │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                                                                     │
│  GPIO OUTPUT:                                                        │
│  GPIO25 → Buzzer (LEDC PWM)                                         │
│  GPIO26 → Green LED (NORMAL)                                        │
│  GPIO27 → Yellow LED (WARNING)                                      │
│  GPIO14 → Red LED (CRITICAL)                                        │
│  GPIO17 → UART2 TX → USB-Serial → PC Dashboard                     │
└─────────────────────────────────────────────────────────────────────┘
                                │
                         UART2 / USB-Serial
                                │
┌─────────────────────────────────────────────────────────────────────┐
│                    PC — Python Dashboard                             │
│                                                                     │
│  SerialReader (Thread) → DataProcessor → Tkinter/Matplotlib GUI     │
│                                                                     │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────────────┐  │
│  │ JSON Parser  │ →  │ Health Score │ →  │  Live Graphs         │  │
│  │ (line-based) │    │ Trend Detect │    │  Status Panel        │  │
│  └──────────────┘    └──────────────┘    │  Alert Log           │  │
│                                          └──────────────────────┘  │
└─────────────────────────────────────────────────────────────────────┘
```

## Data Flow Summary

```
Sensor → ADC → Engineering Units → Moving Average Filter
       → Threshold Comparison (with Hysteresis)
       → Health State: NORMAL / WARNING / CRITICAL
       → LED + Buzzer Alert
       → JSON over UART → Python Dashboard
       → Health Score + Trend → Graph + Event Log
```

## Signal Flow

| Stage | Module | Function |
|---|---|---|
| Acquisition | sensors.c | ADC read → g-force / °C |
| Filtering | filter.c | Moving average (10-sample window) |
| Detection | anomaly.c | Hysteresis FSM → health state |
| Alert | alert.c | GPIO LEDs + LEDC buzzer |
| Communication | comm.c | JSON over UART2 → PC |
| Visualization | dashboard.py | Real-time graph + analytics |
