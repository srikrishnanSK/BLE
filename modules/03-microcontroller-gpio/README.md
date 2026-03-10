# Module 03: Microcontroller Architecture & GPIO

## Table of Contents

1. [Introduction](#introduction)
2. [CPU Architecture: Harvard vs Von Neumann](#cpu-architecture-harvard-vs-von-neumann)
3. [ARM Cortex-M Overview](#arm-cortex-m-overview)
4. [Clock Tree](#clock-tree)
5. [Memory Map](#memory-map)
6. [GPIO Registers](#gpio-registers)
7. [Pin Modes](#pin-modes)
8. [Register-Level vs HAL Programming](#register-level-vs-hal-programming)
9. [Practical Considerations](#practical-considerations)
10. [Summary](#summary)

---

## Introduction

A microcontroller is a complete computer system on a single chip. Unlike a general-purpose
processor in a laptop or server, a microcontroller integrates CPU, memory (Flash and SRAM),
and peripherals (GPIO, timers, ADC, UART, SPI, I2C, and more) into one package. This module
explores the internal architecture of the ARM Cortex-M family of microcontrollers, focusing
on how the CPU accesses memory and peripherals, and dives deep into GPIO (General-Purpose
Input/Output) -- the most fundamental peripheral for interacting with the physical world.

---

## CPU Architecture: Harvard vs Von Neumann

The two dominant processor architectures differ in how they organize memory buses.

### Von Neumann Architecture

A Von Neumann machine uses a single bus for both instructions and data. The CPU fetches an
instruction and reads/writes data over the same path. This creates a structural bottleneck
known as the "Von Neumann bottleneck" because the CPU cannot fetch an instruction and access
data simultaneously.

```
Von Neumann Architecture
========================

    +-------------------+
    |       CPU         |
    |  +-----+ +-----+ |
    |  | ALU | | CU  | |
    |  +-----+ +-----+ |
    |     +-------+     |
    |     | Regs  |     |
    |     +-------+     |
    +--------+----------+
             |
             | <-- Single shared bus
             |     (instructions + data)
             |
    +--------+----------+
    |      Memory       |
    | +------+ +------+ |
    | | Code | | Data | |
    | +------+ +------+ |
    +-------------------+

Characteristics:
  - Single address space
  - Simpler hardware design
  - Structural bottleneck: cannot fetch
    instruction and data simultaneously
  - Used in: x86 processors, original
    microprocessors
```

### Harvard Architecture

A Harvard machine uses separate buses for instructions and data. The CPU can fetch the next
instruction while simultaneously reading or writing data, resulting in higher throughput.

```
Harvard Architecture
====================

    +-------------------+
    |       CPU         |
    |  +-----+ +-----+ |
    |  | ALU | | CU  | |
    |  +-----+ +-----+ |
    |     +-------+     |
    |     | Regs  |     |
    |     +-------+     |
    +---+----------+----+
        |          |
        |          | <-- Separate buses
   Instr|          |Data
   Bus  |          |Bus
        |          |
   +----+---+ +---+----+
   | Program| | Data   |
   | Memory | | Memory |
   | (Flash)| | (SRAM) |
   +---------+ +--------+

Characteristics:
  - Separate address spaces for code and data
  - Higher throughput (parallel fetch + data access)
  - More complex hardware (more pins/buses)
  - Used in: DSPs, most microcontrollers
```

### Modified Harvard Architecture (ARM Cortex-M)

ARM Cortex-M processors use a **modified Harvard architecture**. They have separate
instruction and data buses internally (via the bus matrix), but present a **unified address
space** to the programmer. This means code and data live in the same logical address space,
but the hardware can still fetch instructions and access data in parallel when they target
different memory regions.

```
Modified Harvard (ARM Cortex-M)
================================

    +---------------------+
    |     Cortex-M CPU    |
    |  +------+ +------+  |
    |  | ALU  | | Ctrl |  |
    |  +------+ +------+  |
    |  +---------------+  |
    |  |   Registers   |  |
    |  | R0-R12,SP,LR, |  |
    |  | PC, xPSR      |  |
    |  +---------------+  |
    +---+----------+------+
        |          |
    I-Bus|      D-Bus  (+ System bus)
        |          |
    +---+----------+------+
    |     Bus Matrix      |  <-- Routes requests to
    +--+-----+------+-----+      correct target
       |     |      |
       v     v      v
    +-----+ +----+ +------------+
    |Flash| |SRAM| | Peripherals|
    +-----+ +----+ +------------+

Key point: Single address space visible
to the programmer, but parallel internal
buses for performance.
```

### Comparison Table

```
+-------------------+----------------+----------------+-------------------+
| Feature           | Von Neumann    | Harvard        | Modified Harvard  |
+-------------------+----------------+----------------+-------------------+
| Memory buses      | 1 (shared)     | 2 (separate)   | 2+ (internal)     |
| Address spaces    | 1 (unified)    | 2 (separate)   | 1 (unified)       |
| Instruction fetch | Blocks data    | Parallel       | Parallel          |
|   vs data access  |                |                |                   |
| Complexity        | Simple         | Complex        | Moderate          |
| Code in RAM       | Yes            | No             | Yes               |
| Example           | x86            | PIC, AVR       | ARM Cortex-M      |
+-------------------+----------------+----------------+-------------------+
```

---

## ARM Cortex-M Overview

ARM Cortex-M is a family of 32-bit RISC processor cores designed specifically for
microcontroller applications. ARM (Advanced RISC Machines) licenses the core designs
to chip manufacturers (ST, NXP, TI, Nordic, etc.), who then build complete
microcontrollers around them.

### Cortex-M Family Variants

```
Cortex-M Family
================

+------------+-------+--------+--------+----------+------------------+
| Core       | Bits  | Pipeline| FPU   | DSP      | Typical Use      |
+------------+-------+--------+--------+----------+------------------+
| Cortex-M0  |  32   | 2-stage| No     | No       | Ultra low-power  |
| Cortex-M0+ |  32   | 2-stage| No     | No       | IoT, wearables   |
| Cortex-M3  |  32   | 3-stage| No     | Some     | General embedded |
| Cortex-M4  |  32   | 3-stage| Opt.   | Yes      | Signal processing|
| Cortex-M7  |  32   | 6-stage| Opt.   | Yes      | High performance |
| Cortex-M23 |  32   | 2-stage| No     | No       | Secure IoT       |
| Cortex-M33 |  32   | 3-stage| Opt.   | Yes      | Secure + DSP     |
+------------+-------+--------+--------+----------+------------------+

This module primarily targets the STM32F4 series (Cortex-M4)
but concepts apply broadly across all Cortex-M variants.
```

### Cortex-M4 Core Internals

```
Cortex-M4 CPU Core
===================

+--------------------------------------------------+
|                  Cortex-M4 Core                   |
|                                                   |
|  +----------------+    +---------------------+    |
|  | Register File  |    |  Execution Units    |    |
|  |                |    |                     |    |
|  | R0  (General)  |    |  +------+ +------+  |    |
|  | R1  (General)  |    |  | ALU  | | MAC  |  |    |
|  | R2  (General)  |    |  +------+ +------+  |    |
|  | R3  (General)  |    |  +------+ +------+  |    |
|  | R4  (General)  |    |  | FPU  | |Shift |  |    |
|  | R5  (General)  |    |  +------+ +------+  |    |
|  | R6  (General)  |    +---------------------+    |
|  | R7  (General)  |                               |
|  | R8  (General)  |    +---------------------+    |
|  | R9  (General)  |    |  3-Stage Pipeline   |    |
|  | R10 (General)  |    |                     |    |
|  | R11 (General)  |    | Fetch -> Decode ->  |    |
|  | R12 (General)  |    |            Execute  |    |
|  | R13 (SP)       |    +---------------------+    |
|  | R14 (LR)       |                               |
|  | R15 (PC)       |    +---------------------+    |
|  | xPSR (Status)  |    |  NVIC (Interrupts)  |    |
|  +----------------+    |  Up to 240 IRQs     |    |
|                         |  Configurable prio  |    |
|                         +---------------------+    |
+--------------------------------------------------+
```

### Key Registers

| Register | Alias | Purpose |
|----------|-------|---------|
| R0-R12 | -- | General-purpose registers |
| R13 | SP | Stack Pointer (MSP or PSP) |
| R14 | LR | Link Register (return address) |
| R15 | PC | Program Counter (current instruction) |
| xPSR | -- | Program Status Register (flags N, Z, C, V, etc.) |

### Thumb-2 Instruction Set

Cortex-M exclusively uses the Thumb-2 instruction set, which is a mix of 16-bit and 32-bit
instructions. This provides a good balance between code density (small Flash usage) and
performance. Unlike ARM7/ARM9, there is no "ARM mode" -- only Thumb-2.

### NVIC (Nested Vectored Interrupt Controller)

The NVIC is tightly coupled to the CPU core and provides:

- Up to 240 external interrupt sources (device-specific)
- Programmable priority levels (8 to 256 levels depending on implementation)
- Automatic context saving/restoring on interrupt entry/exit
- Tail-chaining: back-to-back interrupt handling without full context restore
- Late arrival: higher-priority interrupt can preempt during stacking

---

## Clock Tree

The clock tree distributes clock signals from various sources to the CPU core and
peripherals. Understanding it is essential because peripherals will not function until
their clock is enabled, and the system frequency affects performance and power consumption.

### Clock Sources

```
Clock Sources
=============

+------------------+          +------------------+
|   HSI (High      |          |   HSE (High      |
|   Speed Internal)|          |   Speed External)|
|   RC Oscillator  |          |   Crystal/Osc    |
|   Typ: 16 MHz    |          |   Typ: 8-25 MHz  |
+--------+---------+          +--------+---------+
         |                             |
         v                             v
    +---------+                   +---------+
    |  /M     |  PLL Input Mux    |         |
    | (div)   +<------------------+         |
    +---------+                             |
         |                                  |
         v                                  |
    +----------+                            |
    |   PLL    |                            |
    |  x N / P |                            |
    +----+-----+                            |
         |                                  |
         v                                  |
    +----+------+     System Clock Mux      |
    |           +<--------------------------+
    | SYSCLK    +<--- HSI (direct)
    |           |
    +-----+-----+
          |
          v
    +-----+-----+
    |  AHB      |  (HCLK)
    | Prescaler |  /1, /2, /4 ... /512
    +--+-----+--+
       |     |
       v     v
    +--+--+ +-+---+
    |APB1 | |APB2 |   (PCLK1, PCLK2)
    |/1-16| |/1-16|
    +--+--+ +--+--+
       |       |
       v       v
    Low-speed  High-speed
    periph.    periph.
    (UART,     (SPI1,
     I2C,      TIM1,
     TIM2-7)   ADC)

+------------------+          +------------------+
|   LSI (Low       |          |   LSE (Low       |
|   Speed Internal)|          |   Speed External)|
|   ~32 kHz        |          |   32.768 kHz     |
+------------------+          +------------------+
     |                             |
     +-----> RTC, IWDG <-----------+
```

### STM32F4 Typical Clock Configuration

For an STM32F446 with an 8 MHz external crystal running at 180 MHz:

```
HSE = 8 MHz
PLL_M = 8      -->  8 / 8  = 1 MHz (VCO input)
PLL_N = 360    -->  1 * 360 = 360 MHz (VCO output)
PLL_P = 2      -->  360 / 2 = 180 MHz (SYSCLK)
AHB prescaler  = 1  --> HCLK  = 180 MHz
APB1 prescaler = 4  --> PCLK1 = 45 MHz  (max for APB1)
APB2 prescaler = 2  --> PCLK2 = 90 MHz  (max for APB2)
```

### Why Clocks Matter for GPIO

Before using any GPIO port, you must enable its clock in the RCC (Reset and Clock Control)
peripheral. Without the clock, the GPIO registers are unpowered and inaccessible (reads
return 0, writes are ignored).

```c
// Enable GPIOA clock on STM32F4
RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;  // Set bit 0
```

---

## Memory Map

ARM Cortex-M uses a flat 32-bit address space (4 GB total) with predefined regions.
The memory map is standardized across all Cortex-M implementations, though specific
peripheral addresses vary by chip manufacturer.

```
ARM Cortex-M Memory Map (32-bit = 4 GB)
=========================================

0xFFFFFFFF +---------------------------+
           |  System / Vendor-specific |  512 MB
           |  (Internal PPB, debug)    |
0xE0000000 +---------------------------+
           |                           |
           |  External Device          |  1 GB
           |  (External peripherals)   |
0xA0000000 +---------------------------+
           |                           |
           |  External RAM             |  1 GB
           |  (External memory)        |
0x60000000 +---------------------------+
           |                           |
           |  Peripheral               |  512 MB
           |  (On-chip peripherals:    |
           |   GPIO, UART, SPI, I2C,   |
           |   TIM, ADC, DMA, RCC)     |
0x40000000 +---------------------------+
           |                           |
           |  SRAM                     |  512 MB
           |  (On-chip RAM, typically  |
           |   64-256 KB used)         |
0x20000000 +---------------------------+
           |                           |
           |  Code                     |  512 MB
           |  (Flash, typically        |
           |   128 KB - 2 MB used)     |
0x00000000 +---------------------------+
```

### STM32F4 Specific Memory Map (Key Regions)

```
STM32F446 Memory Map (Selected)
=================================

Address         Region              Size
-----------     ------------------  --------
0x0800 0000     Flash (code)        512 KB
0x1FFF 0000     System memory       30 KB (bootloader)
0x1FFF 7800     OTP area            528 B
0x2000 0000     SRAM1               112 KB
0x2001 C000     SRAM2               16 KB

Peripherals (0x4000 0000 - 0x5FFF FFFF):
-----------
0x4000 0000     APB1 peripherals
                  TIM2-TIM7, SPI2/3, USART2/3,
                  I2C1-3, CAN1/2, PWR, DAC
0x4001 0000     APB2 peripherals
                  TIM1, TIM8, USART1/6, ADC,
                  SPI1/4, SYSCFG, EXTI
0x4002 0000     AHB1 peripherals
                  GPIOA  @ 0x4002 0000
                  GPIOB  @ 0x4002 0400
                  GPIOC  @ 0x4002 0800
                  GPIOD  @ 0x4002 0C00
                  GPIOE  @ 0x4002 1000
                  RCC    @ 0x4002 3800
                  DMA1   @ 0x4002 6000
                  DMA2   @ 0x4002 6400
0x4001 0000     AHB2 peripherals
                  USB OTG FS
0x5006 0000     AHB3 peripherals
                  FMC, QUADSPI

Cortex-M Internal:
-----------
0xE000 E010     SysTick
0xE000 E100     NVIC
0xE000 ED00     SCB (System Control Block)
```

### Memory-Mapped I/O

On ARM Cortex-M, there are no special I/O instructions. Peripherals are accessed by reading
from and writing to specific memory addresses. This is called **memory-mapped I/O**. A GPIO
register is just a 32-bit word at a particular address, and you interact with it using normal
load/store instructions (or pointer dereferences in C).

```c
// These two are equivalent:
// 1) Using struct pointer (typical in vendor headers)
GPIOA->ODR |= (1 << 5);

// 2) Using raw address (for understanding)
*((volatile uint32_t *)0x40020014) |= (1 << 5);
//                      ^^^^^^^^^^
//                      GPIOA base (0x40020000) + ODR offset (0x14)
```

The `volatile` keyword is critical: it tells the compiler that the value at this address can
change at any time (because hardware can modify it), so the compiler must not optimize away
reads or reorder accesses.

---

## GPIO Registers

Each GPIO port (GPIOA through GPIOK, depending on the chip) has the same set of registers.
We will examine the key registers using GPIOA on the STM32F4 as our reference.

### GPIO Register Map

```
GPIO Register Map (per port)
=============================

Offset  Register   Full Name                   Access
------  ---------  --------------------------  ------
0x00    MODER      Mode Register               R/W
0x04    OTYPER     Output Type Register        R/W
0x08    OSPEEDR    Output Speed Register       R/W
0x0C    PUPDR      Pull-Up/Pull-Down Register  R/W
0x10    IDR        Input Data Register         R
0x14    ODR        Output Data Register        R/W
0x18    BSRR       Bit Set/Reset Register      W
0x1C    LCKR       Lock Register               R/W
0x20    AFRL       Alternate Function Low       R/W
0x24    AFRH       Alternate Function High      R/W
```

### MODER - Mode Register (Offset 0x00)

The MODER register configures each pin as input, output, alternate function, or analog.
Each pin uses 2 bits, so the 32-bit register handles all 16 pins (0-15) of a port.

```
MODER Register (32 bits, 2 bits per pin)
==========================================

Bit:  31 30 29 28 27 26 25 24 23 22 21 20 19 18 17 16 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
      +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+
      |P15  |P14  |P13  |P12  |P11  |P10  | P9  | P8  | P7  | P6  | P5  | P4  | P3  | P2  | P1  | P0  |
      +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+

Mode values (2-bit field per pin):
  00 = Input mode          (reset state for most pins)
  01 = General purpose output
  10 = Alternate function  (UART, SPI, I2C, PWM, etc.)
  11 = Analog mode         (ADC, DAC)

Example: Configure PA5 as output
  - Pin 5 uses bits [11:10]
  - Clear both bits, then set to 01

  GPIOA->MODER &= ~(0x3 << (5 * 2));   // Clear bits 11:10
  GPIOA->MODER |=  (0x1 << (5 * 2));   // Set to 01 (output)
```

### OTYPER - Output Type Register (Offset 0x04)

Selects between push-pull and open-drain output for each pin. Uses only 1 bit per pin
(lower 16 bits of the register).

```
OTYPER Register (lower 16 bits used, 1 bit per pin)
=====================================================

Bit: 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
     |15|14|13|12|11|10| 9| 8| 7| 6| 5| 4| 3| 2| 1| 0|
     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

  0 = Push-pull   (can drive high AND low)
  1 = Open-drain  (can only pull low; needs external pull-up for high)

Push-Pull vs Open-Drain:

Push-Pull:                    Open-Drain:
  VDD                           VDD
   |                             |
  [P-FET]--+                    [R] (external)
            |                    |
   Pin ----+--- Output          Pin ----+--- Output
            |                            |
  [N-FET]--+                   [N-FET]--+
   |                             |
  GND                           GND

  Output=1: P-FET on            Output=1: N-FET off (floats/pulled up)
  Output=0: N-FET on            Output=0: N-FET on  (pulls low)
```

### OSPEEDR - Output Speed Register (Offset 0x08)

Controls the slew rate (how fast the output transitions). Higher speed means faster edges
but more EMI and power consumption. 2 bits per pin.

```
Speed values (2-bit field per pin):
  00 = Low speed        (~2 MHz)
  01 = Medium speed     (~25 MHz)
  10 = Fast speed       (~50 MHz)
  11 = High speed       (~100 MHz)

Use the lowest speed that meets your timing requirements
to minimize electromagnetic interference (EMI).
```

### PUPDR - Pull-Up / Pull-Down Register (Offset 0x0C)

Configures internal pull-up or pull-down resistors. 2 bits per pin.

```
PUPDR values (2-bit field per pin):
  00 = No pull-up, no pull-down  (floating)
  01 = Pull-up
  10 = Pull-down
  11 = Reserved

Internal Pull Resistors (~40 kOhm on STM32):

Pull-Up:                  Pull-Down:
  VDD                       Pin ---+--- Input
   |                                |
  [~40k]                          [~40k]
   |                                |
  Pin ---+--- Input               GND
```

### IDR - Input Data Register (Offset 0x10)

A read-only register that reflects the current logic level on each pin. Each bit
corresponds to one pin (bit 0 = pin 0, bit 15 = pin 15). Upper 16 bits are reserved.

```
IDR Register (lower 16 bits, read-only)
=========================================

Bit: 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
     |15|14|13|12|11|10| 9| 8| 7| 6| 5| 4| 3| 2| 1| 0|
     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

  0 = Pin is LOW  (near GND)
  1 = Pin is HIGH (near VDD)

Example: Read PA0 (button on Nucleo board)
  if (GPIOA->IDR & (1 << 0)) {
      // PA0 is HIGH
  }

  // Or, more explicitly:
  uint32_t pin_state = (GPIOA->IDR >> 0) & 0x1;
```

### ODR - Output Data Register (Offset 0x14)

A read/write register that sets the output level of each pin. Each bit corresponds to one
pin. Writing a 1 drives the pin HIGH; writing a 0 drives it LOW.

```
ODR Register (lower 16 bits, read/write)
==========================================

Bit: 15 14 13 12 11 10  9  8  7  6  5  4  3  2  1  0
     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
     |15|14|13|12|11|10| 9| 8| 7| 6| 5| 4| 3| 2| 1| 0|
     +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+

  Writing 1 = Pin driven HIGH
  Writing 0 = Pin driven LOW

Example: Toggle PA5
  GPIOA->ODR ^= (1 << 5);

WARNING: Read-modify-write on ODR is NOT atomic!
  GPIOA->ODR |= (1 << 5);
  // This compiles to: LDR -> ORR -> STR (3 instructions)
  // An interrupt between LDR and STR could cause a race condition
  // Use BSRR instead for atomic bit manipulation
```

### BSRR - Bit Set/Reset Register (Offset 0x18)

A write-only register that allows atomic (single-instruction) setting and clearing of
individual output bits without affecting other pins. This eliminates the race condition
problem with read-modify-write on ODR.

```
BSRR Register (32 bits, write-only)
=====================================

Bit: 31 30 29 ... 17 16 | 15 14 13 ... 1  0
     +--+--+--+---+--+--+--+--+--+---+--+--+
     |        BR[15:0]   |     BS[15:0]      |
     | (Reset/Clear bits)| (Set bits)        |
     +--+--+--+---+--+--+--+--+--+---+--+--+

  Bits [15:0]  BS (Bit Set):   Writing 1 sets the corresponding ODR bit (pin HIGH)
  Bits [31:16] BR (Bit Reset): Writing 1 clears the corresponding ODR bit (pin LOW)
  Writing 0 to any bit has no effect.

  If both BS and BR are set for the same pin, BS wins (set takes priority).

Example: Set PA5 HIGH (atomic, single write)
  GPIOA->BSRR = (1 << 5);         // BS5 = 1

Example: Set PA5 LOW (atomic, single write)
  GPIOA->BSRR = (1 << (5 + 16));  // BR5 = 1, i.e., bit 21

Why BSRR is preferred over ODR for individual pin control:
  ODR:  LDR R0, [GPIOA_ODR]     // Read current value
        ORR R0, R0, #(1<<5)     // Modify
        STR R0, [GPIOA_ODR]     // Write back (3 instructions, not atomic)

  BSRR: MOV R0, #(1<<5)
        STR R0, [GPIOA_BSRR]   // Single atomic write (2 instructions)
```

### AFRL / AFRH - Alternate Function Registers (Offset 0x20, 0x24)

When a pin is in alternate function mode (MODER = 10), AFRL and AFRH select which
peripheral is connected to that pin. Each pin gets a 4-bit field (AF0 through AF15).

```
AFRL handles pins 0-7   (4 bits each, 32 bits total)
AFRH handles pins 8-15  (4 bits each, 32 bits total)

AF Number  Typical Function (varies by pin/chip)
---------  ----------------------------------------
AF0        System (SWD, MCO, etc.)
AF1        TIM1, TIM2
AF2        TIM3, TIM4, TIM5
AF3        TIM8, TIM9, TIM10, TIM11
AF4        I2C1, I2C2, I2C3
AF5        SPI1, SPI2
AF6        SPI3
AF7        USART1, USART2, USART3
AF8        UART4, UART5, USART6
AF9        CAN1, CAN2, TIM12-14
AF10       USB OTG
AF11       ETH
AF12       FMC, SDIO
AF13       DCMI
AF14       LCD-TFT
AF15       EVENTOUT

Example: Set PA9 to AF7 (USART1_TX)
  GPIOA->AFRH &= ~(0xF << ((9 - 8) * 4));  // Clear AF bits for pin 9
  GPIOA->AFRH |=  (0x7 << ((9 - 8) * 4));   // Set AF7
```

---

## Pin Modes

A GPIO pin can operate in one of four fundamental modes, each with further configuration
options.

```
GPIO Pin Mode Decision Tree
=============================

Is the pin connecting to
a digital peripheral (UART, SPI, I2C, PWM)?
  |
  +-- YES --> Alternate Function Mode (MODER = 10)
  |           Select AF number via AFRL/AFRH
  |           Configure OTYPER, OSPEEDR, PUPDR as needed
  |
  +-- NO
       |
       Is the pin used for ADC or DAC?
         |
         +-- YES --> Analog Mode (MODER = 11)
         |           No pull-up/down, lowest power on the pin
         |
         +-- NO
              |
              Is the pin an output (driving LED, relay, etc.)?
                |
                +-- YES --> Output Mode (MODER = 01)
                |           Choose push-pull or open-drain (OTYPER)
                |           Choose speed (OSPEEDR)
                |           Usually no pull-up/down needed
                |
                +-- NO --> Input Mode (MODER = 00)
                           Configure pull-up/pull-down (PUPDR)
                           or use external resistor
                           Read via IDR
```

### Input Mode Details

```
Input Configuration
====================

External Button Circuit (active LOW with pull-up):

  VDD (3.3V)
   |
  [10k]  <-- External pull-up resistor
   |
   +-------> MCU Pin (configured as input)
   |
  [ / ]  <-- Push button (normally open)
   |
  GND

  Button released: Pin reads HIGH (1)
  Button pressed:  Pin reads LOW  (0)

Or use internal pull-up (saves external component):
  GPIOA->PUPDR |= (0x1 << (pin * 2));  // Enable pull-up
```

### Output Mode Details

```
Output Driving an LED
======================

Option 1: Active HIGH (sourcing current)      Option 2: Active LOW (sinking current)

  MCU Pin ---[330R]---+--- LED_A ---+          VDD
  (Output)            |    (+) (-)  |           |
                      |             |          LED_A
                      +-------------+          (-)  (+)
                                    |           |
                                   GND        [330R]
                                                |
                                            MCU Pin
                                            (Output)

  ODR = 1: LED ON                           ODR = 0: LED ON
  ODR = 0: LED OFF                          ODR = 1: LED OFF
```

---

## Register-Level vs HAL Programming

There are two primary approaches to programming STM32 peripherals.

### Register-Level (Bare Metal)

Direct manipulation of hardware registers using addresses and bit operations.

```c
/* Register-Level: Blink LED on PA5 */

// Enable GPIOA clock
*((volatile uint32_t *)0x40023830) |= (1 << 0);

// Configure PA5 as output
*((volatile uint32_t *)0x40020000) &= ~(3 << 10);
*((volatile uint32_t *)0x40020000) |=  (1 << 10);

// Toggle LED
while (1) {
    *((volatile uint32_t *)0x40020014) ^= (1 << 5);
    for (volatile int i = 0; i < 1000000; i++);
}
```

Or, more readably with CMSIS headers:

```c
/* Register-Level with CMSIS headers */
#include "stm32f4xx.h"

RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

GPIOA->MODER &= ~GPIO_MODER_MODE5_Msk;
GPIOA->MODER |=  GPIO_MODER_MODE5_0;  // 01 = output

while (1) {
    GPIOA->ODR ^= GPIO_ODR_OD5;
    for (volatile int i = 0; i < 1000000; i++);
}
```

### HAL (Hardware Abstraction Layer)

ST provides the HAL library that wraps register access in function calls.

```c
/* HAL-Level: Blink LED on PA5 */
#include "stm32f4xx_hal.h"

int main(void) {
    HAL_Init();
    SystemClock_Config();

    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin   = GPIO_PIN_5;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOA, &gpio);

    while (1) {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        HAL_Delay(500);
    }
}
```

### Comparison

```
+---------------------+-------------------------+-------------------------+
| Aspect              | Register-Level          | HAL                     |
+---------------------+-------------------------+-------------------------+
| Code size           | Minimal                 | Larger (library code)   |
| Performance         | Optimal                 | Slight overhead         |
| Readability         | Lower (bit magic)       | Higher (named funcs)    |
| Portability         | Chip-specific           | Across STM32 family     |
| Learning value      | Deep understanding      | Faster development      |
| Debug difficulty    | Need datasheet open     | Stack traces help       |
| Interrupt safety    | Must handle yourself    | Some built-in safety    |
| Startup time        | Faster compile/link     | Slower (more code)      |
+---------------------+-------------------------+-------------------------+

Recommendation: Learn register-level first for understanding, then use
HAL (or LL - Low-Level drivers) for production projects.
```

---

## Practical Considerations

### Bit Manipulation Cheat Sheet

```c
// Set bit n
register |= (1 << n);

// Clear bit n
register &= ~(1 << n);

// Toggle bit n
register ^= (1 << n);

// Check if bit n is set
if (register & (1 << n)) { /* bit is set */ }

// Set a 2-bit field at position p to value v (0-3)
register &= ~(0x3 << (p * 2));    // Clear field
register |=  (v   << (p * 2));    // Set new value

// Set a 4-bit field at position p to value v (0-15)
register &= ~(0xF << (p * 4));    // Clear field
register |=  (v   << (p * 4));    // Set new value
```

### Common GPIO Setup Pattern

```c
void gpio_setup_output(GPIO_TypeDef *port, uint32_t pin) {
    // 1. Enable clock (must determine which bit based on port)
    //    GPIOA=bit0, GPIOB=bit1, GPIOC=bit2, etc.
    uint32_t port_index = ((uint32_t)port - GPIOA_BASE) / 0x400;
    RCC->AHB1ENR |= (1 << port_index);

    // 2. Set mode to output (01)
    port->MODER &= ~(0x3 << (pin * 2));
    port->MODER |=  (0x1 << (pin * 2));

    // 3. Set output type to push-pull (0)
    port->OTYPER &= ~(1 << pin);

    // 4. Set speed to low (00) - sufficient for LED toggling
    port->OSPEEDR &= ~(0x3 << (pin * 2));

    // 5. No pull-up/pull-down (00)
    port->PUPDR &= ~(0x3 << (pin * 2));
}

void gpio_setup_input(GPIO_TypeDef *port, uint32_t pin, uint32_t pull) {
    uint32_t port_index = ((uint32_t)port - GPIOA_BASE) / 0x400;
    RCC->AHB1ENR |= (1 << port_index);

    // Set mode to input (00)
    port->MODER &= ~(0x3 << (pin * 2));

    // Set pull-up/pull-down
    port->PUPDR &= ~(0x3 << (pin * 2));
    port->PUPDR |=  (pull << (pin * 2));
}
```

### Startup Sequence for GPIO

```
Step-by-step GPIO Initialization
==================================

1. Enable the clock to the GPIO port
   RCC->AHB1ENR |= RCC_AHB1ENR_GPIOxEN;
   (without clock, all register accesses are ignored)

2. Configure pin mode (MODER)
   Input, Output, Alternate Function, or Analog

3. Configure output type (OTYPER) -- if output/AF
   Push-pull or Open-drain

4. Configure speed (OSPEEDR) -- if output/AF
   Low, Medium, Fast, or High

5. Configure pull-up/pull-down (PUPDR)
   None, Pull-up, or Pull-down

6. For AF mode: select alternate function (AFRL/AFRH)
   AF0 through AF15

7. Set initial output value (ODR or BSRR) -- if output
```

---

## Summary

This module covered the foundational concepts of microcontroller architecture and GPIO:

- **Harvard vs Von Neumann**: ARM Cortex-M uses a modified Harvard architecture with
  separate internal buses but a unified address space.
- **ARM Cortex-M**: A 32-bit RISC core with 16 general-purpose registers, Thumb-2
  instruction set, and an integrated NVIC for interrupt handling.
- **Clock Tree**: Multiple clock sources (HSI, HSE, PLL) feed through prescalers to
  provide clock signals to the CPU and peripherals. Peripheral clocks must be explicitly
  enabled via the RCC registers.
- **Memory Map**: A flat 4 GB address space with standardized regions for code, SRAM,
  peripherals, and system components. Peripherals are accessed through memory-mapped I/O.
- **GPIO Registers**: MODER (pin mode), OTYPER (output type), OSPEEDR (speed), PUPDR
  (pull resistors), IDR (input read), ODR (output write), BSRR (atomic set/reset), and
  AFR (alternate function selection).
- **Pin Modes**: Input, output (push-pull/open-drain), alternate function, and analog.
- **Register-Level vs HAL**: Register-level programming provides direct control and
  understanding; HAL provides portability and ease of use.

### Next Steps

- Work through the [examples](examples/) to see these concepts in action
- Complete the [exercises](exercises/) to build hands-on skills
- Take the [quiz](quiz.md) to test your understanding
- Build the [Simon Says project](project/) to apply everything together

---

*Module 03 of the Embedded Systems Learning Platform*
