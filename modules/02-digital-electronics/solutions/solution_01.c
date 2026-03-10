/**
 * Module 02 - Solution 01: Binary Arithmetic
 *
 * Implements unsigned and signed 8-bit addition/subtraction with overflow
 * and underflow detection, plus multi-byte (32-bit) addition using byte
 * arrays -- simulating how a CPU without a 32-bit ALU would operate.
 *
 * Build:  gcc -Wall -Wextra -std=c99 -o solution_01 solution_01.c
 * Run:    ./solution_01
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * Part A: Unsigned 8-bit Addition
 * -----------------------------------------------------------------------*/

typedef struct {
    uint8_t result;
    bool    carry;     /* true if result overflowed (>255) */
} add_u8_result_t;

add_u8_result_t add_u8(uint8_t a, uint8_t b)
{
    add_u8_result_t r;
    uint16_t sum = (uint16_t)a + (uint16_t)b;

    r.result = (uint8_t)(sum & 0xFF);
    r.carry  = (sum > 0xFF);

    return r;
}

/* -----------------------------------------------------------------------
 * Part B: Unsigned 8-bit Subtraction
 * -----------------------------------------------------------------------*/

typedef struct {
    uint8_t result;
    bool    borrow;    /* true if a < b (underflow) */
} sub_u8_result_t;

sub_u8_result_t sub_u8(uint8_t a, uint8_t b)
{
    sub_u8_result_t r;

    r.borrow = (a < b);
    /* In unsigned arithmetic, subtraction wraps around modulo 256 */
    r.result = (uint8_t)(a - b);

    return r;
}

/* -----------------------------------------------------------------------
 * Part C: Signed 8-bit Addition with Overflow Detection
 * -----------------------------------------------------------------------*/

typedef struct {
    int8_t  result;
    bool    overflow;  /* true if signed overflow occurred */
} add_s8_result_t;

add_s8_result_t add_s8(int8_t a, int8_t b)
{
    add_s8_result_t r;

    /* Perform the addition in a wider type to capture the raw result,
     * then truncate to 8 bits to match what hardware would produce. */
    int16_t sum = (int16_t)a + (int16_t)b;
    r.result = (int8_t)(uint8_t)(sum & 0xFF);

    /* Signed overflow: both operands same sign, result differs.
     * Positive + Positive = Negative  => overflow
     * Negative + Negative = Positive  => overflow */
    bool sign_a = (a < 0);
    bool sign_b = (b < 0);
    bool sign_r = (r.result < 0);
    r.overflow = (sign_a == sign_b) && (sign_r != sign_a);

    return r;
}

/* -----------------------------------------------------------------------
 * Part D: Multi-byte Addition (32-bit via 4 bytes, little-endian)
 * -----------------------------------------------------------------------*/

typedef struct {
    uint8_t result[4];
    bool    carry;
} add_u32_bytes_result_t;

/**
 * Add two 32-bit numbers stored as arrays of 4 bytes (little-endian),
 * propagating carries from byte to byte -- the way a CPU without a
 * 32-bit ALU would do it.
 */
add_u32_bytes_result_t add_u32_bytes(const uint8_t a[4], const uint8_t b[4])
{
    add_u32_bytes_result_t r;
    uint16_t carry = 0;

    for (int i = 0; i < 4; i++) {
        uint16_t sum = (uint16_t)a[i] + (uint16_t)b[i] + carry;
        r.result[i] = (uint8_t)(sum & 0xFF);
        carry = (sum >> 8) & 1;
    }

    r.carry = (carry != 0);
    return r;
}

/* -----------------------------------------------------------------------
 * Helper: print a 32-bit value stored as 4 little-endian bytes in hex
 * -----------------------------------------------------------------------*/

static void print_u32_bytes(const uint8_t bytes[4])
{
    /* Print as a single hex value (big-endian display) */
    printf("0x%02X%02X%02X%02X", bytes[3], bytes[2], bytes[1], bytes[0]);
}

/* -----------------------------------------------------------------------
 * Helper: print binary representation of an 8-bit value
 * -----------------------------------------------------------------------*/

static void print_binary8(uint8_t val)
{
    for (int i = 7; i >= 0; i--) {
        putchar((val >> i) & 1 ? '1' : '0');
        if (i == 4) putchar(' ');
    }
}

/* -----------------------------------------------------------------------
 * Main: Run all test cases from the exercise
 * -----------------------------------------------------------------------*/

