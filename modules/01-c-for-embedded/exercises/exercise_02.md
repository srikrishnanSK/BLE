# Exercise 02: Endianness Conversion

## Objective

Understand byte ordering (endianness) and implement conversion functions. This is
critical when communicating between processors with different byte orders, reading
binary file formats, or handling network protocols on embedded systems.

## Background

**Little-endian** stores the least significant byte at the lowest memory address.
ARM Cortex-M, x86, and most modern processors are little-endian.

**Big-endian** stores the most significant byte at the lowest memory address.
Network protocols (TCP/IP), many file formats (JPEG, PNG), and some
microcontrollers (Motorola 68K, some PIC) use big-endian.

Example for `0x12345678`:

```
Address:    0x00   0x01   0x02   0x03
Little-E:   0x78   0x56   0x34   0x12   (LSB first)
Big-E:      0x12   0x34   0x56   0x78   (MSB first)
```

## Task

Write a program that:

1. **Detects** the endianness of the host machine at runtime using a union or
   pointer cast.

2. **Implements** these byte-swap functions WITHOUT using library functions:

```c
/* Swap bytes of a 16-bit value */
uint16_t swap16(uint16_t val);

/* Swap bytes of a 32-bit value */
uint32_t swap32(uint32_t val);

/* Convert host byte order to big-endian (network order) */
uint16_t host_to_be16(uint16_t val);
uint32_t host_to_be32(uint32_t val);

/* Convert big-endian to host byte order */
uint16_t be_to_host16(uint16_t val);
uint32_t be_to_host32(uint32_t val);
```

3. **Demonstrates** converting a sensor data packet. Suppose a sensor sends
   4 bytes in big-endian order representing a 32-bit temperature reading.
   Parse the bytes `{0x00, 0x01, 0xC2, 0x08}` into a host-order `uint32_t`.

4. **Prints** the byte-level representation of a `uint32_t` to show how the
   value is stored in memory.

## Hints

- To swap 16-bit: `(val << 8) | (val >> 8)`
- To swap 32-bit, swap pairs of bytes: shift and mask each of the 4 bytes into
  its new position.
- To detect endianness: store `0x01` in a `uint16_t` and check if the first
  byte (via a `uint8_t` pointer) is `0x01` (little-endian) or `0x00` (big-endian).
- A round-trip conversion (host -> big-endian -> host) must return the original value.

## Expected Output (on a little-endian machine)

```
Host byte order: Little-Endian

swap16(0x1234) = 0x3412
swap32(0x12345678) = 0x78563412

Sensor bytes (big-endian): 00 01 C2 08
Parsed value (host order): 0x0001C208 = 115208

Memory layout of 0x12345678:
  Address+0: 0x78
  Address+1: 0x56
  Address+2: 0x34
  Address+3: 0x12

Round-trip check: 0x12345678 -> BE -> host = 0x12345678 [PASS]
```

## Compilation

```bash
gcc -Wall -Wextra -std=c99 -o exercise_02 solution_02.c
```
