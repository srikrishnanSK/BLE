# Exercise 01: Bitfield Extraction

## Objective

Learn to extract and manipulate bitfields from hardware registers using
bitwise operators. This is one of the most common tasks in embedded programming:
reading configuration fields, status flags, and data from memory-mapped registers.

## Background

Hardware registers pack multiple pieces of information into a single word. For
example, a 32-bit status register might contain:

```
Bits [31:24]  Device ID        (8 bits, read-only)
Bits [23:20]  Hardware revision (4 bits, read-only)
Bits [19:16]  Error code        (4 bits)
Bits [15:8]   Data count        (8 bits)
Bit  [7]      Overflow flag     (1 bit)
Bit  [6]      Underflow flag    (1 bit)
Bits [5:4]    Operating mode    (2 bits)
Bits [3:0]    Channel select    (4 bits)
```

To extract a field, you **shift right** to move the field to bit 0, then **mask**
off the bits you want.

Formula: `value = (register >> shift) & mask`

Where `mask = (1 << width) - 1` creates a mask of `width` ones.

## Task

Write a program that:

1. Defines the register layout above using `#define` macros for each field's
   shift amount and mask.

2. Implements these functions:

```c
/* Extract a field from a register value */
uint32_t extract_field(uint32_t reg, uint8_t shift, uint32_t mask);

/* Insert a value into a field of a register */
uint32_t insert_field(uint32_t reg, uint8_t shift, uint32_t mask, uint32_t value);

/* Extract each named field from the status register */
void decode_status_register(uint32_t reg);
```

3. Given the register value `0xA3F5_40B7`, decode and print every field.

4. Modify the register to:
   - Change the operating mode to `0x3`
   - Clear the overflow flag
   - Set the channel select to `0xC`
   Print the modified register value.

## Hints

- A mask of N ones: `(1U << N) - 1`. For example, 4 ones = `0xF`.
- To clear a field before inserting: `reg &= ~(mask << shift)`
- To insert: `reg |= ((value & mask) << shift)`
- Use `uint32_t` for all register values to match 32-bit hardware registers.
- Use `1U` (unsigned) in shift expressions to avoid signed overflow.

## Expected Output

```
Status Register: 0xA3F540B7
  Device ID:        0xA3
  Hardware Rev:     0xF
  Error Code:       0x5
  Data Count:       0x40
  Overflow:         1
  Underflow:        0
  Operating Mode:   0x3
  Channel Select:   0x7

Modified Register: 0xA3F540BC  (changed mode, cleared overflow, set channel)
```

Note: Your exact modified value may differ depending on the order of operations.
Verify each field individually.

## Compilation

```bash
gcc -Wall -Wextra -std=c99 -o exercise_01 solution_01.c
```
