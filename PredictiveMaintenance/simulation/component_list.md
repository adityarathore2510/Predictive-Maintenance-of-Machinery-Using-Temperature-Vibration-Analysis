# Component List — Bill of Materials

## Active Components

| Component | Model | Quantity | Unit Cost (INR) | Total | Justification |
|-----------|-------|----------|-----------------|-------|---------------|
| Microcontroller | ESP32 DevKit V1 | 1 | ₹350 | ₹350 | Dual-core, built-in ADC, WiFi-ready |
| Accelerometer | ADXL335 Module | 1 | ₹180 | ₹180 | 3-axis, analog output, industrial range |
| Temperature Sensor | LM35 (TO-92) | 1 | ₹25 | ₹25 | Linear 10mV/°C, no digital protocol overhead |
| USB-Serial Bridge | CP2102 / CH340G | 1 | ₹70 | ₹70 | Dashboard communication |

## Passive / Discrete Components

| Component | Value | Quantity | Cost | Total |
|-----------|-------|----------|------|-------|
| LED (Green) | 5mm, 2.0V | 1 | ₹3 | ₹3 |
| LED (Yellow) | 5mm, 2.1V | 1 | ₹3 | ₹3 |
| LED (Red) | 5mm, 1.8V | 1 | ₹3 | ₹3 |
| Resistor | 220Ω ¼W | 3 | ₹1 | ₹3 |
| Resistor | 100Ω ¼W | 1 | ₹1 | ₹1 |
| Passive Buzzer | 5V, 2kHz | 1 | ₹20 | ₹20 |
| Ceramic Capacitor | 100nF | 2 | ₹2 | ₹4 |

## Mechanical / Prototyping

| Component | Quantity | Cost |
|-----------|----------|------|
| Breadboard (full size) | 1 | ₹80 |
| Jumper Wires (M-M set) | 1 set | ₹50 |
| USB Cable (Micro-B) | 1 | ₹40 |

## Total BOM Cost: ≈ ₹836

---

## Component Selection Justification

### Why ADXL335 over MPU6050 (digital)?

| Aspect | ADXL335 (analog) | MPU6050 (I2C) |
|---|---|---|
| Interface | Direct ADC | I2C protocol overhead |
| Noise | Higher (ADC dependent) | Lower (integrated DSP) |
| Simplicity | Read ADC pin directly | Library + I2C driver |
| Firmware complexity | Minimal | Moderate |
| Cost | ₹180 | ₹200 |
| Industrial relevance | High (analog sensors common) | Medium |

**Decision:** ADXL335 was chosen because it demonstrates ADC interfacing skills,
which are more fundamental to embedded systems than using a pre-packaged I2C sensor library.
It also keeps the firmware dependency-free at the sensor layer.

### Why LM35 over DHT11/DS18B20?

- LM35 is a pure analog sensor — directly demonstrates ADC-to-engineering-unit conversion
- DHT11: digital one-wire protocol, requires timing-sensitive bit-banging
- DS18B20: digital one-wire with CRC — adds library dependency, hides the ADC work
- LM35: datasheet formula is trivially explainable in interviews

### Why passive buzzer over active buzzer?

Active buzzers contain an internal oscillator — they make noise when DC power is applied.
A passive buzzer requires an AC signal (PWM). Using the passive buzzer:
- Demonstrates LEDC/PWM peripheral usage (interview-worthy)
- Allows frequency control (warning vs critical tones can differ)
- Better understanding of how PWM generates audible sound
