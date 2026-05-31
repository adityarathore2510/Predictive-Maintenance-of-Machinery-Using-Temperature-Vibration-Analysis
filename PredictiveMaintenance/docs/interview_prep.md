# Placement Interview Preparation
## Predictive Machine Failure Monitoring System — ESP32

---

### Section 1: Core Project Questions

---

**Q1: Explain your project in 2 minutes.**

> "I built an industrial predictive maintenance system using an ESP32 microcontroller.
> The system reads vibration from an ADXL335 accelerometer and temperature from an LM35 sensor
> through the ESP32's 12-bit ADC. The raw data goes through a moving average filter to remove noise,
> then through a threshold-based anomaly detection module with hysteresis to classify machine health
> as NORMAL, WARNING, or CRITICAL.
>
> The firmware is built on FreeRTOS with five tasks running concurrently — sensor acquisition, filtering,
> anomaly detection, alert management, and serial communication. The alerts drive GPIO LEDs and a PWM
> buzzer directly. Telemetry is sent as JSON packets over UART to a Python dashboard that plots live
> graphs and calculates a health score.
>
> The entire architecture is modular — each subsystem is in its own source file with a clean API,
> making it easy to swap sensors, change thresholds, or add MQTT communication."

---

**Q2: Why did you choose ESP32 over STM32 or Arduino?**

> - ESP32's Xtensa LX6 dual-core architecture allows real-time sensor tasks and communication tasks to
>   run on separate cores — reducing interference.
> - Built-in 12-bit ADC with multiple channels eliminates the need for an external ADC IC.
> - Native FreeRTOS support in ESP-IDF is production-grade, unlike Arduino's single-threaded model.
> - Built-in WiFi/BLE means MQTT upgrade is possible without hardware changes.
> - STM32 is more capable for precision control but overkill for this monitoring application, and
>   requires more complex toolchain setup.

---

**Q3: What is a moving average filter and why did you choose it?**

> A moving average filter maintains a sliding window of the last N samples and outputs their mean.
> I implemented it using a circular buffer with a running sum — so each update is O(1), not O(N).
>
> **Why:** Raw ADC readings from vibration sensors contain high-frequency noise from electrical
> interference and sensor quantization. The moving average smooths this while preserving the
> slowly evolving trend that indicates machine degradation. It's computationally trivial on a
> microcontroller and doesn't add latency in a way that matters at our detection timescale.
>
> **Trade-off:** It introduces a lag of (window_size × sample_period) / 2 = 250ms at our settings.
> This is acceptable for mechanical fault detection, which evolves over seconds to days.

---

**Q4: Explain hysteresis in your anomaly detection.**

> Without hysteresis, if vibration = 1.01g → WARNING, then 0.99g → NORMAL. This causes
> the alert to toggle hundreds of times per second near the threshold — useless in practice.
>
> With hysteresis: Enter WARNING when vibration > 1.0g, but only exit WARNING when vibration < 0.9g.
> The 0.1g band prevents oscillation. This is a standard technique used in industrial PID controllers,
> Schmitt triggers, and thermostat designs.
>
> Implementation: I used a boolean flag per sensor channel inside an anomaly_state_t structure,
> toggled only when the appropriate crossing direction is detected.

---

**Q5: How does your FreeRTOS task architecture work?**

> Five tasks with a strict priority hierarchy:
>
> 1. **task_sensor** (Priority 5, highest) — 20Hz ADC reads, pushes to Queue1
> 2. **task_filter**  (Priority 4) — consumes Queue1, applies moving average, pushes to Queue2
> 3. **task_anomaly** (Priority 3) — consumes Queue2, runs hysteresis FSM, updates shared result
> 4. **task_alert**   (Priority 3) — reads shared result every 100ms, controls LEDs and buzzer
> 5. **task_comm**    (Priority 2, lowest) — reads shared result every 500ms, formats and sends JSON
>
> Tasks 1–3 are pinned to Core 1 for real-time determinism. Tasks 4–5 on Core 0.
> Inter-task data transfer uses FreeRTOS queues (Thread-safe FIFO).
> The shared result struct is protected by a mutex semaphore.

---

**Q6: Why use queues instead of global variables between tasks?**

> Global variables shared between tasks create race conditions. For example, if task_filter
> is writing a float while task_anomaly is reading it, a context switch mid-write produces a
> corrupted value. This is a data integrity bug that's extremely hard to reproduce and debug.
>
> FreeRTOS queues solve this by providing an atomic, thread-safe FIFO. The sender copies data
> INTO the queue; the receiver copies data OUT. There's no shared memory reference.
>
> For the shared result (written by anomaly, read by two tasks), I use a mutex semaphore to
> guarantee atomic read-write access — a standard RTOS synchronization primitive.

---

**Q7: What is predictive maintenance and how does your system implement it?**

