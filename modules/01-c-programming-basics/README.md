# Module 01: C Programming Basics for Embedded Systems

## Overview

This module introduces the fundamentals of C programming with a focus on embedded
systems development. Unlike desktop C programming, embedded C must deal with limited
memory, no operating system (bare-metal), direct hardware access, and strict control
over data sizes. Every concept in this module is presented through the lens of
embedded targets: STM32 (ARM Cortex-M), MSP430 (16-bit TI), and Arduino (AVR/ARM).

---

## 1. Data Types and Sizes on Embedded Targets

### Why Fixed-Width Types Matter

On a desktop PC, `int` is typically 32 bits. On an MSP430, `int` is 16 bits. On an
8-bit AVR (Arduino Uno), `int` is also 16 bits. Code that assumes `int` is 32 bits
will silently produce wrong results on these platforms.

The solution is to use **fixed-width integer types** from `<stdint.h>`:

| Type        | Size    | Range (signed)                          | Range (unsigned)      |
|-------------|---------|------------------------------------------|-----------------------|
| `int8_t`    | 1 byte  | -128 to 127                              | —                     |
| `uint8_t`   | 1 byte  | —                                        | 0 to 255              |
| `int16_t`   | 2 bytes | -32,768 to 32,767                        | —                     |
| `uint16_t`  | 2 bytes | —                                        | 0 to 65,535           |
| `int32_t`   | 4 bytes | -2,147,483,648 to 2,147,483,647          | —                     |
| `uint32_t`  | 4 bytes | —                                        | 0 to 4,294,967,295    |

### Platform-Specific Sizes

```
Platform      | char | int  | long | pointer
--------------+------+------+------+--------
AVR (Arduino) |  8   |  16  |  32  |   16
MSP430        |  8   |  16  |  32  |   16/20
ARM Cortex-M  |  8   |  32  |  32  |   32
Desktop x86   |  8   |  32  |  32/64| 32/64
```

**Rule of thumb:** Always use `<stdint.h>` types in embedded code to guarantee
data sizes across all targets.

### The `volatile` Qualifier (Preview)

In embedded systems, variables may be modified by hardware or interrupt service
routines. The `volatile` keyword prevents the compiler from optimizing away reads
and writes to such variables. We introduce it here and cover it in depth in Module 02.

```c
volatile uint8_t uart_rx_flag = 0;  /* May be set by UART ISR */
```

---

## 2. Variables and Constants

### Variable Declaration

In embedded C, variables should be declared with the smallest type that fits the data:

```c
uint8_t  led_state = 0;       /* 0 or 1: only needs 1 byte */
uint16_t adc_value = 0;       /* 10-12 bit ADC: needs 16 bits */
int32_t  temperature = 0;     /* Signed temperature in 0.1 degree units */
```

### Constants

Use `const` for values that should not change. The compiler can place these in
flash/ROM, saving precious RAM:

```c
const uint8_t MAX_RETRIES = 3;
const uint16_t BAUD_RATE = 9600;
```

For compile-time constants, `#define` is also common (but lacks type safety):

```c
#define LED_PIN   13
#define BUFFER_SIZE 64
```

### Storage Classes

- `static` — retains value between function calls; limits scope to file if used at file level
- `extern` — declares a variable defined in another file
- `register` — hint to compiler (mostly ignored by modern compilers)

---

## 3. Operators

### Arithmetic Operators

`+`, `-`, `*`, `/`, `%` — standard arithmetic. In embedded systems, note:
- Division and modulo are expensive on CPUs without hardware dividers (MSP430, AVR)
- Use shifts for powers of 2: `x >> 3` instead of `x / 8`
- Use bitwise AND for modulo by powers of 2: `x & 0x0F` instead of `x % 16`

### Relational Operators

`==`, `!=`, `<`, `>`, `<=`, `>=`

**Pitfall:** Comparing signed and unsigned values:
```c
int8_t  a = -1;
uint8_t b = 1;
if (a < b) { /* This may NOT behave as expected! */ }
```
The signed value gets implicitly converted to unsigned, making `-1` become `255`.

### Logical Operators

`&&`, `||`, `!` — short-circuit evaluation. Useful for safe pointer checks:
```c
if (ptr != NULL && ptr->valid) { /* Safe: won't dereference NULL */ }
```

### Bitwise Operators

These are the most important operators in embedded programming:

| Operator | Name       | Use Case                       |
|----------|------------|--------------------------------|
| `&`      | AND        | Clear bits, mask bits          |
| `\|`     | OR         | Set bits                       |
| `^`      | XOR        | Toggle bits                    |
| `~`      | NOT        | Invert all bits                |
| `<<`     | Left shift | Multiply by power of 2         |
| `>>`     | Right shift| Divide by power of 2           |

Common patterns:
```c
reg |=  (1 << bit);   /* Set bit   */
reg &= ~(1 << bit);   /* Clear bit */
reg ^=  (1 << bit);   /* Toggle bit */
if (reg & (1 << bit)) /* Check bit  */
```

---

## 4. Control Flow

### if / else

```c
if (button_pressed()) {
    led_on();
} else {
    led_off();
}
```

### switch-case

Ideal for command parsers, state machines, and menu systems in embedded:

```c
switch (command) {
    case CMD_START:  motor_start(); break;
    case CMD_STOP:   motor_stop();  break;
    case CMD_STATUS: send_status(); break;
    default:         send_error();  break;
}
```

### for loop

```c
for (uint8_t i = 0; i < BUFFER_SIZE; i++) {
    buffer[i] = 0;  /* Clear buffer */
}
```

