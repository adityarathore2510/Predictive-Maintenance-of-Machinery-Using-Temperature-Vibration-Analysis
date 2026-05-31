# Proteus Simulation Setup Guide

## Overview

Since ESP32 native Proteus models are limited, this guide describes a
practical simulation strategy using the available tools — including
a hardware-free demonstration using the Python dashboard's built-in
demo mode, which generates realistic sensor data without any hardware.

---

## Approach 1 — Python Demo Mode (Recommended for Initial Testing)

The Python dashboard has a built-in `--demo` flag that generates
realistic simulated sensor data including:
- Realistic vibration oscillations with noise
- Slow temperature rise simulation
- Automatic cycling through NORMAL → WARNING → CRITICAL → NORMAL
- Alert state transitions

```bash
# Install dependencies
pip install -r requirements.txt

# Run in demo mode
python dashboard.py --demo
```

This is sufficient to demonstrate the full system behavior to a
recruiter and validate the dashboard logic without any hardware.

---

## Approach 2 — Proteus Simulation with Virtual Terminal

### Step 1: Create New Project
1. Open Proteus 8 Professional
2. File → New Project
3. Set working directory to `PredictiveMaintenance/simulation/`
4. Select "ISIS Schematic Capture"

### Step 2: Place Core Components

**Microcontroller (ESP32 approximation):**
- If ESP32 model is available: search "ESP32"
- Alternative: Use Arduino MEGA (ATMEGA2560) as functional equivalent
  for simulation purposes. The firmware logic is identical, only pin
  numbers differ.

**Sensor Simulation:**
```
For ADXL335:
  → Place: POT-HG (potentiometer)
  → Wire wiper output to ADC pin
  → Label: "VIB_SIM"
  → Turning potentiometer simulates 0–3.3V (0g to +2g vibration)

For LM35:
  → Place: VSOURCE (DC voltage source)
  → Set: 0V to 1.0V range
  → Wire to ADC pin
  → Label: "TEMP_SIM"
  → 250mV = 25°C, 650mV = 65°C (warning zone)
```

**Output Peripherals:**
```
LEDs:
  → Place: LED-GREEN (GPIO26), LED-YELLOW (GPIO27), LED-RED (GPIO14)
  → Each with 220Ω series resistor to GND

Buzzer:
  → Place: SOUNDER component
  → Connect to PWM output pin
  → 100Ω series resistor

Virtual Serial Terminal:
  → Instruments → VIRTUAL TERMINAL
  → Configure: 115200 baud, 8N1
  → Connect to UART TX pin
  → This displays JSON packets in real time during simulation
```

### Step 3: Configure Simulation
1. Simulation → Configure (or F7)
2. Set maximum timestep: 1ms (for accurate ADC simulation)
3. Set run time: 60 seconds (one full alert cycle)

### Step 4: Run Simulation
1. Press Play (F9)
2. Adjust potentiometer to simulate vibration increase
3. Observe LEDs changing state (Green → Yellow → Red)
4. Observe buzzer activation
5. Virtual Terminal shows JSON: `{"id":"ESP32-PM-01","vib":1.23,...}`

---

## Approach 3 — Real Hardware Testing

### Bill of Materials

| Component | Qty | Approximate Cost (INR) |
|-----------|-----|------------------------|
| ESP32 DevKit V1 | 1 | ₹350 |
| ADXL335 Module | 1 | ₹180 |
| LM35 (TO-92) | 1 | ₹25 |
| LED (Green, Yellow, Red) | 3 | ₹10 |
| Passive Buzzer | 1 | ₹20 |
| 220Ω Resistors | 3 | ₹5 |
| 100Ω Resistor | 1 | ₹2 |
| Jumper Wires | 20 | ₹50 |
| Breadboard | 1 | ₹80 |
| CP2102 USB-Serial | 1 | ₹70 |
| **Total** | | **≈ ₹800** |

### Hardware Setup Steps
1. Connect ADXL335 ZOUT → GPIO36
2. Connect LM35 VOUT → GPIO39
3. Connect LEDs through 220Ω resistors to GPIO26, 27, 14
4. Connect buzzer through 100Ω to GPIO25
5. Connect CP2102: ESP32 GPIO17 → CP2102 RXD, GND → GND
6. Flash firmware using ESP-IDF:
   ```
   idf.py build flash monitor
   ```
7. Connect USB-Serial to PC, open dashboard:
   ```
   python dashboard.py --port COM3
   ```
8. Mount ADXL335 on a vibrating surface (e.g., motor or fan)

---

## Validation Checklist

- [ ] Green LED ON at startup (NORMAL state)
- [ ] Moving potentiometer above 50% → Yellow LED + slow buzzer (WARNING)
- [ ] Moving potentiometer above 80% → Red LED + fast buzzer (CRITICAL)
- [ ] Dashboard shows corresponding health state change
- [ ] JSON packets visible in serial terminal / dashboard
- [ ] Alert count increments on each state change
- [ ] Trend indicator shows "RISING ↑" when vibration increases

---

## Signal Analysis for Validation

At 0g (machine idle):
- ADXL335 ZOUT ≈ 1650mV (zero-g bias)
- ADC raw ≈ 2047
- Displayed: 0.000g

At 1g (warning zone):
- ADXL335 ZOUT ≈ 1950mV
- ADC raw ≈ 2418
- Displayed: ≈ 1.000g → WARNING triggered

At 2g (critical zone):
- ADXL335 ZOUT ≈ 2250mV
- ADC raw ≈ 2790
- Displayed: ≈ 2.000g → CRITICAL triggered
