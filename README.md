# Real-Time Multisensor Room Monitoring System

## Project Overview
This project implements a concurrent, real-time room environmental monitoring system using an ESP32 microcontroller running FreeRTOS and native ESP-IDF APIs. Designed and simulated within the Wokwi simulation platform via PlatformIO, the system acquires real-time telemetry (temperature, humidity, and ambient light), evaluates room occupancy through passive infrared (PIR) sensing, visualizes diagnostics across dynamic pages on an SSD1306 OLED display, and sounds an acoustic alarm when room temperatures exceed safe thresholds.

## Features
- **Deterministic Periodic Telemetry**: Dual-core scheduled environmental sensing via DHT22 and LDR photoresistor using drift-free timing.
- **Multitasking Concurrency**: Modular FreeRTOS architecture utilizing 5 independent tasks with distinct preemptive priorities.
- **Inter-Task Communication (IPC)**: Thread-safe data transfer utilizing queues, binary state signaling via FreeRTOS event groups, and mutual exclusion for serial telemetry.
- **Interactive UI Navigation**: Quadrature rotary encoder driver enabling cyclic page-flipping on a 128x64 I2C OLED display.
- **Dynamic State Machine**: Automated power-saving states toggling between `ACTIVE` and `INACTIVE` based on motion timeouts.
- **Automated Verification**: Integrated Unity unit testing framework and zero-defect static code analysis via cppcheck.

## Learning Objectives
- Architect concurrent embedded firmware strictly under the Espressif IoT Development Framework (ESP-IDF) and FreeRTOS without Arduino wrappers.
- Prevent race conditions, task starvation, and timing drift using queues, mutexes, event flags, and `vTaskDelayUntil()`.
- Implement modular software architecture cleanly separating hardware-independent business logic from peripheral abstraction drivers.
- Exercise comprehensive software engineering practices including automated unit testing, static analysis, structured documentation, and Git version control.

## System Architecture
The application adheres to a four-tier embedded software architecture separating application threads from physical silicon:

![System Architecture](docs/images/system-architecture.png)

*Figure 1: Four-layer system architecture showing hardware decoupling, driver HAL, FreeRTOS kernel services, and concurrent application threads.*

## FreeRTOS Architecture
The system schedules five concurrent application tasks governed strictly by scheduling urgency and latency tolerance:

![FreeRTOS Architecture](docs/images/freertos-architecture.png)

*Figure 2: FreeRTOS task communication graph illustrating task responsibilities, communication pipes, and peripheral endpoints.*

## Hardware / Simulated Components
| Component | Interface / Protocol | Primary Responsibility |
| :--- | :--- | :--- |
| **ESP32-WROOM-32** | Tensilica Xtensa Dual-Core | Primary system-on-chip and real-time controller |
| **DHT22 (AM2302)** | Single-Bus Bit-Bang | Digital temperature and relative humidity sensing |
| **LDR Photoresistor** | ADC1 Single-Shot (0–3.3V) | Ambient light level monitoring (normalized 0–100%) |
| **HC-SR501 PIR** | Digital Input (GPIO) | Passive infrared motion and occupancy detection |
| **Rotary Encoder** | Quadrature Gray Code | User page navigation and state wake interrupts |
| **SSD1306 OLED** | I2C (Wire Bus, 0x3C) | Visual 128x64 graphical dashboard and page displays |
| **Active Buzzer** | Digital Push-Pull (GPIO) | Acoustic warning tone generator for thermal limits |

## Pin Configuration
| Peripheral | Pin Label | ESP32 GPIO | Mode / Configuration |
| :--- | :--- | :--- | :--- |
| **DHT22 Data** | SDA / OUT | `GPIO 4` | Open-drain pull-up / Bit-bang bidirectional |
| **LDR Photoresistor** | A0 | `GPIO 34` | Analog Input (ADC1_CHANNEL_6, 12-bit) |
| **PIR Motion Sensor** | OUT | `GPIO 13` | Digital Input with internal pull-down |
| **Rotary Encoder CLK** | CLK | `GPIO 18` | Digital Input with internal pull-up |
| **Rotary Encoder DT** | DT | `GPIO 19` | Digital Input with internal pull-up |
| **SSD1306 OLED SDA** | SDA | `GPIO 21` | I2C Data Master (400 kHz Fast Mode) |
| **SSD1306 OLED SCL** | SCL | `GPIO 22` | I2C Clock Master (400 kHz Fast Mode) |
| **Active Buzzer** | Positive (+) | `GPIO 23` | Digital Output (Active High Push-Pull) |