> Predictive maintenance (PdM) predicts equipment failure BEFORE it occurs, allowing maintenance
> to be scheduled during planned downtime rather than during unexpected breakdowns.
>
> Reactive maintenance: Fix after failure — very expensive.
> Preventive maintenance: Fixed schedule — wastes resources.
> Predictive maintenance: Condition-based — optimal cost vs reliability.
>
> My system implements the foundational layer of PdM:
> 1. Vibration trending — a rising slope in filtered vibration over time indicates bearing wear
> 2. Temperature trending — gradually rising temperature indicates cooling system degradation
> 3. Alert count tracking — a machine accumulating alerts at increasing frequency is near failure
>
> The trend slope (linear regression over last 30 samples) gives early warning even before the
> threshold is crossed — this is the predictive element.

---

**Q8: How did you convert ADC values to engineering units?**

> ESP32 ADC: 12-bit resolution (0–4095 counts), Vref = 3.3V.
>
> **Voltage:** V_mv = (raw / 4095) × 3300 mV
>
> **LM35:** Sensitivity = 10mV/°C, so Temp_C = V_mv / 10
> At 25°C: Vout = 250mV → raw ≈ 310 counts
>
> **ADXL335:** Zero-g bias = Vcc/2 = 1650mV, Sensitivity ≈ 300mV/g
> g = (V_mv − 1650) / 300
> I take the absolute value (magnitude) since I care about vibration amplitude, not direction.

---

### Section 2: Embedded Systems Fundamentals

---

**Q9: What is FreeRTOS and why is it used in embedded systems?**

> FreeRTOS is a real-time operating system kernel designed for microcontrollers.
> It provides: task scheduling (priority-based preemptive), inter-task communication
> (queues, semaphores, mutexes), and deterministic timing guarantees.
>
> In bare-metal firmware, a single while(1) loop cannot serve multiple subsystems
> simultaneously without complex state machines. FreeRTOS allows each subsystem to be
> expressed as an independent task with well-defined responsibilities, making the code
> modular, testable, and maintainable.

---

**Q10: What is a circular buffer and what are its advantages?**

> A circular buffer (ring buffer) is a fixed-size array used as a continuous loop.
> A head pointer tracks the next write position; it wraps around to 0 when it reaches the end.
>
> Advantages:
> - O(1) insert and oldest-element eviction — no shifting required
> - Fixed memory footprint — safe for embedded systems with no heap
> - Cache-friendly — sequential memory access pattern
>
> In my filter: I maintain a running sum and subtract the outgoing element, so the average
> update is O(1) regardless of window size.

---

**Q11: What is LEDC and why did you use it for the buzzer?**

> LEDC (LED Control) is ESP32's PWM peripheral, originally designed for LED dimming but
> usable for any PWM output.
>
> A passive buzzer requires an AC signal (PWM at audible frequency) to produce sound.
> Using LEDC instead of GPIO toggling:
> - Hardware-driven — no CPU cycles consumed during sound generation
> - Frequency and duty cycle configurable via registers
> - RTOS-safe — won't miss pulses due to task preemption
>
> I configured it at 2kHz (audible, not harsh) with 50% duty cycle for maximum volume.
> Duty = 0 → silent; Duty = 128 (of 255) → sound.

---

**Q12: What are the ESP32 ADC limitations you encountered?**

> 1. **Nonlinearity:** ESP32's SAR ADC has nonlinearity near 0V and Vcc. I avoided this
>    by using 11dB attenuation (0–3.9V range) and ensuring sensor output stays in the
>    linear midrange (0.5V–2.5V).
>
> 2. **ADC1 vs ADC2:** ADC2 cannot be used when WiFi is active. I exclusively used ADC1
>    (GPIO32–GPIO39) to avoid conflicts even when MQTT/WiFi is enabled.
>
> 3. **Sampling noise:** Multiple back-to-back reads can introduce noise. The moving average
>    filter handles this at the application layer.
>
> 4. **Calibration:** For production accuracy, `esp_adc_cal` provides Vref calibration.
>    I documented this in code comments as a production upgrade path.

---

### Section 3: Technical Design Questions

---

**Q13: How would you add MQTT to this system?**

> The comm.c module already has the MQTT topic constants defined in config.h.
> Adding MQTT requires:
> 1. Initialize WiFi (esp_wifi_init, connect to AP)
> 2. Initialize MQTT client (esp_mqtt_client_init with broker URI)
> 3. In task_comm, call esp_mqtt_client_publish() with the JSON string
> 4. Fallback to UART if MQTT disconnects
>
> This is a standard ESP-IDF pattern. The modular design means comm.c is the ONLY file
> that changes — sensor, filter, anomaly, and alert modules are untouched.

---

**Q14: How would you scale this to monitor 50 machines?**

