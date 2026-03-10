# Exercise 06: Parity Bit Calculator

## Objective

Implement even and odd parity computation, verification, and application
to serial data framing.  Parity is the simplest error-detection code and
appears in UART, memory (ECC RAM), and bus protocols.

## Background

A parity bit is a single bit added to a data word so that the total number
of 1-bits (including the parity bit) is either always even or always odd.

- **Even parity:** total number of 1s (data + parity) is even
- **Odd parity:** total number of 1s (data + parity) is odd

Parity detects all single-bit errors but cannot detect two-bit errors or
determine which bit is wrong (for that, you need Hamming codes).

### UART Example

A UART frame with 8 data bits and even parity:
```
[START] [D0] [D1] [D2] [D3] [D4] [D5] [D6] [D7] [PARITY] [STOP]
```

## Requirements

### Part A -- Parity Calculation

```c
/**
 * Count the number of 1-bits in a value (population count / Hamming weight).
 */
int popcount(uint32_t value);

/**
 * Compute even parity bit for a data byte.
 * Returns 0 or 1 such that (popcount(data) + parity) is even.
 */
uint8_t even_parity(uint8_t data);

/**
 * Compute odd parity bit for a data byte.
 * Returns 0 or 1 such that (popcount(data) + parity) is odd.
 */
uint8_t odd_parity(uint8_t data);
```

### Part B -- Parity Verification

```c
/**
 * Check if a received byte + parity bit has valid even parity.
 * @param data     The received data byte
 * @param parity   The received parity bit (0 or 1)
 * @return         true if parity is correct
 */
bool check_even_parity(uint8_t data, uint8_t parity);

bool check_odd_parity(uint8_t data, uint8_t parity);
```

### Part C -- UART Frame Simulator

Simulate sending and receiving bytes with parity:

```c
typedef struct {
    uint8_t data;
    uint8_t parity;
    bool    valid;       /* Set by receiver after parity check */
} uart_frame_t;

/**
 * Create a UART frame with even parity.
 */
uart_frame_t uart_frame_create(uint8_t data);

/**
 * Simulate a parity check on a received frame.
 * Optionally inject a bit error for testing.
 */
bool uart_frame_verify(uart_frame_t *frame);
```

### Part D -- Multi-Byte Block Parity

Extend parity to a block of bytes.  Compute a longitudinal parity byte
where each bit is the parity of the corresponding bit position across
all bytes in the block.

```c
/**
 * Compute longitudinal (column) parity for a data block.
 * @param data     Array of data bytes
 * @param length   Number of bytes
 * @return         Parity byte (XOR of all data bytes)
 */
uint8_t longitudinal_parity(const uint8_t *data, size_t length);
```

### Part E -- Error Injection and Detection Test

1. Send 10 bytes with correct parity -- verify all pass
2. Flip one bit in one byte -- verify that byte fails parity check
3. Flip two bits in one byte -- show that parity fails to detect it
4. Use longitudinal parity to detect single-byte errors in a block

## Parity Truth Table (for reference)

| Data Byte | 1-count | Even Parity | Odd Parity |
|-----------|---------|-------------|------------|
| 0x00      | 0       | 0           | 1          |
| 0x01      | 1       | 1           | 0          |
| 0x03      | 2       | 0           | 1          |
| 0x07      | 3       | 1           | 0          |
| 0x0F      | 4       | 0           | 1          |
| 0xFF      | 8       | 0           | 1          |
| 0xA5      | 4       | 0           | 1          |
| 0x55      | 4       | 0           | 1          |

## Hints

1. The fastest way to compute parity of a byte in software:
   ```c
   uint8_t parity = data;
   parity ^= parity >> 4;
   parity ^= parity >> 2;
   parity ^= parity >> 1;
   parity &= 1;
   ```
   This gives even parity (0 if even number of 1-bits).

2. Odd parity = NOT(even parity) = even_parity ^ 1.

3. Longitudinal parity is simply the XOR of all bytes in the block.

4. For UART simulation, represent the frame as a struct and flip bits
   using XOR: `data ^= (1 << bit_position)`.

## Deliverable

A single C file `solution_06.c`.