## Task Design
| Task Name | Responsibility | Periodicity / Trigger | Priority | Assigned Core | Typical Block Condition |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `MotionTask` | Sample PIR sensor and refresh activity timers | 50 ms periodic | 3 | Core 0 | `vTaskDelay` |
| `InputTask` | Poll quadrature states and update display mode | 20 ms periodic | 3 | Core 0 | `vTaskDelay` |
| `SensorTask` | Read DHT22 and LDR photoresistor | 2000 ms periodic | 2 | Core 1 | `vTaskDelayUntil` |
| `AlarmTask` | Evaluate thresholds and toggle buzzer | On sensor queue receive | 2 | Core 1 | `xQueueReceive` (Block) |
| `DisplayTask` | Render UI pages on SSD1306 OLED | On mode update / telemetry | 1 | Core 1 | `xQueueReceive` (Block) |

### Priority Justification
- **Priority 3 (`InputTask`, `MotionTask`)**: Real-time user input and human occupancy pulses are ephemeral. Dropping an encoder click leads to poor responsiveness, necessitating top scheduling priority.
- **Priority 2 (`SensorTask`, `AlarmTask`)**: Sensor acquisition takes tens of milliseconds. Processing alarms based on fresh telemetry is vital for safety, running immediately once raw data is packaged.
- **Priority 1 (`DisplayTask`)**: I2C bus rendering involves transmitting 1024 bytes to the SSD1306 RAM, which is computationally sluggish. Running at lowest priority prevents UI flushes from delaying critical telemetry or user inputs.

## Inter-Task Communication
Thread synchronization avoids race conditions and data corruption across dual-core operations:
- **`sensorQueue` (Depth 1)**: Transfers structured `SensorData` payloads from `SensorTask` to both `DisplayTask` and `AlarmTask` using `xQueueOverwrite()`.
- **`modeQueue` (Depth 1)**: Conveys current enumerated display pages from `InputTask` to `DisplayTask`.
- **`systemEvents` (Event Group)**: Manages `EVENT_ACTIVE` (Bit 0) flags to coordinate wake/sleep transitions between `MotionTask` and consumers.
- **`serialMutex`**: Guarded using `xSemaphoreTake(serialMutex, portMAX_DELAY)` within `safe_log()` to prevent mangled diagnostic serial writes over UART.

## State Machine
The core runtime dynamically manages display wake cycles via a deterministic two-state finite state machine:

![State Machine](docs/images/state-machine.png)

*Figure 3: System state-machine transition model.*

- **`ACTIVE` State**: The OLED screen is illuminated, telemetry screens rotate dynamically upon encoder input, sensors cycle normally, and acoustic alarms fire if temperatures violate bounds.
- **`INACTIVE` State**: If no PIR motion is recorded for $\ge$ 15 seconds, the state drops to `INACTIVE`. The OLED screen clears to conserve power, while background sensor polling and PIR interrupt monitoring remain active. Subsequent PIR movement restores the state to `ACTIVE`.

## Repository Structure
```text
bca152-freertos-multisensor/
├── .vscode/               # VS Code PlatformIO workspace definitions
├── docs/
│   ├── images/
│   │   ├── finished-system.png
│   │   ├── freertos-architecture.png
│   │   ├── state-machine.png
│   │   ├── system-architecture.png
│   │   └── wokwi-circuit.png
│   └── laboratory-report.pdf
├── include/               # Public firmware API definitions
│   ├── alarm.h
│   ├── display.h
│   ├── input.h
│   ├── motion.h
│   ├── rtos_objects.h
│   ├── sensors.h
│   └── system_state.h
├── src/                   # Implementation units
│   ├── alarm.cpp
│   ├── display.cpp
│   ├── input.cpp
│   ├── main.cpp
│   ├── motion.cpp
│   ├── rtos_objects.cpp
│   ├── sensors.cpp
│   └── system_state.cpp
├── test/                  # Unity automated verification suites
│   ├── test_alarm.cpp
│   ├── test_navigation.cpp
│   └── test_state.cpp
├── diagram.json           # Wokwi simulation component layout
├── platformio.ini         # PlatformIO toolchain & framework flags
├── README.md              # Public engineering portfolio documentation
└── wokwi.toml             # Wokwi simulator bridging configurations