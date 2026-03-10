# Module 05: Timers, Interrupts & PWM

## Overview

Timers and interrupts are the heartbeat of every embedded system. Timers provide
precise time measurement, waveform generation, and event scheduling. Interrupts
allow the processor to respond to events immediately rather than wasting cycles
polling. PWM (Pulse Width Modulation) bridges the digital and analog worlds,
enabling smooth LED dimming, motor control, and signal synthesis.

This module covers the full landscape: timer architecture, the interrupt
lifecycle through the NVIC, ISR best practices, PWM generation, input capture,
and watchdog timers.

---

## Table of Contents

1. [Timer Architecture](#1-timer-architecture)
2. [Interrupt Lifecycle & NVIC](#2-interrupt-lifecycle--nvic)
3. [ISR Best Practices](#3-isr-best-practices)
4. [PWM Generation](#4-pwm-generation)
5. [Input Capture](#5-input-capture)
6. [Watchdog Timers](#6-watchdog-timers)
7. [Software Timers](#7-software-timers)
8. [Examples](#8-examples)
9. [Exercises](#9-exercises)
10. [Project](#10-project)

---

## 1. Timer Architecture

### 1.1 What Is a Hardware Timer?

A hardware timer is a peripheral counter driven by a clock source. It counts
up (or down) at a known frequency, enabling precise time measurement without
occupying the CPU.

```
  Clock Source                Timer Block
  +-----------+     +-------------------------------+
  | System    |     |                               |
  | Clock     +---->| Prescaler  ---> Counter (CNT) |
  | (e.g.     |     |   /PSC           |            |
  |  72 MHz)  |     |              Compare (CCR)    |
  +-----------+     |                  |            |
                    |           Overflow/Match ---+--> IRQ
                    |                             +--> Output Pin
                    +-------------------------------+
```

### 1.2 Key Registers

| Register | Name              | Purpose                                      |
|----------|-------------------|----------------------------------------------|
| CNT      | Counter           | Current count value                          |
| PSC      | Prescaler         | Divides clock before reaching counter        |
| ARR      | Auto-Reload       | Value at which counter resets (period)       |
| CCRx     | Capture/Compare   | Threshold for output compare or stored edge  |
| CR1      | Control Register  | Enable, direction, alignment mode            |
| DIER     | DMA/IRQ Enable    | Which events trigger interrupts or DMA       |
| SR       | Status Register   | Flags for update, capture, compare events    |

### 1.3 Timer Clock Calculation

```
Timer Frequency = Clock Source / (PSC + 1)
Timer Period    = (ARR + 1) / Timer Frequency
Timer Period    = (ARR + 1) * (PSC + 1) / Clock Source

Example: 72 MHz clock, PSC=71, ARR=999
  Timer Freq = 72,000,000 / 72 = 1,000,000 Hz (1 MHz)
  Period     = 1000 / 1,000,000 = 1 ms
```

### 1.4 Counting Modes

```
  Up-counting:
  CNT: 0 -> 1 -> 2 -> ... -> ARR -> 0 -> 1 -> ...
                                  ^
                              Update Event (overflow)

  Down-counting:
  CNT: ARR -> ARR-1 -> ... -> 1 -> 0 -> ARR -> ...
                                    ^
                              Update Event (underflow)

  Center-aligned:
  CNT: 0 -> 1 -> ... -> ARR-1 -> ARR -> ARR-1 -> ... -> 1 -> 0 -> ...
                                   ^                          ^
                              Update Events (both peaks)
```

### 1.5 Timer Types (STM32 Example)

| Type           | Timers      | Channels | Features                     |
|----------------|-------------|----------|------------------------------|
| Advanced       | TIM1, TIM8  | 4        | PWM, complementary out, BRK  |
| General 32-bit | TIM2, TIM5  | 4        | PWM, input capture, encoder  |
| General 16-bit | TIM3, TIM4  | 4        | PWM, input capture           |
| Basic          | TIM6, TIM7  | 0        | DAC trigger, time base only  |
| Low-power      | LPTIM1      | 1        | Runs in low-power modes      |

---

## 2. Interrupt Lifecycle & NVIC

### 2.1 What Is an Interrupt?

An interrupt is an asynchronous signal that causes the processor to suspend
its current execution, save context, and jump to a dedicated handler called
an Interrupt Service Routine (ISR). After the ISR completes, execution
resumes where it left off.

### 2.2 Interrupt Flow (ARM Cortex-M)

```
  Normal Execution
  +------------------+
  | main() running   |
  | instruction N    |  <--- IRQ arrives here
  +--------+---------+
           |
           v
  +------------------+
  | 1. Finish current|     Hardware does this automatically:
  |    instruction   |     - Pushes R0-R3, R12, LR, PC, xPSR
  +--------+---------+       onto the stack (8 words)
           |               - Loads PC from vector table
           v               - Switches to Handler mode
  +------------------+
  | 2. Context Save  |
  |    (automatic    |
  |     stacking)    |
  +--------+---------+
           |
           v
  +------------------+
  | 3. Vector Table  |     Vector table at address 0x00000000:
  |    Lookup        |     +--------+------------------+
  +--------+---------+     | Offset | Handler          |
           |               +--------+------------------+
           v               | 0x0000 | Initial SP       |
  +------------------+     | 0x0004 | Reset            |
  | 4. ISR Executes  |     | 0x0008 | NMI              |
  |    (Handler mode)|     | 0x000C | HardFault        |
  +--------+---------+     | ...    | ...              |
           |               | 0x0040 | EXTI0 (example)  |
           v               +--------+------------------+
  +------------------+
  | 5. Context       |
  |    Restore       |
  |    (automatic    |
  |     unstacking)  |
  +--------+---------+
           |
           v
  +------------------+
  | 6. Resume main() |
  |    instruction   |
  |    N+1           |
  +------------------+
```

### 2.3 The NVIC (Nested Vectored Interrupt Controller)

The NVIC is the ARM Cortex-M interrupt management unit. It handles:

- **Priority assignment** (0 = highest priority)
- **Enable/disable** of individual interrupt lines
- **Pending status** tracking
- **Nesting** of higher-priority interrupts over lower-priority ones

```
  NVIC Architecture
  +------------------------------------------------+
  |                    NVIC                         |
  |                                                 |
  |  IRQ Sources        Priority       Pending      |
  |  +---------+      +--------+     +--------+    |
  |  | TIM2    +----->| Pri: 2 +---->|  Pend  +-+  |
  |  +---------+      +--------+     +--------+ |  |
  |  | USART1  +----->| Pri: 1 +---->|  Pend  +-+  |
  |  +---------+      +--------+     +--------+ |  |
  |  | EXTI0   +----->| Pri: 0 +---->|  Pend  +-+  |
  |  +---------+      +--------+     +--------+ |  |
  |  | ADC     +----->| Pri: 3 +---->|  Pend  +-+  |
  |  +---------+      +--------+     +--------+ |  |
  |                                              |  |
  |              Priority Resolution  <----------+  |
  |                      |                          |
  |                      v                          |
  |               To Processor Core                 |
  +------------------------------------------------+
```

### 2.4 Priority and Preemption

ARM Cortex-M uses a priority grouping system with **preemption priority**
(group priority) and **sub-priority**:

```
  8-bit Priority Field (STM32 typically uses upper 4 bits)
  +---+---+---+---+---+---+---+---+
  | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
  +---+---+---+---+---+---+---+---+
  |<- Preemption ->|<- Sub-pri  ->|
       Priority         ority
  (determines           (tiebreaker
   nesting)              within same
                         preemption
                         level)

  Priority Group Configuration Examples:
  +-------+------------+-----------+------------------------+
  | Group | Preemption | Sub-pri   | Use Case               |
  +-------+------------+-----------+------------------------+
  |   0   |  0 bits    | 4 bits    | No preemption          |
  |   1   |  1 bit     | 3 bits    | 2 preempt, 8 sub      |
  |   2   |  2 bits    | 2 bits    | 4 preempt, 4 sub      |
  |   3   |  3 bits    | 1 bit     | 8 preempt, 2 sub      |
  |   4   |  4 bits    | 0 bits    | 16 preempt, no sub    |
  +-------+------------+-----------+------------------------+
```

### 2.5 Nesting Example

```
  Time ----->
  Priority 0 (highest)    :         +--ISR_A--+
  Priority 1              :    +--IS|R_B      |ISR_B--+
  Priority 2              : +--ISR_C|         |       |ISR_C--+
  main()                  : -----+  |         |       |       +-----
                                 ^  ^         ^       ^       ^
                              IRQ_C IRQ_A   A done  B done  C done
                              fires  fires  resumes resumes resumes
                                     (preempts B)    B       C
```

### 2.6 Interrupt Latency

Interrupt latency is the time from the IRQ assertion to the first ISR
instruction executing:

```
  |<------------ Total Interrupt Latency ------------>|
  |                                                    |
  |  Sync   | Stacking | Vector   | Pipeline | ISR    |
  |  (0-1   | (12 cyc  | Fetch    | Fill     | Entry  |
  |  cycle)  | Cortex-M)| (varies) |          |        |
  |         |          |          |          |        |
  IRQ      Start      Stack     Vector     First ISR
  asserted  save      done      fetched    instruction
```

On Cortex-M: Best case is 12 cycles (with zero wait-state memory).

### 2.7 Tail-Chaining

When a lower-priority interrupt is pending as a higher-priority ISR finishes,
the processor skips unstacking and restacking -- it goes directly to the
next ISR. This saves ~6 cycles.

```
  Without tail-chaining:
  ISR_A ---> Unstack ---> Restack ---> ISR_B
             (12 cyc)    (12 cyc)

  With tail-chaining:
  ISR_A ---> ISR_B   (only ~6 cycles gap)
```

---

## 3. ISR Best Practices

### 3.1 The Golden Rules

1. **Keep ISRs short** -- Do minimal work, defer processing to main loop
2. **No blocking calls** -- Never use `delay()`, `printf()`, `malloc()` in ISRs
3. **Use volatile** -- Variables shared between ISR and main must be `volatile`
4. **Clear interrupt flags** -- Always clear the peripheral's interrupt flag
5. **Avoid complex logic** -- No floating point, no loops of uncertain length
6. **Minimize critical sections** -- Disable interrupts for the shortest time

### 3.2 ISR Communication Patterns

```
  Pattern 1: Flag-based (simplest)
  +----------+                    +----------+
  | ISR      |  volatile flag     | main()   |
  |          +------ flag=1 ----->| if(flag) |
  |          |                    | { ... }  |
  +----------+                    +----------+

  Pattern 2: Ring Buffer (for data streams)
  +----------+     +---+---+---+---+---+    +----------+
  | ISR      +---->| W | . | . | . | R |<---+ main()   |
  | (writes) |     +---+---+---+---+---+    | (reads)  |
  +----------+     Ring Buffer               +----------+

  Pattern 3: Double Buffer (for block data)
  +----------+     Buffer A    Buffer B      +----------+
  | ISR      +---->[ filling ] [ ready  ]<---+ main()   |
  |          |     (swap on complete)         | (process)|
  +----------+                                +----------+
```

### 3.3 The `volatile` Keyword

```c
/* WRONG -- compiler may optimize away the read */
int flag = 0;
void ISR(void) { flag = 1; }
void main(void) { while (!flag); }  /* May never exit! */

/* CORRECT -- tells compiler the value can change externally */
volatile int flag = 0;
void ISR(void) { flag = 1; }
void main(void) { while (!flag); }  /* Works correctly */
```

### 3.4 Critical Sections

```c
/* Disabling interrupts to protect shared data */
__disable_irq();       /* Enter critical section */
shared_counter++;      /* Atomic with respect to ISRs */
__enable_irq();        /* Exit critical section */

/* Better: Save and restore interrupt state */
uint32_t primask = __get_PRIMASK();
__disable_irq();
shared_counter++;
__set_PRIMASK(primask);  /* Restore previous state */
```

---

## 4. PWM Generation

### 4.1 What Is PWM?

PWM (Pulse Width Modulation) creates a digital signal that approximates an
analog voltage by rapidly switching between HIGH and LOW at a fixed frequency.
The ratio of HIGH time to total period is the **duty cycle**.

```
  Duty Cycle = T_on / T_period * 100%

  25% Duty Cycle:                 75% Duty Cycle:
  +--+            +--+            +--------+    +--------+
  |  |            |  |            |        |    |        |
  +  +------------+  +--------   +        +--+ +        +--+
  |<--- Period -->|               |<- Period->|

  Average Voltage = V_supply * Duty Cycle
  3.3V at 25% = 0.825V average
  3.3V at 75% = 2.475V average
```

### 4.2 PWM Timer Configuration

```
  Timer in PWM Mode 1 (up-counting):
  CNT counts 0 -> ARR, then resets

  ARR = 999 (period = 1000 counts)
  CCR = 250 (25% duty cycle)

  CNT:  0    250        999   0    250        999
        |     |          |    |     |          |
  Output: HIGH |  LOW     |  HIGH |   LOW      |
        +------+          +------+             +
        |      |          |      |             |
        +      +----------+      +-------------+

  When CNT < CCR  -> Output HIGH
  When CNT >= CCR -> Output LOW
```

### 4.3 PWM Resolution

```
  Resolution (bits) = log2(ARR + 1)

  +----------+----------+------------+------------------+
  | ARR + 1  | Bits     | Steps      | Min Duty Change  |
  +----------+----------+------------+------------------+
  |      256 |  8-bit   |     256    |  0.39%           |
  |     1024 | 10-bit   |    1024    |  0.098%          |
  |     4096 | 12-bit   |    4096    |  0.024%          |
  |    65536 | 16-bit   |   65536    |  0.0015%         |
  +----------+----------+------------+------------------+

  Trade-off: Higher resolution = lower maximum PWM frequency
  PWM Freq = Timer Clock / (PSC + 1) / (ARR + 1)
```

### 4.4 Dead-Time Insertion (Advanced Timers)

For driving H-bridges (motor control), complementary PWM outputs need
dead-time to prevent shoot-through:

```
  Without dead-time (DANGEROUS for H-bridge):
  CH1:  +------+      +------+
        |      |      |      |
  ------+      +------+      +------
  CH1N: -------+      +------+      +---   <-- Overlap causes
               |      |      |      |       shoot-through!
               +------+      +------+

  With dead-time (SAFE):
  CH1:  +------+         +------+
        |      |         |      |
  ------+      +---------+      +------
  CH1N: ----------+      +--------+
                  |      |        |
                  +------+        +------
              |DT|            |DT|
              Dead-time       Dead-time
```

---

## 5. Input Capture

### 5.1 Concept

Input capture records the timer count value (CNT) when an external signal
edge is detected. This allows precise measurement of:

- **Frequency** (period between rising edges)
- **Pulse width** (time between rising and falling edge)
- **Duty cycle** (pulse width / period)

```
  Input Signal:
         +--------+     +----+        +--------+
         |        |     |    |        |        |
  -------+        +-----+    +--------+        +-------

  Timer:  |<- captured values stored in CCR register ->|

  Capture on rising edge:
    CCR values: T1=1000, T2=5000, T3=7000, T4=11000

  Period between T1 and T2: 5000 - 1000 = 4000 counts
  Frequency = Timer_Clock / 4000
```

### 5.2 Ultrasonic Sensor Timing

```
  MCU                                 HC-SR04
  +--------+    Trigger (10us)    +----------+
  | GPIO   +--------------------->| Trig     |
  |        |                      |          |
  | Timer  |<---------------------+ Echo     |
  | Input  |    Echo pulse        |          |
  | Capture|    (proportional     +----------+
  +--------+     to distance)

  Echo Timing:
  +----------+                          +----------+
  | Trigger  |                          | Echo     |
  | 10us     |                          | pulse    |
  +----+-----+                          +--+----+--+
       |                                   |    |
       v                                   v    v
  Trigger sent                        Rising  Falling
                                      edge    edge
                                      T1      T2

  Distance = (T2 - T1) * Speed_of_Sound / 2
  Speed of sound = 343 m/s = 0.0343 cm/us
  Distance (cm) = (T2 - T1) * 0.0343 / 2
```

---

## 6. Watchdog Timers

### 6.1 Purpose

A watchdog timer is a safety mechanism that resets the system if the software
fails to "kick" (refresh) the watchdog within a defined time window. This
protects against:

- Infinite loops
- Deadlocks
- Runaway code
- Hardware faults causing software hangs

### 6.2 Watchdog Operation

```
  Normal Operation:
  +-------+    +-------+    +-------+    +-------+
  | Task  +--->| Kick  +--->| Task  +--->| Kick  +--->  ...
  |       |    | WDG   |    |       |    | WDG   |
  +-------+    +-------+    +-------+    +-------+

  WDG Counter:  MAX ... falling ... kicked back to MAX ... falling ...


  Fault Condition:
  +-------+    +-------+         HANG!
  | Task  +--->| Kick  +--->| Task stuck |
  |       |    | WDG   |    | in loop... |
  +-------+    +-------+    +-----------+

  WDG Counter:  MAX ... falling ... falling ... 0 = RESET!
                                                    ^
                                               System resets
```

### 6.3 Types of Watchdog

```
  Independent Watchdog (IWDG):            Window Watchdog (WWDG):
  - Separate RC oscillator                - Uses system clock
  - Simple: just must kick                - Must kick within a window
    before timeout                        - Too early = reset!
  - Runs even if main clock fails         - Too late = reset!

  IWDG:                                   WWDG:
  |<---- Timeout ---->|                   |<-Too early->|<-Window->|<-Late->|
  +---------+---------+                   +------+------+----------+--------+
  | OK zone           | RESET             |RESET | OK   | OK zone  | RESET  |
  +---------+---------+                   +------+------+----------+--------+
  Kick any time before timeout            Must kick only in window
```

---

## 7. Software Timers

### 7.1 Concept

Software timers use a single hardware timer to manage multiple virtual timers.
A periodic hardware interrupt checks which software timers have expired and
calls their callbacks.

```
  Hardware Timer (1ms tick)
        |
        v
  +-----+------+
  | Tick ISR   |
  | for each   |
  | sw_timer:  |
  |  decrement |
  |  if 0:     |
  |   set flag |
  +-----+------+
        |
  +-----+-----+-----+-----+
  |     |     |     |     |
  v     v     v     v     v
  SW    SW    SW    SW    SW
  Tim1  Tim2  Tim3  Tim4  Tim5
  100ms 250ms 1s    50ms  500ms
  LED   ADC   Log   Btn   Comm
```

### 7.2 Software Timer States

```
  +----------+    start()    +-----------+
  |          +-------------->|           |
  | STOPPED  |               | RUNNING   +----+
  |          |<--------------+           |    | expires
  +----+-----+    stop()     +-----+-----+    | (one-shot)
       ^                           |          |
       |                           | expires  |
       +------ (one-shot) --------+          |
                                              |
                              +--------+      |
                              |EXPIRED |<-----+
                              |(call   |
                              |callback|  reload
                              +---+----+ (periodic)
                                  |        |
                                  +--------+
```

---

## 8. Examples

| # | File | Description |
|---|------|-------------|
| 1 | `examples/01_timer_delay.c` | Microsecond-precision delay using timer |
| 2 | `examples/02_periodic_interrupt.c` | Periodic interrupt handler (1ms tick) |
| 3 | `examples/03_pwm_led.c` | PWM LED dimming with smooth fade |
| 4 | `examples/04_servo_control.c` | Servo motor control (1-2ms pulse) |
| 5 | `examples/05_ultrasonic.c` | Ultrasonic distance measurement |
| 6 | `examples/06_watchdog.c` | Watchdog timer implementation |
| 7 | `examples/07_software_timer.c` | Software timer library |

---

## 9. Exercises

| # | File | Topic |
|---|------|-------|
| 1 | `exercises/exercise_01.md` | Non-blocking precise delay |
| 2 | `exercises/exercise_02.md` | Multi-channel PWM for RGB LED |
| 3 | `exercises/exercise_03.md` | Frequency counter |
| 4 | `exercises/exercise_04.md` | Reaction time game |
| 5 | `exercises/exercise_05.md` | PWM motor speed with acceleration ramp |
| 6 | `exercises/exercise_06.md` | Interrupt-driven UART |
| 7 | `exercises/exercise_07.md` | Nested interrupt priority demo |

---

## 10. Project

**Digital Oscilloscope** -- Build an ASCII-art oscilloscope using ADC sampling
triggered by a timer, with frequency/amplitude/duty-cycle measurement.
See `project/README.md` for full details.

---

## Quick Reference

### Common Timer Formulas

```
Timer Tick Frequency  = F_clk / (PSC + 1)
Timer Period (sec)    = (ARR + 1) * (PSC + 1) / F_clk
PWM Frequency         = F_clk / ((PSC + 1) * (ARR + 1))
PWM Duty Cycle (%)    = CCR / (ARR + 1) * 100
Input Capture Period  = (Capture2 - Capture1) / Timer_Tick_Freq
```

### Interrupt Priority Cheat Sheet

```
Lower number = Higher priority (0 is highest)
Preemption priority: determines if ISR_A can interrupt ISR_B
Sub-priority: determines order when two ISRs pend simultaneously
```

### Watchdog Timeout Calculation

```
IWDG Timeout = (Prescaler * Reload) / LSI_Frequency
Example: Prescaler=64, Reload=625, LSI=32kHz
  Timeout = (64 * 625) / 32000 = 1.25 seconds
```
