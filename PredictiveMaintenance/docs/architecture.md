# System Architecture — Technical Deep Dive

## Design Philosophy

Every architectural decision in this project follows three principles:
1. **Separation of Concerns** — each module does one thing well
2. **Determinism** — critical timing paths have guaranteed execution priority
3. **Observability** — every subsystem emits structured data for debugging

---

## Firmware Architecture

### Module Dependency Graph

```
main.c (Orchestrator)
  ├── config.h         (Pure constants — no code)
  ├── sensors.c        (depends on: config.h, esp_adc)
  ├── filter.c         (depends on: config.h only — portable)
  ├── anomaly.c        (depends on: config.h, filter.h types)
  ├── alert.c          (depends on: config.h, driver/gpio, driver/ledc)
  └── comm.c           (depends on: config.h, anomaly.h, driver/uart)
```

**Key property:** filter.c has ZERO ESP-IDF dependencies.
It can be compiled and unit-tested on any platform (PC, STM32, etc.)
This is the correct embedded architecture principle.

---

## FreeRTOS Task Analysis

### Priority Inversion Prevention

The semaphore protecting `g_current_result` is a **mutex** (not a binary semaphore).
FreeRTOS mutexes implement **Priority Inheritance Protocol** — if a low-priority task (comm)
holds the mutex while a high-priority task (anomaly) is waiting, the OS temporarily boosts
the low-priority task's priority to prevent starvation. This prevents priority inversion.

### Core Affinity Strategy

```
Core 1 (Real-Time Core):
  task_sensor  (priority 5)
  task_filter  (priority 4)
  task_anomaly (priority 3)
  → These form the deterministic sampling pipeline
  → No WiFi stack interference

Core 0 (System Core):
  task_alert   (priority 3)
  task_comm    (priority 2)
  WiFi/TCP stack (when enabled)
  → Alert and comm latency is acceptable (100–500ms)
```

### Queue Sizing Rationale

Queue depth = 4 packets:
- At 20Hz sensor rate + 20ms max task blocking = max 1 sample queued
- Depth of 4 absorbs burst latency without excessive memory use
- Each sensor_data_t ≈ 16 bytes → 4 × 16 = 64 bytes total per queue

---

## Signal Processing Pipeline

### Noise Characteristics

ADXL335 has a noise density of ~150µg/√Hz. At 20Hz bandwidth:
- RMS noise ≈ 150µg × √20 ≈ 670µg ≈ 0.00067g
- This is below our minimum resolution (1 ADC count ≈ 0.003g at 3.3V/4095 counts)
- Dominant noise source: ADC quantization + power supply ripple

### Filter Trade-offs

| Filter Type | Complexity | Latency | Noise Rejection |
|---|---|---|---|
| No filter (raw) | Trivial | 0ms | None |
| Moving average N=5 | O(1) | 125ms | Moderate |
| **Moving average N=10** | O(1) | 250ms | Good |
| Moving average N=20 | O(1) | 500ms | Very Good |
| IIR (1st order low-pass) | O(1) | Depends on α | Good |
| Kalman filter | O(N²) | Low | Excellent |

**Selected: N=10 moving average** — best balance of noise rejection, latency, and implementation simplicity for RTOS use.

---

## Anomaly Detection State Machine

```
State Diagram (per sensor channel):

           vib > WARNING_G
 NORMAL ─────────────────────► WARNING
   ▲                              │
   │  vib < (WARNING_G - HYST)    │  vib >= CRITICAL_G
   └──────────────────────────────┤
                                  │
                              CRITICAL ◄────────────────
                                  │    vib >= CRITICAL_G (from any state)
                                  │
                       vib < (CRITICAL_G - HYST)
                                  │
                                  └────────────────► WARNING
```

**Combined Health State:**
```
if (vib_state == CRITICAL || temp_state == CRITICAL) → HEALTH_CRITICAL
else if (vib_state == WARNING || temp_state == WARNING) → HEALTH_WARNING
else → HEALTH_NORMAL
```

---

## Communication Protocol Design

### Why UART2 (not UART0)?

UART0 is shared with the ESP-IDF bootloader and debug console.
Using UART0 for application data causes mixed output (debug logs + JSON).
UART2 (GPIO16/17) is exclusively allocated to application serial communication.

### JSON vs Binary Protocol

| Aspect | JSON | Binary (e.g., COBS) |
|---|---|---|
| Parsing | trivial (json.loads) | requires framing library |
| Debugging | human-readable | requires decoder |
| Bandwidth | ~100 bytes/msg | ~20 bytes/msg |
| Error recovery | line-boundary | requires sync word |

At 115200 baud, 100 bytes per 500ms = 200 bytes/second — well under 14,400 bytes/second capacity.
JSON is the correct choice for this application.

---

## Dashboard Architecture

### Threading Model

```
Main Thread (Tkinter event loop):
  → All UI updates (labels, graphs)
  → Calls reader.get_latest() — non-blocking

Background Thread (SerialReader):
  → Blocking readline() on serial port
  → JSON parse
  → Append to thread-safe deque
  → NO UI calls — never touches Tkinter from background thread
```

**Why this matters:** Tkinter is NOT thread-safe. All UI operations must happen in the main thread.
The background thread only writes to a thread-safe deque (protected by threading.Lock).
The main thread reads from the deque via root.after() callbacks — this is the correct pattern.

### Health Score Mathematics

```
vib_severity  = clamp(vib_g / CRITICAL_VIB_G, 0.0, 1.0)
temp_severity = clamp(temp_c / CRITICAL_TEMP_C, 0.0, 1.0)
composite     = max(vib_severity, temp_severity)
health_score  = 100 × (1 - composite)
```

This is a linear model. In production, it would be replaced by a weighted combination
with historical calibration data. But for a monitoring dashboard, it's accurate, explainable,
and directly connected to physical measurements.

---

## Scalability Roadmap

### Phase 1 (Current): Single Node, UART
- ESP32 → USB-Serial → Python dashboard on same PC

### Phase 2: Single Node, WiFi MQTT
- ESP32 → WiFi → MQTT Broker → Python dashboard (any PC on network)
- Code change: Replace comm.c UART calls with esp_mqtt_client_publish()

### Phase 3: Multi-Node Industrial
- N ESP32 nodes → WiFi → MQTT Broker → InfluxDB → Grafana
- Each node: unique DEVICE_ID, same firmware binary
- Dashboard: subscribes to wildcard topic, displays all machines

### Phase 4: Production Hardening
- RS-485 physical layer for noise immunity over long runs
- SD card local data logging (offline resilience)
- OTA update via ESP-IDF OTA component
- Hardware watchdog (WDT) for autonomous recovery
