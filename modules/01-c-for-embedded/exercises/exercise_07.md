# Exercise 07: Memory-Mapped I/O Simulation

## Objective

Simulate a complete memory-mapped I/O peripheral system, including GPIO, UART,
and Timer peripherals. This exercise teaches you how embedded software interacts
with hardware through memory-mapped registers, using pointers, volatile, and
structs.

## Background

In embedded systems, hardware peripherals are controlled through registers that
appear at fixed memory addresses. The CPU reads and writes these addresses just
like RAM, but the reads and writes trigger hardware actions.

A typical peripheral has a block of registers:

```
UART Base: 0x40004400
  +0x00  SR   (Status Register)      - read-only
  +0x04  DR   (Data Register)        - read/write
  +0x08  BRR  (Baud Rate Register)   - read/write
  +0x0C  CR1  (Control Register 1)   - read/write
  +0x10  CR2  (Control Register 2)   - read/write
```

In C, we model this with a struct and a pointer to the base address:

```c
typedef struct {
    volatile uint32_t SR;
    volatile uint32_t DR;
    volatile uint32_t BRR;
    volatile uint32_t CR1;
    volatile uint32_t CR2;
} UART_TypeDef;

#define UART1  ((UART_TypeDef *)0x40004400)
```

## Task

Create a simulated microcontroller with three peripherals. Use arrays as
backing memory and struct pointers to access them.

### Peripheral 1: GPIO

```c
typedef struct {
    volatile uint32_t MODER;    /* +0x00: Mode register (2 bits per pin) */
    volatile uint32_t OTYPER;   /* +0x04: Output type (1 bit per pin) */
    volatile uint32_t OSPEEDR;  /* +0x08: Speed (2 bits per pin) */
    volatile uint32_t PUPDR;    /* +0x0C: Pull-up/pull-down (2 bits per pin) */
    volatile uint32_t IDR;      /* +0x10: Input data (read-only, 1 bit per pin) */
    volatile uint32_t ODR;      /* +0x14: Output data (1 bit per pin) */
    volatile uint32_t BSRR;     /* +0x18: Bit set/reset (write-only) */
} GPIO_TypeDef;
```

### Peripheral 2: UART (simplified)

```c
typedef struct {
    volatile uint32_t SR;       /* +0x00: Status register */
    volatile uint32_t DR;       /* +0x04: Data register */
    volatile uint32_t BRR;      /* +0x08: Baud rate register */
    volatile uint32_t CR1;      /* +0x0C: Control register */
} UART_TypeDef;
```

### Peripheral 3: Timer (simplified)

```c
typedef struct {
    volatile uint32_t CR1;      /* +0x00: Control register */
    volatile uint32_t SR;       /* +0x04: Status register */
    volatile uint32_t CNT;      /* +0x08: Counter value */
    volatile uint32_t ARR;      /* +0x0C: Auto-reload value */
    volatile uint32_t PSC;      /* +0x10: Prescaler */
} TIM_TypeDef;
```

### Implement these functions:

```c
void gpio_init(GPIO_TypeDef *gpio, uint8_t pin, uint8_t mode);
void gpio_write(GPIO_TypeDef *gpio, uint8_t pin, uint8_t value);
uint8_t gpio_read(const GPIO_TypeDef *gpio, uint8_t pin);

void uart_init(UART_TypeDef *uart, uint32_t baud_rate);
void uart_send_byte(UART_TypeDef *uart, uint8_t data);
void uart_send_string(UART_TypeDef *uart, const char *str);

void timer_init(TIM_TypeDef *timer, uint32_t prescaler, uint32_t period);
void timer_start(TIM_TypeDef *timer);
void timer_stop(TIM_TypeDef *timer);
uint32_t timer_get_count(const TIM_TypeDef *timer);
```

## Requirements

1. All register fields must be `volatile`.
2. GPIO MODER uses 2-bit fields: `00`=input, `01`=output, `10`=alternate, `11`=analog.
3. GPIO BSRR: writing to bits [15:0] sets the corresponding ODR bit; writing
   to bits [31:16] resets the corresponding ODR bit. This is write-only and
   always reads as 0.
4. Print register values after each operation to show the state changes.

## Expected Output

```
=== Memory-Mapped I/O Simulation ===

--- GPIO Init: Pin 5 as Output ---
  MODER: 0x00000400  (pin 5 = output mode)
  ODR:   0x00000000

--- GPIO Write: Pin 5 = 1 ---
  ODR:   0x00000020
  BSRR write: 0x00000020  (set pin 5)

--- GPIO Read: Pin 5 ---
  Value: 1

--- UART Init: 9600 baud ---
  BRR: 0x00001A0B  (assuming 42 MHz clock)
  CR1: 0x0000200C  (UART enabled, TX+RX enabled)

--- UART Send: 'H' ---
  DR: 0x00000048
  SR: 0x00000080  (TXE set)

--- Timer Init ---
  PSC: 0x0000FFFF
  ARR: 0x000003E8
  CR1: 0x00000001  (timer enabled)
```

## Compilation

```bash
gcc -Wall -Wextra -std=c99 -o exercise_07 solution_07.c
```
