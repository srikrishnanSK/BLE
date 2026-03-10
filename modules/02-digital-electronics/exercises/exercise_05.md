# Exercise 05: Gray Code Converter

## Objective

Implement bidirectional Gray code conversion with applications in rotary
encoders and error-resistant data transmission.

## Background

Gray code (also called reflected binary code) changes only one bit between
successive values.  This property makes it essential for:

- **Rotary encoders:** avoid momentary wrong readings during transitions
- **ADC design:** reduce conversion errors
- **K-map layout:** adjacent cells differ by one variable
- **Error detection:** a single-bit error can only change the value by 1

The Gray code sequence for 3 bits:
```
Decimal  Binary   Gray
  0       000      000
  1       001      001
  2       010      011
  3       011      010
  4       100      110
  5       101      111
  6       110      101
  7       111      100
```

## Requirements

### Part A -- Basic Conversion

```c
/* Convert binary to Gray code */
uint32_t binary_to_gray(uint32_t binary);

/* Convert Gray code to binary */
uint32_t gray_to_binary(uint32_t gray);
```

### Part B -- N-bit Gray Code Sequence Generator

```c
/**
 * Generate and print the complete Gray code sequence for N bits.
 * Must handle N = 1 through 8.
 */
void print_gray_sequence(int num_bits);
```

### Part C -- Rotary Encoder Decoder

Simulate a rotary encoder that outputs 2-bit Gray code on pins A and B.
Decode the rotation direction and step count.

```c
typedef struct {
    uint8_t  prev_gray;     /* Previous 2-bit Gray code reading */
    int32_t  position;      /* Accumulated position counter     */
    int8_t   direction;     /* +1 = CW, -1 = CCW, 0 = no move  */
} encoder_t;

void encoder_init(encoder_t *enc);

/**
 * Process a new 2-bit Gray code reading from the encoder.
 * @param enc   Encoder state
 * @param gray  2-bit Gray code (bits 0-1)
 * @return      Direction of movement: +1, -1, or 0
 */
int8_t encoder_update(encoder_t *enc, uint8_t gray);
```

The valid transitions for a quadrature encoder are:
```
CW:  00 -> 01 -> 11 -> 10 -> 00
CCW: 00 -> 10 -> 11 -> 01 -> 00
```

### Part D -- Verify the Single-Bit-Change Property

Write a function that verifies the Gray code property: consecutive codes
differ in exactly one bit position.

```c
/**
 * @return true if the Gray code sequence for num_bits satisfies
 *         the single-bit-change property (including wrap-around).
 */
bool verify_gray_property(int num_bits);
```

### Part E -- Gray Code Counter (bonus)

Implement a counter that counts in Gray code directly (without converting
from binary) using XOR feedback:

```c
typedef struct {
    uint8_t gray_value;
    uint8_t num_bits;
} gray_counter_t;

void gray_counter_init(gray_counter_t *gc, uint8_t num_bits);
uint8_t gray_counter_increment(gray_counter_t *gc);
uint8_t gray_counter_decrement(gray_counter_t *gc);
```

## Test Cases

1. Verify `binary_to_gray(gray_to_binary(x)) == x` for all 8-bit values
2. Verify `gray_to_binary(binary_to_gray(x)) == x` for all 8-bit values
3. Simulate encoder sequence: 00, 01, 11, 10, 00, 01, 11, 10 (2 CW turns)
4. Simulate encoder reverse: 00, 10, 11, 01, 00 (1 CCW turn)
5. Simulate encoder with noise: 00, 01, 00, 01, 11, 10 (stutter then CW)

## Hints

1. Binary to Gray: `G = B ^ (B >> 1)`
2. Gray to Binary: apply XOR from MSB downward:
   ```
   B[n] = G[n]
   B[n-1] = B[n] ^ G[n-1]
   B[n-2] = B[n-1] ^ G[n-2]
   ...
   ```
3. For the encoder, use a lookup table indexed by `(prev_gray << 2) | curr_gray`
   with entries: 0 (invalid/no-move), +1 (CW), -1 (CCW).

## Deliverable

A single C file `solution_05.c`.