int main(void)
{
    printf("===========================================\n");
    printf("  Solution 01: Binary Arithmetic\n");
    printf("===========================================\n\n");

    /* --- Part A: Unsigned 8-bit Addition --- */
    printf("--- Part A: Unsigned 8-bit Addition ---\n\n");

    struct { uint8_t a, b; } u8_add_tests[] = {
        {200, 100},
        {100,  50},
        {255,   1},
        {  0,   0},
        {128, 128},
    };
    int n_add = sizeof(u8_add_tests) / sizeof(u8_add_tests[0]);

    printf("  %-6s %-6s %-12s %-12s %s\n",
           "A", "B", "A (binary)", "B (binary)", "Result");
    printf("  %-6s %-6s %-12s %-12s %s\n",
           "------", "------", "----------", "----------", "------");

    for (int i = 0; i < n_add; i++) {
        uint8_t a = u8_add_tests[i].a;
        uint8_t b = u8_add_tests[i].b;
        add_u8_result_t r = add_u8(a, b);

        printf("  %-6u %-6u ", a, b);
        print_binary8(a);
        printf("   ");
        print_binary8(b);
        printf("   %3u  carry=%d\n", r.result, r.carry);
    }

    /* --- Part B: Unsigned 8-bit Subtraction --- */
    printf("\n--- Part B: Unsigned 8-bit Subtraction ---\n\n");

    struct { uint8_t a, b; } u8_sub_tests[] = {
        { 50,  30},
        { 30,  50},
        {  0,   1},
        {255, 255},
    };
    int n_sub = sizeof(u8_sub_tests) / sizeof(u8_sub_tests[0]);

    printf("  %-6s %-6s %-8s %s\n", "A", "B", "Result", "Flag");
    printf("  %-6s %-6s %-8s %s\n", "------", "------", "------", "------");

    for (int i = 0; i < n_sub; i++) {
        uint8_t a = u8_sub_tests[i].a;
        uint8_t b = u8_sub_tests[i].b;
        sub_u8_result_t r = sub_u8(a, b);

        printf("  %-6u %-6u %-8u borrow=%d\n", a, b, r.result, r.borrow);
    }

    /* --- Part C: Signed 8-bit Addition --- */
    printf("\n--- Part C: Signed 8-bit Addition ---\n\n");

    struct { int8_t a, b; } s8_add_tests[] = {
        { 100,   50},   /* overflow: 150 > 127 */
        {-100,  -50},   /* overflow: -150 < -128 */
        {  50,   30},   /* no overflow */
        { -50,   30},   /* no overflow */
        { 127,    1},   /* overflow */
        {-128,   -1},   /* overflow */
    };
    int n_s8 = sizeof(s8_add_tests) / sizeof(s8_add_tests[0]);

    printf("  %-6s %-6s %-12s %-12s %-8s %s\n",
           "A", "B", "A (binary)", "B (binary)", "Result", "Flag");
    printf("  %-6s %-6s %-12s %-12s %-8s %s\n",
           "------", "------", "----------", "----------", "------", "------");

    for (int i = 0; i < n_s8; i++) {
        int8_t a = s8_add_tests[i].a;
        int8_t b = s8_add_tests[i].b;
        add_s8_result_t r = add_s8(a, b);

        printf("  %+5d %+5d ", a, b);
        print_binary8((uint8_t)a);
        printf("   ");
        print_binary8((uint8_t)b);
        printf("   %+5d  overflow=%d\n", r.result, r.overflow);
    }

    /* --- Part D: Multi-byte Addition --- */
    printf("\n--- Part D: Multi-Byte (32-bit) Addition ---\n\n");

    /* Test: 0x0000FFFF + 0x00000001 = 0x00010000 */
    {
        uint8_t a[4] = {0xFF, 0xFF, 0x00, 0x00};  /* 0x0000FFFF LE */
        uint8_t b[4] = {0x01, 0x00, 0x00, 0x00};  /* 0x00000001 LE */
        add_u32_bytes_result_t r = add_u32_bytes(a, b);

        printf("  ");
        print_u32_bytes(a);
        printf(" + ");
        print_u32_bytes(b);
        printf(" = ");
        print_u32_bytes(r.result);
        printf("  carry=%d\n", r.carry);
    }

    /* Test: 0xFFFFFFFF + 0x00000001 = 0x00000000 with carry */
    {
        uint8_t a[4] = {0xFF, 0xFF, 0xFF, 0xFF};
        uint8_t b[4] = {0x01, 0x00, 0x00, 0x00};
        add_u32_bytes_result_t r = add_u32_bytes(a, b);

        printf("  ");
        print_u32_bytes(a);
        printf(" + ");
        print_u32_bytes(b);
        printf(" = ");
        print_u32_bytes(r.result);
        printf("  carry=%d\n", r.carry);
    }

    /* Test: 0x12345678 + 0x11111111 = 0x23456789 */
    {
        uint8_t a[4] = {0x78, 0x56, 0x34, 0x12};
        uint8_t b[4] = {0x11, 0x11, 0x11, 0x11};
        add_u32_bytes_result_t r = add_u32_bytes(a, b);

        printf("  ");
        print_u32_bytes(a);
        printf(" + ");
        print_u32_bytes(b);
        printf(" = ");
        print_u32_bytes(r.result);
        printf("  carry=%d\n", r.carry);
    }

    /* Test: 0x00FF00FF + 0x00010001 = 0x01000100 */
    {
        uint8_t a[4] = {0xFF, 0x00, 0xFF, 0x00};
        uint8_t b[4] = {0x01, 0x00, 0x01, 0x00};
        add_u32_bytes_result_t r = add_u32_bytes(a, b);

        printf("  ");
        print_u32_bytes(a);
        printf(" + ");
        print_u32_bytes(b);
        printf(" = ");
        print_u32_bytes(r.result);
        printf("  carry=%d\n", r.carry);
    }

    printf("\nAll tests complete.\n");
    return 0;
}