> Three changes:
> 1. Each ESP32 gets a unique DEVICE_ID in config.h
> 2. Replace UART output with MQTT → messages go to broker
> 3. MQTT topics: "plant/{machine_id}/sensors" → broker routes to dashboard
>
> Dashboard subscribes to "plant/+/sensors" (wildcard) and displays all machines.
> The broker (e.g., Mosquitto) can handle thousands of simultaneous clients.
> This is the standard Industry 4.0 IoT architecture.

---

**Q15: What is the sampling rate of your system and why did you choose 20Hz?**

> Sampling rate: 50ms interval → 20Hz
>
> **Nyquist consideration:** For bearing fault detection, the characteristic frequencies
> are typically 5–500Hz (depending on motor RPM and bearing geometry). 20Hz only captures
> low-frequency mechanical trends, which is sufficient for threshold-based PdM.
>
> For full spectral analysis (FFT-based bearing diagnostics), a 5kHz+ sampling rate is needed.
> That's a well-defined future improvement — I documented it in the README.

---

**Q16: Explain your JSON communication format.**

> I chose newline-delimited JSON for three reasons:
> 1. Self-describing — no fixed-length parsing or magic bytes needed
> 2. Python json.loads() parses it natively in one line
> 3. Human-readable — easy to debug with a serial terminal
>
> Trade-off: JSON is verbose (~100 bytes/packet). For bandwidth-constrained systems,
> CBOR (Concise Binary Object Representation) or custom binary framing would be better.
> At 115200 baud, 100 bytes per 500ms is trivially within capacity.

---

**Q17: How does your health score work?**

> Health Score = 100% × (1 − max(vib_severity, temp_severity))
>
> Severity is a normalized 0.0–1.0 value where:
> - 0.0 = perfectly healthy
> - 1.0 = at or above critical threshold
>
> max() takes the worst sensor reading.
> At NORMAL: score ≈ 90–100%
> At WARNING: score ≈ 50–90%
> At CRITICAL: score ≈ 0–50%
>
> This gives a single, intuitive metric for recruiters and operators to understand
> machine condition without needing to interpret raw sensor values.

---

### Section 4: Resume & HR Questions

---

**Q18: What was the most challenging part of this project?**

> The hysteresis implementation in the anomaly detector was the most thought-provoking.
> Initially, the alert system was flickering at state boundaries. I analyzed the root cause:
> the detection threshold was a simple comparator with no memory.
>
> Adding a stateful FSM with a hysteresis band (entering and exiting at different thresholds)
> solved the problem completely. This is a technique used in industrial PLCs and Schmitt triggers,
> so implementing it in software on a microcontroller was a satisfying application of the concept.

---

**Q19: What would you improve if you had more time?**

> Three improvements in order of impact:
> 1. **FFT vibration analysis** — compute bearing fault frequencies from vibration spectrum.
>    Identify specific fault modes (outer race, inner race, ball defect) by their characteristic
>    frequencies. This requires 5kHz+ sampling and a 512-point FFT.
> 2. **MQTT + remote dashboard** — replace USB-serial with WiFi MQTT, enabling remote monitoring.
> 3. **SD card data logging** — store historical data for long-term trend analysis, enabling
>    maintenance to correlate sensor history with actual failure events.

---

**Q20: Is this project Industry 4.0 relevant?**

> Yes. Industry 4.0 is characterized by interconnected, data-driven manufacturing.
> This project implements the sensing and edge-computing layer:
> - Edge analytics (anomaly detection runs on the microcontroller itself, not in the cloud)
> - Real-time monitoring (500ms data latency from sensor to dashboard)
> - Condition-based maintenance (replacing schedule-based preventive maintenance)
>
> The MQTT upgrade path connects it to the broader Industry 4.0 stack:
> Edge Device → MQTT Broker → Time-Series DB (InfluxDB) → Grafana Dashboard
>
> This is exactly the architecture used by companies like Siemens, Bosch, and Honeywell
> in their industrial IoT product lines.

---

## Key Technical Terms to Know Cold

| Term | One-Sentence Explanation |
|------|--------------------------|
| FreeRTOS | Real-time OS kernel providing preemptive task scheduling on microcontrollers |
| RTOS Priority Inversion | When a high-priority task is blocked by a low-priority task holding a shared resource |
| Circular Buffer | Fixed-size array used as a ring for O(1) FIFO operations |
| Hysteresis | Dual-threshold state transition to prevent oscillation near decision boundary |
| Moving Average Filter | Smoothing filter that outputs the mean of the last N samples |
| Nyquist Theorem | Sampling rate must be > 2× maximum signal frequency to avoid aliasing |
| ADC Resolution | Bit-width of ADC conversion: 12-bit = 4096 discrete voltage levels |
| LEDC | ESP32's hardware PWM peripheral used for buzzer and LED control |
| MQTT | Lightweight publish-subscribe protocol for IoT device communication |
| Predictive Maintenance | Condition-based maintenance triggered by sensor data, not a fixed schedule |
| Industry 4.0 | Fourth industrial revolution: IoT, data analytics, and automation in manufacturing |
