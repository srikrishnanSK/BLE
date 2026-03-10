# Exercise 06: Bit-Banded Register Access

## Objective

Simulate ARM Cortex-M bit-banding, a hardware feature that maps each bit in a
peripheral register to a unique 32-bit word address. This lets you set or clear
individual bits with a single atomic write, without a read-modify-write cycle.

## Background

On ARM Cortex-M3/M4, two memory regions support bit-banding:

- **Peripheral bit-band region:** `0x4000_0000` to `0x400F_FFFF` (1 MB)
- **Peripheral bit-band alias:** `0x4200_0000` to `0x43FF_FFFF` (32 MB)

The formula to compute the alias address for bit `n` of a byte at address `A`:

```
alias_addr = alias_base + (A - region_base) * 32 + n * 4
```

Writing `1` to the alias address sets the bit. Writing `0` clears it. This is
atomic -- no read-modify-write needed, so no interrupt race conditions.

Since we cannot access real hardware in this exercise, we will **simulate**
bit-banding using arrays and pointer arithmetic.

## Task

```c
/* Simulated memory region (represents peripheral registers) */
#define PERIPH_BASE       0x40000000UL
#define PERIPH_BB_BASE    0x42000000UL

/* Simulated register file: 256 bytes of "peripheral memory" */
static uint8_t periph_mem[256];

/* Calculate the bit-band alias offset for a given register offset and bit */
uint32_t bitband_alias(uint32_t reg_offset, uint8_t bit_number);

/* Set a bit using simulated bit-banding */
void bitband_set(uint8_t *mem, uint32_t reg_offset, uint8_t bit);

/* Clear a bit using simulated bit-banding */
void bitband_clear(uint8_t *mem, uint32_t reg_offset, uint8_t bit);

/* Read a bit using simulated bit-banding */
uint8_t bitband_read(const uint8_t *mem, uint32_t reg_offset, uint8_t bit);
```

Also implement traditional read-modify-write (RMW) access for comparison:

```c
/* Traditional read-modify-write: set bit */
void rmw_set_bit(uint8_t *mem, uint32_t reg_offset, uint8_t bit);

/* Traditional read-modify-write: clear bit */
void rmw_clear_bit(uint8_t *mem, uint32_t reg_offset, uint8_t bit);
```

## Requirements

1. Implement the bit-band alias address calculation matching the ARM formula.
2. The simulation must produce the same results as RMW for all operations.
3. Create a GPIO simulation that uses bit-banding:
   - Define `GPIO_ODR` at offset `0x14` (output data register)
   - Define `GPIO_IDR` at offset `0x10` (input data register)
   - Toggle an LED pin using bit-banding
   - Read a button pin using bit-banding
4. Print the alias address calculations to verify correctness.

## Hints

- In simulation, map the alias offset back to byte and bit:
  `byte_offset = alias_offset / 32`, `bit = (alias_offset % 32) / 4`
- The alias region is 32x larger than the base region because each bit gets
  a 4-byte word.
- Real bit-banding only works for the specific memory regions defined by the
  Cortex-M architecture.

## Expected Output

```
=== Bit-Band Simulation ===

Alias address for reg_offset=0x14, bit=5:
  Base addr:  0x40000014
  Alias addr: 0x42000294
  Alias offset: 0x294  (byte 0x14 * 32 + bit 5 * 4)

GPIO ODR test (bit-banded):
  Initial ODR: 0x00
  Set pin 5:   0x20
  Set pin 0:   0x21
  Clear pin 5: 0x01
  Toggle pin 0 (read + flip + write): 0x00

Verification: bit-band matches RMW for all 8 bits [PASS]
```

## Compilation

```bash
gcc -Wall -Wextra -std=c99 -o exercise_06 solution_06.c
```
