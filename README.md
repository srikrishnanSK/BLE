# Embedded Systems Learning Platform

A hands-on, project-driven learning platform for embedded systems — from zero to professional BLE product development.

## What Makes This Different

- **Every concept has working code** — not just theory slides
- **Progressive exercises** — guided problems with solutions you can check
- **Quizzes at every level** — test your understanding before moving on
- **Real projects, not toy demos** — each module ends with a buildable project
- **Monetizable capstones** — three complete product-level projects you can ship

## Learning Paths

| Path | For | Modules |
|------|-----|---------|
| **Complete Beginner** | No embedded experience | 01 → 02 → 03 → 04 → 05 → 06 → 07 → 08 → 09 → 10 |
| **Software Developer** | Know C, new to hardware | 02 → 03 → 04 → 05 → 06 → 07 → 08 → 09 → 10 |
| **Arduino Graduate** | Want to go deeper | 01 → 03 → 04 → 05 → 06 → 07 → 08 → 09 → 10 |
| **BLE Fast Track** | Want BLE skills quickly | 01 → 03 → 04 → 07 → 08 → 09 → 10 |

## Curriculum

| # | Module | Level | Key Topics |
|---|--------|-------|------------|
| 01 | [C for Embedded Systems](modules/01-c-for-embedded/) | Beginner | Pointers, bitwise ops, volatile, memory layout, structs |
| 02 | [Digital Electronics](modules/02-digital-electronics/) | Beginner | Binary, logic gates, timing, debouncing |
| 03 | [Microcontroller & GPIO](modules/03-microcontroller-gpio/) | Intermediate | ARM Cortex-M, registers, GPIO, pin modes |
| 04 | [Communication Protocols](modules/04-communication-protocols/) | Intermediate | UART, SPI, I2C — register-level and HAL |
| 05 | [Timers, Interrupts & PWM](modules/05-timers-interrupts-pwm/) | Intermediate | Timer config, ISR design, PWM generation |
| 06 | [ADC, DAC & Sensors](modules/06-adc-dac-sensors/) | Intermediate | Analog conversion, sensor drivers, filtering |
| 07 | [BLE Fundamentals](modules/07-ble-fundamentals/) | Advanced | BLE stack, GATT, advertising, connections |
| 08 | [BLE Advanced](modules/08-ble-advanced/) | Advanced | Custom profiles, Mesh, OTA, BLE 5.x |
| 09 | [RTOS Concepts](modules/09-rtos-concepts/) | Advanced | FreeRTOS tasks, queues, mutexes, scheduling |
| 10 | [Power Management](modules/10-power-management/) | Advanced | Sleep modes, battery optimization, power budgets |

## Capstone Projects

| Project | Business Case | Combines |
|---------|--------------|----------|
| [BLE Asset Tracker](capstone-projects/ble-asset-tracker/) | Indoor positioning for warehouses/hospitals | BLE, RSSI, algorithms, power mgmt |
| [BLE Sensor Hub](capstone-projects/ble-sensor-hub/) | Environmental monitoring for agriculture/buildings | Sensors, GATT, RTOS, data logging |
| [BLE Smart Lock](capstone-projects/ble-smart-lock/) | Access control for coworking/Airbnb | BLE security, motor control, OTA |

## Getting Started

### Minimum Hardware (~$24)
- ESP32-DevKitC ($8)
- Breadboard + jumper wires ($5)
- LEDs, resistors, push buttons ($8)
- USB cable ($3)

### Software Setup
1. Install [PlatformIO](https://platformio.org/) or [Arduino IDE](https://www.arduino.cc/en/software)
2. Clone this repository
3. Start with Module 01 — no hardware needed for the first two modules

### Each Module Contains
```
module/
├── README.md        # Theory & concepts with diagrams
├── examples/        # Working, commented code — read & run
├── exercises/       # Coding challenges (problem statements)
├── solutions/       # Complete solutions with explanations
├── quiz.md          # Knowledge check (MCQ + short answer)
├── quiz_answers.md  # Answer key with explanations
└── project/         # End-of-module mini-project
    ├── README.md    # Project brief & requirements
    ├── starter.c    # Skeleton code
    └── solution.c   # Reference implementation
```

## Scoring

- Examples completed: 10 pts per module
- Exercises: 40 pts per module
- Quiz: 20 pts per module
- Project: 30 pts per module
- **Module total: 100 pts | Platform total: 1300 pts**

## Full Plan

See [PLATFORM_PLAN.md](PLATFORM_PLAN.md) for the complete platform architecture, hardware shopping list, monetization strategy, and detailed module breakdowns.