### while / do-while

```c
/* Wait for hardware flag */
while (!(UART_STATUS & RX_READY)) {
    /* Busy wait */
}

/* Execute at least once, then check */
do {
    value = read_sensor();
} while (value == SENSOR_BUSY);
```

### break / continue

- `break` — exit the innermost loop or switch
- `continue` — skip to the next iteration

---

## 5. Functions

Functions in embedded C should be:
- **Small and focused** — easier to test and optimize
- **Predictable** — avoid deep recursion (limited stack on embedded)
- **Well-prototyped** — always declare before use

```c
/* Prototype (declaration) */
uint16_t adc_read(uint8_t channel);

/* Definition */
uint16_t adc_read(uint8_t channel) {
    /* Select channel, start conversion, wait, return result */
    select_channel(channel);
    start_conversion();
    while (!conversion_done()) { }
    return read_result();
}
```

### Pass by Value

C passes all arguments by value. To modify the caller's data, pass a pointer:

```c
void swap(uint32_t *a, uint32_t *b) {
    uint32_t temp = *a;
    *a = *b;
    *b = temp;
}
```

---

## 6. Arrays

Arrays are contiguous blocks of memory — perfect for buffers, lookup tables, and
data storage in embedded systems:

```c
uint8_t tx_buffer[64];              /* Transmit buffer */
const uint8_t sine_table[256] = {   /* Lookup table in flash */
    128, 131, 134, ...
};
```

**Important:** C does not check array bounds. Writing past the end of an array
corrupts adjacent memory — a common source of embedded bugs.

---

## 7. Strings in C

Strings are null-terminated arrays of `char`:

```c
char message[] = "Hello";  /* Actually 6 bytes: H, e, l, l, o, \0 */
```

In embedded systems, avoid `<string.h>` functions that rely on dynamic memory.
Implement what you need manually or use bounded versions (`strncpy`, `snprintf`).

Common operations to implement yourself:
- String length (count until `\0`)
- String compare (byte by byte)
- String copy (with length limit)

---

## 8. The Compilation Process

Understanding the build process is essential for embedded development:

```
Source (.c)
    |
    v
[Preprocessor] --- expands #include, #define, #ifdef
    |
    v
Translation Unit (.i)
    |
    v
[Compiler] --- translates C to assembly
    |
    v
Assembly (.s)
    |
    v
[Assembler] --- converts assembly to machine code
    |
    v
Object File (.o)
    |
    v
[Linker] --- combines objects, resolves symbols, applies linker script
    |
    v
Executable (.elf)
    |
    v
[objcopy] --- extracts binary image for flashing
    |
    v
Binary (.bin / .hex)
```

### Key Concepts

- **Preprocessor:** Text substitution phase. Handles `#include`, `#define`,
  `#ifdef`. No understanding of C syntax.
- **Compiler:** Translates C into target-specific assembly. Performs optimizations.
- **Assembler:** Converts assembly mnemonics into machine code (object files).
- **Linker:** Combines object files, resolves external references, places code
  and data at specific addresses according to the linker script.
- **Linker script:** Defines memory regions (Flash, RAM) and section placement.
  Critical for embedded targets.

---

## 9. Your First Embedded-Style Program

Even without hardware, you can write code that follows embedded conventions:

```c
#include <stdint.h>
#include <stdio.h>

/* Simulated hardware register */
static volatile uint32_t GPIO_ODR = 0x00000000;

/* Pin definitions */
#define LED_PIN  5
#define SET_BIT(reg, bit)   ((reg) |= (1U << (bit)))
#define CLEAR_BIT(reg, bit) ((reg) &= ~(1U << (bit)))
#define READ_BIT(reg, bit)  (((reg) >> (bit)) & 1U)

void led_on(void)  { SET_BIT(GPIO_ODR, LED_PIN); }
void led_off(void) { CLEAR_BIT(GPIO_ODR, LED_PIN); }

int main(void) {
    led_on();
    printf("LED state: %s\n", READ_BIT(GPIO_ODR, LED_PIN) ? "ON" : "OFF");
    led_off();
    printf("LED state: %s\n", READ_BIT(GPIO_ODR, LED_PIN) ? "ON" : "OFF");
    return 0;
}
```

This style — using macros for bit manipulation, simulated registers, and small
focused functions — directly translates to real hardware code.

---

## Module Contents

- **examples/** — 6 working code examples demonstrating each concept
- **exercises/** — 8 hands-on exercises with problem statements and hints
- **solutions/** — Complete solutions for all exercises
- **tests/** — Automated tests using assert()
- **quiz.md** — 20 multiple-choice questions
- **quiz_answers.md** — Detailed answer explanations
- **project/** — Serial Command Line Calculator capstone project

## Prerequisites

- A C compiler (GCC recommended): `gcc` on Linux/macOS, MinGW on Windows
- Basic command line familiarity
- Text editor or IDE of your choice

## Building Examples

```bash
gcc -Wall -Wextra -std=c99 -o example examples/01_data_types.c
./example
```

## Target Board Notes

| Board    | Compiler         | Notes                                    |
|----------|------------------|------------------------------------------|
| STM32    | arm-none-eabi-gcc| 32-bit ARM, hardware multiply/divide     |
| MSP430   | msp430-elf-gcc   | 16-bit, no hardware multiply on some     |
| Arduino  | avr-gcc / arm-gcc| 8-bit AVR or 32-bit ARM depending on board|
| Host PC  | gcc / clang      | For simulation and testing               |
