# Module 06: Digital Electronics & Number Systems

## Overview

This module covers the foundational digital electronics concepts that every embedded systems
engineer must master. We start with number systems and representations used by processors,
move through Boolean algebra and logic design, and finish with practical hardware interface
topics like debouncing and signal levels.

## Prerequisites

- Module 01-05 (C programming fundamentals)
- Basic understanding of voltage and current

---

## 1. Number Systems and Conversions

### 1.1 Binary (Base-2)

Computers operate in binary. Each digit (bit) is either 0 or 1.

| Decimal | Binary (8-bit) | Hex  | Octal |
|---------|----------------|------|-------|
| 0       | 0000 0000      | 0x00 | 000   |
| 1       | 0000 0001      | 0x01 | 001   |
| 10      | 0000 1010      | 0x0A | 012   |
| 127     | 0111 1111      | 0x7F | 177   |
| 255     | 1111 1111      | 0xFF | 377   |

**Position values (8-bit):**
```
Bit:    7    6    5    4    3    2    1    0
Value: 128   64   32   16    8    4    2    1
```

**Conversion: Decimal to Binary (Repeated Division)**
```
Convert 42 to binary:
42 / 2 = 21 remainder 0  (LSB)
21 / 2 = 10 remainder 1
10 / 2 =  5 remainder 0
 5 / 2 =  2 remainder 1
 2 / 2 =  1 remainder 0
 1 / 2 =  0 remainder 1  (MSB)
Result: 101010
```

### 1.2 Hexadecimal (Base-16)

Hex digits: 0-9, A-F. Each hex digit represents exactly 4 bits (a nibble).

```
Binary:  1010 1111 0011 1100
Hex:        A    F    3    C  = 0xAF3C
```

This is why hex is so useful in embedded systems: it maps directly to binary
and makes register values readable.

### 1.3 Octal (Base-8)

Each octal digit represents 3 bits. Used in Unix permissions but rarely in
embedded work.

```
Binary: 001 101 110
Octal:    1   5   6  = 0156
```

### 1.4 Binary Arithmetic

**Addition:**
```
  0 + 0 = 0
  0 + 1 = 1
  1 + 0 = 1
  1 + 1 = 10 (0, carry 1)
```

**Example: 0110 1011 + 0011 1101**
```
    0110 1011   (107)
  + 0011 1101   ( 61)
  -----------
    1010 1000   (168)
```

**Overflow** occurs when the result exceeds the number of available bits. For
unsigned 8-bit, any result >= 256 overflows.

---

## 2. Two's Complement Representation

### 2.1 Concept

Two's complement is how processors represent signed integers. The MSB is the
sign bit: 0 = positive, 1 = negative.

**Range for N-bit two's complement:** -(2^(N-1)) to +(2^(N-1) - 1)
- 8-bit:  -128 to +127
- 16-bit: -32768 to +32767
- 32-bit: -2,147,483,648 to +2,147,483,647

### 2.2 Computing Two's Complement (Negation)

