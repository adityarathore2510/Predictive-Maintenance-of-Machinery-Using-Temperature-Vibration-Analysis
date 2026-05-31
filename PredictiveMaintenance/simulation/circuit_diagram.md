# Circuit Diagram & Hardware Architecture

## Overview

This document describes the complete circuit architecture for the
Predictive Machine Failure Monitoring System.

---

## ESP32 Pin Assignment Table

| GPIO | Function | Component | Notes |
|------|----------|-----------|-------|
| GPIO36 | ADC1_CH0 Input | ADXL335 Z-axis OUT | Input-only pin, no pull |
| GPIO39 | ADC1_CH3 Input | LM35 OUT | Input-only pin, no pull |
| GPIO25 | LEDC PWM Output | Passive Buzzer | Via 100Ω current limit |
| GPIO26 | Digital Output | Green LED | Via 220Ω resistor to GND |
| GPIO27 | Digital Output | Yellow LED | Via 220Ω resistor to GND |
| GPIO14 | Digital Output | Red LED | Via 220Ω resistor to GND |
| GPIO17 | UART2 TX | USB-Serial Adapter (TX→RX) | CP2102 or CH340 |
| GPIO16 | UART2 RX | USB-Serial Adapter (RX→TX) | Optional — for future Rx |
| 3.3V | VCC Rail | ADXL335 Vs, LM35 Vs | Decoupled per sensor |
| GND | Common Ground | All components | Star topology preferred |

---

## Component Connections

### 1. ADXL335 Accelerometer (Vibration Sensor)

```
ADXL335 Module            ESP32 DevKit V1
─────────────            ─────────────────
VS (3.3V)       ────────  3.3V
GND             ────────  GND
ZOUT            ────────  GPIO36 (ADC1_CH0)
(XOUT, YOUT)    [NC]      Not connected (Z-axis only for vertical vibration)

Decoupling: 100nF ceramic capacitor between VS and GND (as close to module as possible)
```

**Why Z-axis only?**
In industrial motor monitoring, bearing failures primarily manifest as
vertical vibration. Using a single axis simplifies ADC channel usage
and is sufficient for threshold-based detection at this stage.

---

### 2. LM35 Temperature Sensor

```
LM35 (TO-92 Package)       ESP32 DevKit V1
────────────────────       ─────────────────
Pin 1: +VS (5V or 3.3V) ─  3.3V  (operating range: 4–30V, but 3.3V works)
Pin 2: VOUT             ─  GPIO39 (ADC1_CH3)
Pin 3: GND              ─  GND

NOTE: At 3.3V supply, LM35 output is accurate 2°C–100°C range.
      At 25°C: Vout = 250mV → ADC raw ≈ 310 (at 3.3V Vref)
```

**Why LM35 over DS18B20?**
LM35 outputs an analog voltage directly proportional to temperature.
No digital protocol overhead, no timing library needed — clean ADC
read compatible with any microcontroller.

---

### 3. Alert System

```
Alert LEDs:
  ESP32 GPIO26 ──[220Ω]──► Green LED Anode ──► Cathode → GND
  ESP32 GPIO27 ──[220Ω]──► Yellow LED Anode ──► Cathode → GND
  ESP32 GPIO14 ──[220Ω]──► Red LED Anode ──► Cathode → GND

  220Ω limits current to ≈ (3.3V - 2.0V) / 220Ω ≈ 5.9mA
  Well within ESP32 GPIO source limit of 12mA.

Passive Buzzer:
  ESP32 GPIO25 ──[100Ω]──► Buzzer (+) ──► Buzzer (-) → GND
  100Ω protects against short during PWM switching.
  Buzzer operating: 5V preferred, but functional at 3.3V.
```

---

### 4. USB-Serial Bridge (UART2 → PC Dashboard)

```
ESP32             CP2102 / CH340G Module      PC
──────            ─────────────────────       ──
GPIO17 (TX) ────  RXD
GPIO16 (RX) ────  TXD
GND         ────  GND
                  USB → USB → Python COM Port
```

---

## Power Requirements

| Component | Vcc | Icc (typical) |
|-----------|-----|----------------|
| ESP32 DevKit (active WiFi off) | 3.3V (onboard reg from 5V USB) | ~80mA |
| ADXL335 | 3.3V | 350µA |
| LM35 | 3.3V | 60µA |
| 3× LEDs | 3.3V | 6mA × 3 = 18mA |
| Buzzer | 3.3V | ~20mA |
| **TOTAL** | **5V USB** | **~120mA** |

A standard USB 2.0 port (500mA) is sufficient.
No external power supply required for bench testing.

---

## Industrial Scalability Note

In a real deployment, this circuit would be enclosed in an IP67-rated
enclosure, powered from a 24V industrial rail via a DC-DC converter
to 5V/3.3V. The UART bridge would be replaced by RS-485 transceivers
(e.g., MAX485) for noise immunity over long cable runs.

---

## Proteus Simulation Components

The following components are available in Proteus 8 and can be used
directly to simulate this circuit. See `proteus_guide.md` for step-by-step instructions.

| Real Component | Proteus Equivalent | Library |
|---|---|---|
| ESP32 DevKit | ESP32-WROOM-32 or MCU + ADC model | Third-party ESP32 library |
| ADXL335 | Potentiometer (simulates analog Vout) | Basic Parts |
| LM35 | Analog Voltage Source / Potentiometer | Basic Parts |
| Green/Yellow/Red LED | LED-GREEN, LED-YELLOW, LED-RED | Active Components |
| Passive Buzzer | SOUNDER or BUZZER | Active Components |
| 220Ω Resistors | RES | Passive Components |
| CP2102 | Virtual Serial Port Terminal | Instruments |
