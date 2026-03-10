# Exercise 01: Binary Arithmetic

## Objective

Implement functions that perform binary addition and subtraction on unsigned
and signed (two's complement) integers, with overflow and underflow detection.
Work at the bit level to reinforce how the ALU inside a processor operates.

## Background

In embedded systems, understanding binary arithmetic is not academic -- it is
practical.  You will routinely encounter:

- Register values that overflow silently (C unsigned wrap-around)
- Signed overflow causing undefined behavior in C
- Carry and borrow flags that hardware exposes but C hides from you

## Requirements

Write a C program `solution_01.c` that implements and tests:

### Part A -- Unsigned 8-bit Addition

```c
typedef struct {
    uint8_t result;
    bool    carry;     /* true if result overflowed (>255) */
} add_u8_result_t;

add_u8_result_t add_u8(uint8_t a, uint8_t b);
```

### Part B -- Unsigned 8-bit Subtraction

```c
typedef struct {
    uint8_t result;
    bool    borrow;    /* true if a < b (underflow) */
} sub_u8_result_t;

sub_u8_result_t sub_u8(uint8_t a, uint8_t b);
```

### Part C -- Signed 8-bit Addition with Overflow Detection

```c
typedef struct {
    int8_t  result;
    bool    overflow;  /* true if signed overflow occurred */
} add_s8_result_t;

add_s8_result_t add_s8(int8_t a, int8_t b);
```

Signed overflow occurs when:
- Two positive numbers produce a negative result, or
- Two negative numbers produce a positive result

### Part D -- Multi-byte Addition

```c
/**
 * Add two 32-bit numbers stored as arrays of 4 bytes (little-endian),
 * propagating carries from byte to byte -- the way a CPU without a
 * 32-bit ALU would do it.
 */
typedef struct {
    uint8_t result[4];
    bool    carry;
} add_u32_bytes_result_t;

add_u32_bytes_result_t add_u32_bytes(const uint8_t a[4], const uint8_t b[4]);
```

## Test Cases

Your program must print results for at least these cases:

| Operation           | A      | B      | Expected Result | Flag      |
|---------------------|--------|--------|-----------------|-----------|
| add_u8              | 200    | 100    | 44              | carry=1   |
| add_u8              | 100    | 50     | 150             | carry=0   |
| sub_u8              | 50     | 30     | 20              | borrow=0  |
| sub_u8              | 30     | 50     | 236             | borrow=1  |
| add_s8              | 100    | 50     | -106            | overflow=1|
| add_s8              | -100   | -50    | 106             | overflow=1|
| add_s8              | 50     | 30     | 80              | overflow=0|
| add_s8              | -50    | 30     | -20             | overflow=0|
| add_u32_bytes       | 0x0000FFFF | 0x00000001 | 0x00010000 | carry=0 |

## Hints

1. For unsigned carry detection: cast operands to `uint16_t`, add, then check
   bit 8.
2. For signed overflow, use the rule: overflow = (sign_a == sign_b) &&
   (sign_result != sign_a).
3. For multi-byte: process byte 0 first, capture carry, feed it into byte 1,
   and so on.

## Deliverable

A single C file `solution_01.c` that compiles with:
```
gcc -Wall -Wextra -std=c99 -o solution_01 solution_01.c
```
