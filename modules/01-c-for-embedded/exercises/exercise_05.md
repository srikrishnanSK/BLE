# Exercise 05: CRC-8 Calculator

## Objective

Implement a CRC-8 (Cyclic Redundancy Check) calculator suitable for embedded
communication protocols. CRCs detect data corruption in serial links (UART, SPI,
I2C), stored data, and network packets. Understanding CRC math is essential for
embedded engineers.

## Background

A CRC treats the input data as a long polynomial and divides it by a fixed
"generator" polynomial. The remainder of this division is the CRC value.

For CRC-8 with polynomial `0x07` (x^8 + x^2 + x + 1), used in SMBus and
many sensor protocols:

1. Initialize the CRC register (often to `0x00` or `0xFF`)
2. For each input byte:
   a. XOR the byte into the CRC register
   b. For each bit (8 times):
      - If the MSB of CRC is 1: shift left, XOR with polynomial
      - If the MSB of CRC is 0: just shift left

The lookup table method pre-computes step 2b for all 256 possible byte values,
making the per-byte calculation a single XOR and table lookup.

## Task

Implement three versions of CRC-8:

```c
/* Version 1: Bit-by-bit calculation (smallest code, slowest) */
uint8_t crc8_bitwise(const uint8_t *data, uint16_t length);

/* Version 2: Table-driven calculation (256-byte table, fastest) */
void crc8_generate_table(uint8_t table[256], uint8_t polynomial);
uint8_t crc8_table(const uint8_t *data, uint16_t length, const uint8_t table[256]);

/* Version 3: Half-byte (nibble) table (16-byte table, good compromise) */
void crc8_generate_nibble_table(uint8_t table[16], uint8_t polynomial);
uint8_t crc8_nibble(const uint8_t *data, uint16_t length, const uint8_t table[16]);
```

Use polynomial `0x07` (CRC-8/SMBus) with initial value `0x00`.

## Requirements

1. All three methods must produce identical results for the same input.
2. Implement a test that verifies against known CRC-8 values:
   - `"123456789"` should produce CRC = `0xF4` (CRC-8/SMBus standard check)
   - An empty input should produce CRC = `0x00`
   - `{0xFF}` should produce CRC = `0xF1`
3. Print the full 256-entry lookup table in a formatted way.
4. Measure and compare performance of all three methods by running each
   on a 1024-byte buffer multiple times.

## Hints

- MSB check: `if (crc & 0x80)`
- After shifting: `crc = (crc << 1) ^ polynomial` or `crc = crc << 1`
- Table generation: compute the CRC of each single byte value 0x00 through 0xFF
- The nibble table uses the same logic but processes 4 bits at a time
- Use `clock()` from `<time.h>` for basic timing

## Expected Output

```
=== CRC-8 Calculator ===

CRC-8 Lookup Table (polynomial 0x07):
  00 07 0E 09 1C 1B 12 15 38 3F 36 31 24 23 2A 2D
  70 77 7E 79 6C 6B 62 65 48 4F 46 41 54 53 5A 5D
  ...  (16 rows total)

Test vector: "123456789"
  Bitwise:  0xF4 [PASS]
  Table:    0xF4 [PASS]
  Nibble:   0xF4 [PASS]

Test: empty input
  CRC = 0x00 [PASS]

Test: {0xFF}
  CRC = 0xF1 [PASS]

Performance (1024 bytes, 10000 iterations):
  Bitwise: XXX us
  Table:   XXX us
  Nibble:  XXX us
```

## Compilation

```bash
gcc -Wall -Wextra -std=c99 -O2 -o exercise_05 solution_05.c
```
