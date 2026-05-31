# Step-by-Step Deployment Guide

## Prerequisites

### Software Requirements

| Tool | Version | Purpose |
|------|---------|---------|
| ESP-IDF | v5.1+ | ESP32 firmware build system |
| Python | 3.10+ | Dashboard runtime |
| Git | Any | Version control |
| VS Code + ESP-IDF Extension | Optional | IDE support |

### Hardware Requirements (optional — demo mode works without)
- ESP32 DevKit V1
- ADXL335 module
- LM35 sensor
- LEDs × 3 (green, yellow, red)
- 220Ω resistors × 3
- 100Ω resistor × 1
- Passive buzzer × 1
- CP2102 USB-Serial adapter
- Jumper wires + breadboard

---

## Part 1: Dashboard (No Hardware Required)

### Step 1: Install Python Dependencies

```bash
cd PredictiveMaintenance/dashboard
pip install -r requirements.txt
```

### Step 2: Run Dashboard in Demo Mode

```bash
python dashboard.py --demo
```

The dashboard will open and automatically cycle through:
- NORMAL state (green, vibration < 1.0g)
- WARNING state (yellow, vibration 1.0–2.0g)
- CRITICAL state (red, vibration > 2.0g)
- Recovery back to NORMAL

This demonstrates the complete system behavior without any hardware.

---

## Part 2: Firmware Setup (Hardware Required)

### Step 1: Install ESP-IDF

**Windows (recommended):**
1. Download ESP-IDF Tools Installer from: https://docs.espressif.com/projects/esp-idf/
2. Run installer — select ESP-IDF v5.1 or later
3. Open "ESP-IDF PowerShell" from Start Menu

**Linux/Mac:**
```bash
mkdir -p ~/esp && cd ~/esp
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32
. ./export.sh
```

### Step 2: Build Firmware

```bash
cd PredictiveMaintenance/firmware
idf.py set-target esp32
idf.py build
```

Expected output: `Project build complete. To flash, run: idf.py flash`

### Step 3: Flash to ESP32

```bash
# Connect ESP32 via USB
idf.py -p COM3 flash    # Windows (replace COM3 with actual port)
idf.py -p /dev/ttyUSB0 flash    # Linux
```

### Step 4: Verify Serial Output

```bash
idf.py -p COM3 monitor
```

You should see:
```
I (1234) MAIN: === Predictive Maintenance System v1.0.0 ===
I (1235) MAIN: Device ID: ESP32-PM-01
I (1240) SENSORS: Sensors initialized. Vib: GPIO36, Temp: GPIO39
I (1241) ALERT: Alert system initialized
I (1242) COMM: UART2 initialized at 115200 baud (TX: GPIO17)
I (1250) MAIN: All tasks created. System running.
```

After 500ms, JSON packets appear:
```
{"id":"ESP32-PM-01","ts":1,"vib":0.0124,"temp":26.50,"vs":0.006,"ts_s":0.331,"health":"NORMAL","alerts":0}
```

### Step 5: Connect Dashboard to Hardware

```bash
cd dashboard
python dashboard.py --port COM3    # Windows
python dashboard.py --port /dev/ttyUSB0    # Linux
```

---

## Part 3: Hardware Assembly

Follow `simulation/circuit_diagram.md` for detailed connections.

Quick reference:
```
ADXL335  ZOUT → GPIO36
LM35     VOUT → GPIO39
Green LED     → 220Ω → GPIO26
Yellow LED    → 220Ω → GPIO27
Red LED       → 220Ω → GPIO14
Buzzer (+)    → 100Ω → GPIO25
CP2102   TXD → GPIO16 (ESP32 RX)
CP2102   RXD → GPIO17 (ESP32 TX)
All GNDs connected to common GND
```

---

## Debugging Checklist

### Firmware Issues

- [ ] `idf.py build` fails → Check ESP-IDF version (v5.1+ required)
- [ ] `idf.py flash` fails → Press BOOT button on ESP32 during flash
- [ ] No serial output → Check baud rate is 115200 in monitor
- [ ] ADC reads all zeros → Verify GPIO36/39 are used (ADC1 only)
- [ ] ADC reads all 4095 → Input voltage exceeds 3.3V — check sensor wiring

### Dashboard Issues

- [ ] `ModuleNotFoundError: serial` → `pip install pyserial`
- [ ] `ModuleNotFoundError: matplotlib` → `pip install matplotlib`
- [ ] Port not found → Check Device Manager for COM port number
- [ ] No data in graphs → Run `--demo` mode first to verify dashboard works
- [ ] Graphs frozen → Check serial reader thread with `reader.connected`

### Hardware Issues

- [ ] LEDs not lighting → Check LED polarity (anode to GPIO, cathode to GND via resistor)
- [ ] Buzzer silent → Verify GPIO25 is connected to (+) terminal of buzzer
- [ ] Vibration always 0g → ADXL335 ZOUT not connected or sensor not powered
- [ ] Temperature shows 0°C → LM35 not powered (check 3.3V connection)
- [ ] Temperature shows 330°C → ADC reading Vcc directly — check LM35 wiring

---

## Testing Strategy

### Unit Test: Filter Module (PC, no hardware)

```c
// test_filter.c — can compile on any platform
#include "filter.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    moving_avg_filter_t f;
    filter_init(&f);

    // Test: average of 1.0 added 10 times = 1.0
    for (int i = 0; i < 10; i++) filter_update(&f, 1.0f);
    assert(fabsf(filter_get_average(&f) - 1.0f) < 0.001f);

    // Test: new high value doesn't immediately dominate
    float avg = filter_update(&f, 10.0f);
    assert(avg < 2.0f);    // Old samples still dominate

    printf("Filter tests PASSED\n");
    return 0;
}
```

### Integration Test: Threshold Validation

With hardware connected:
1. Simulate 0g vibration (ADC ~= midpoint) → LED must be GREEN
2. Adjust to simulate 1.5g (increase analog input) → LED must be YELLOW within 500ms
3. Adjust to simulate 2.5g → LED must be RED + buzzer fast pulse
4. Return to 0g → GREEN after hysteresis clears

### Dashboard Validation

1. Run `python dashboard.py --demo`
2. Verify graphs update every 500ms
3. Verify health indicator changes color during WARNING/CRITICAL phases
4. Verify event log shows state transitions
5. Verify health score drops below 50% during CRITICAL phase