1. Invert all bits (one's complement)
2. Add 1

```
+5  = 0000 0101
      --------- invert
      1111 1010
      --------- add 1
-5  = 1111 1011
```

### 2.3 Why Two's Complement?

- Addition works the same for signed and unsigned
- Only one representation of zero (unlike sign-magnitude or one's complement)
- Hardware is simpler: subtraction is just add-the-negation

```
 5 + (-3) using two's complement:
  0000 0101  (+5)
+ 1111 1101  (-3)
-----------
  0000 0010  (+2)  (carry out is discarded)
```

---

## 3. IEEE 754 Floating Point

### 3.1 Single Precision (32-bit)

```
| Sign (1 bit) | Exponent (8 bits) | Mantissa (23 bits) |
```

**Value = (-1)^sign x 1.mantissa x 2^(exponent - 127)**

Example: Representing 6.75
```
6.75 = 110.11 in binary = 1.1011 x 2^2
Sign = 0 (positive)
Exponent = 2 + 127 = 129 = 1000 0001
Mantissa = 1011 0000 0000 0000 0000 000
Result: 0 10000001 10110000000000000000000 = 0x40D80000
```

### 3.2 Special Values

| Value             | Sign | Exponent | Mantissa   |
|-------------------|------|----------|------------|
| +0                | 0    | 00000000 | all zeros  |
| -0                | 1    | 00000000 | all zeros  |
| +Infinity         | 0    | 11111111 | all zeros  |
| -Infinity         | 1    | 11111111 | all zeros  |
| NaN               | X    | 11111111 | non-zero   |
| Denormalized      | X    | 00000000 | non-zero   |

### 3.3 Floating Point Pitfalls in Embedded

- **No FPU?** Software floating point is very slow (ARM Cortex-M0 has no FPU)
- **Precision errors:** 0.1 + 0.2 != 0.3 (use fixed-point instead)
- **Comparison:** Never use `==` with floats; use an epsilon threshold
- **Determinism:** Floating point results may vary by compiler optimization level

---

## 4. Boolean Algebra and Logic Gates

### 4.1 Basic Gates

| Gate | Symbol | Truth Table                    | C Operator |
|------|--------|--------------------------------|------------|
| AND  | &      | 0&0=0, 0&1=0, 1&0=0, 1&1=1   | `&`        |
| OR   | \|     | 0\|0=0, 0\|1=1, 1\|0=1, 1\|1=1 | `\|`     |
| NOT  | ~      | ~0=1, ~1=0                     | `~`        |
| NAND | ~&     | NOT(AND)                       | `~(a&b)`  |
| NOR  | ~\|    | NOT(OR)                        | `~(a\|b)` |
| XOR  | ^      | 0^0=0, 0^1=1, 1^0=1, 1^1=0   | `^`        |
| XNOR | ~^     | NOT(XOR)                       | `~(a^b)`  |

### 4.2 Boolean Laws

```
Identity:       A & 1 = A          A | 0 = A
Null:           A & 0 = 0          A | 1 = 1
Idempotent:     A & A = A          A | A = A
Complement:     A & ~A = 0         A | ~A = 1
Involution:     ~~A = A
Commutative:    A & B = B & A      A | B = B | A
Associative:    (A&B)&C = A&(B&C)  (A|B)|C = A|(B|C)
Distributive:   A&(B|C) = (A&B)|(A&C)
                A|(B&C) = (A|B)&(A|C)
De Morgan's:    ~(A&B) = ~A | ~B
                ~(A|B) = ~A & ~B
Absorption:     A | (A & B) = A
                A & (A | B) = A
```

### 4.3 De Morgan's Theorem in Practice

De Morgan's is critical for understanding how to simplify logic and build
gates from NAND or NOR only (both are universal gates).

```c
/* These are equivalent: */
if (!(a && b))  ...   /* same as */  if (!a || !b) ...
if (!(a || b))  ...   /* same as */  if (!a && !b) ...
```

---

## 5. Truth Tables

A truth table lists all possible input combinations and their outputs.

**Example: 2-input multiplexer**
```
SEL | A | B | OUT
----|---|---|----
 0  | 0 | 0 |  0
 0  | 0 | 1 |  0
 0  | 1 | 0 |  1
 0  | 1 | 1 |  1
 1  | 0 | 0 |  0
 1  | 0 | 1 |  1
 1  | 1 | 0 |  0
 1  | 1 | 1 |  1

OUT = (NOT(SEL) AND A) OR (SEL AND B)
```

---

## 6. Karnaugh Maps (K-Maps)

K-maps provide a visual method to simplify Boolean expressions by grouping
adjacent 1s. Adjacent cells differ by only one variable (Gray code ordering).

### 6.1 2-Variable K-Map

```
        B=0   B=1
A=0  |  m0  |  m1  |
A=1  |  m2  |  m3  |
```

### 6.2 3-Variable K-Map

```
          BC=00  BC=01  BC=11  BC=10
A=0    |  m0  |  m1  |  m3  |  m2  |
A=1    |  m4  |  m5  |  m7  |  m6  |
```

Note the Gray code ordering: 00, 01, 11, 10 (not 00, 01, 10, 11).

### 6.3 4-Variable K-Map

```
          CD=00  CD=01  CD=11  CD=10
AB=00  |  m0  |  m1  |  m3  |  m2  |
AB=01  |  m4  |  m5  |  m7  |  m6  |
AB=11  | m12  | m13  | m15  | m14  |
AB=10  |  m8  |  m9  | m11  | m10  |
```

### 6.4 Grouping Rules

1. Groups must contain 1, 2, 4, 8, or 16 cells (powers of 2)
2. Groups must be rectangular
3. Groups can wrap around edges
4. Every 1 must be covered
5. Larger groups = simpler expression
6. Overlapping groups are allowed

### 6.5 Example

```
F(A,B,C,D) = sum(0,1,2,5,8,9,10)

          CD=00  CD=01  CD=11  CD=10
AB=00  |  1   |  1   |  0   |  1   |
AB=01  |  0   |  1   |  0   |  0   |
AB=10  |  1   |  1   |  0   |  1   |
AB=11  |  0   |  0   |  0   |  0   |

Groups:
- {m0,m1,m8,m9}: ~B & ~D simplified to: B'D (actually B'C')
  Wait, let me be precise:
  m0(0000), m1(0001), m8(1000), m9(1001) -> varies: A,D; constant: B=0,C=0
  => B'C'
- {m0,m2,m8,m10}: m0(0000), m2(0010), m8(1000), m10(1010) -> B=0,D=0
  => B'D'
- {m1,m5}: m1(0001), m5(0101) -> A=0,C=0,D=1
  => A'C'D

F = B'C' + B'D' + A'C'D
```

---

## 7. Combinational Circuits

### 7.1 Multiplexer (MUX)

Selects one of many inputs to pass to output. A 2:1 MUX has 2 data inputs,
1 select, and 1 output.

```
OUT = (SEL == 0) ? A : B
    = (~SEL & A) | (SEL & B)
```

An N-to-1 MUX needs ceil(log2(N)) select lines.

### 7.2 Demultiplexer (DEMUX)

Routes one input to one of many outputs based on select lines.

### 7.3 Encoder

2^N inputs to N outputs. Priority encoder outputs the highest active input.

### 7.4 Decoder

N inputs to 2^N outputs. Only one output is active at a time.
Used for: memory address decoding, 7-segment display driving.

---

## 8. Sequential Circuits

### 8.1 Latches

**SR Latch:** Set-Reset, level-triggered
```
S=1, R=0 -> Q=1 (Set)
S=0, R=1 -> Q=0 (Reset)
S=0, R=0 -> Q=Q (Hold)
S=1, R=1 -> Invalid
```

**D Latch:** Data latch, level-triggered. Q follows D when Enable is high.

### 8.2 Flip-Flops

Edge-triggered (rising or falling edge of clock).

**D Flip-Flop:** Captures D on clock edge. Foundation of registers.

**JK Flip-Flop:** Like SR but J=1,K=1 toggles output.

**T Flip-Flop:** Toggle on clock edge when T=1. Used for counters.

### 8.3 Counters

- **Ripple counter:** Each flip-flop clocks the next (slower, simpler)
- **Synchronous counter:** All flip-flops share clock (faster, uses more logic)
- **Up/down counter:** Direction controlled by input signal

### 8.4 Timing Diagrams

Timing diagrams show signal values over time. Critical concepts:
- **Setup time (tsu):** Data must be stable before clock edge
- **Hold time (th):** Data must remain stable after clock edge
- **Propagation delay (tp):** Time from input change to output change
- **Clock-to-Q delay (tcq):** Time from clock edge to output change

---

## 9. Pull-Up and Pull-Down Resistors

### 9.1 Why They're Needed

An unconnected digital input floats between logic levels, causing unpredictable
behavior. Pull-up/pull-down resistors define a default logic level.

### 9.2 Pull-Up Resistor

```
VCC ----[R]----+---- Input Pin
               |
           [Switch]
               |
GND -----------+

Switch open: Input = HIGH (pulled up to VCC)
Switch closed: Input = LOW (connected to GND)
```

Typical values: 4.7k-10k ohms. Internal pull-ups on STM32 are ~40k.

### 9.3 Pull-Down Resistor

```
VCC -----------+
               |
           [Switch]
               |
GND ----[R]----+---- Input Pin

Switch open: Input = LOW (pulled down to GND)
Switch closed: Input = HIGH (connected to VCC)
```

### 9.4 I2C Pull-Ups

I2C requires external pull-up resistors (2.2k-4.7k) because SDA and SCL are
open-drain outputs that can only pull low. Pull-ups provide the high state.

---

## 10. Voltage Dividers

### 10.1 Basic Formula

```
VCC ----[R1]----+---- Vout
                |
               [R2]
                |
GND ------------+

Vout = VCC * R2 / (R1 + R2)
```

### 10.2 Applications in Embedded

- **Level shifting:** 5V to 3.3V (R1=1.8k, R2=3.3k gives ~3.24V)
- **ADC input scaling:** Measure a 12V battery with a 3.3V ADC
- **Sensor biasing:** Set operating point for analog sensors

### 10.3 Loading Effects

If you connect a load (like an ADC input) with finite impedance, the voltage
divider output changes. Use buffer op-amp or ensure load impedance >> R2.

---

## 11. Debouncing

### 11.1 The Problem

Mechanical switches bounce: when pressed or released, the contact bounces
multiple times over 1-20ms, causing multiple edges.

```
Ideal:    ______|‾‾‾‾‾‾
Actual:   ______|‾|_|‾|_|‾‾‾‾
               ^ bouncing
```

### 11.2 Hardware Debouncing

**RC Filter + Schmitt Trigger:**
```
Switch ----[R]----+---- Schmitt Trigger ---- Clean Output
                  |
                 [C]
                  |
GND ---------------

Time constant = R * C
Typical: R=10k, C=100nF -> RC = 1ms
```

### 11.3 Software Debouncing

**Simple delay approach:**
```c
if (button_pressed()) {
    delay_ms(20);          /* Wait for bouncing to stop */
    if (button_pressed()) {
        /* Confirmed press */
    }
}
```

**Timer-based approach (non-blocking):**
```c
#define DEBOUNCE_MS 20

static uint32_t last_change_time = 0;
static uint8_t stable_state = 0;
static uint8_t last_raw = 0;

uint8_t debounce_read(uint8_t raw_input) {
    if (raw_input != last_raw) {
        last_change_time = get_tick_ms();
        last_raw = raw_input;
    }
    if ((get_tick_ms() - last_change_time) >= DEBOUNCE_MS) {
        stable_state = last_raw;
    }
    return stable_state;
}
```

**Shift register approach:**
```c
/* Sample button every 5ms, shift into register */
static uint8_t shift_reg = 0x00;

uint8_t debounce_shift(uint8_t raw) {
    shift_reg = (shift_reg << 1) | (raw & 1);
    if (shift_reg == 0xFF) return 1;  /* 8 consecutive highs */
    if (shift_reg == 0x00) return 0;  /* 8 consecutive lows */
    return stable_state;              /* Still bouncing */
}
```

---

## 12. Signal Levels

### 12.1 TTL (Transistor-Transistor Logic)

| Parameter         | Value |
|-------------------|-------|
| VCC               | 5V    |
| Logic LOW input   | 0 - 0.8V |
| Logic HIGH input  | 2.0 - 5.0V |
| Logic LOW output  | 0 - 0.4V |
| Logic HIGH output | 2.4 - 5.0V |

### 12.2 CMOS (Complementary Metal-Oxide Semiconductor)

| Parameter         | 3.3V CMOS    | 5V CMOS      |
|-------------------|--------------|---------------|
| VCC               | 3.3V         | 5.0V          |
| Logic LOW input   | 0 - 0.99V   | 0 - 1.5V     |
| Logic HIGH input  | 2.31 - 3.3V | 3.5 - 5.0V   |
| Logic LOW output  | 0 - 0.4V    | 0 - 0.5V     |
| Logic HIGH output | 2.9 - 3.3V  | 4.5 - 5.0V   |

### 12.3 3.3V vs 5V Interfacing

**5V output -> 3.3V input:** DANGER! Can damage 3.3V devices.
Solutions:
- Voltage divider (R1=1.8k, R2=3.3k)
- Level shifter IC (TXB0108, BSS138-based)
- Series resistor + clamp diode

**3.3V output -> 5V input:**
- Many 5V CMOS inputs accept 3.3V as HIGH (threshold ~2.5V)
- 5V TTL always accepts 3.3V as HIGH (threshold = 2.0V)
- If not: use level shifter or open-drain output with pull-up to 5V

### 12.4 GPIO Voltage Tolerance

| Board            | I/O Voltage | 5V Tolerant? |
|------------------|-------------|--------------|
| STM32F4 Discovery| 3.3V        | Most pins    |
| MSP430 LaunchPad | 3.3V        | No           |
| Arduino Uno      | 5.0V        | N/A          |
| Arduino Due      | 3.3V        | No           |

---

## Key Takeaways

1. Master hex-to-binary conversion -- you will read register values constantly
2. Understand two's complement for signed integer behavior and overflow
3. Use fixed-point math instead of floating point on MCUs without FPU
4. De Morgan's theorem is essential for simplifying conditional logic
5. Always debounce mechanical inputs (20ms minimum)
6. Never connect 5V signals to 3.3V inputs without level shifting
7. Pull-up/pull-down resistors prevent floating inputs
8. Voltage dividers are your friend but beware of loading effects

---

## Further Reading

- "Digital Design and Computer Architecture" by Harris & Harris
- "The Art of Electronics" by Horowitz & Hill (Chapters 1, 8, 10)
- ARM Cortex-M Technical Reference Manual (signal timing)
- STM32 Reference Manual (GPIO electrical characteristics)
