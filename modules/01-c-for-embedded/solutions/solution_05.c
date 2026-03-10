/**
 * @file solution_05.c
 * @brief Solution for Exercise 05: CRC-8 Calculator
 *
 * Implements three variants of CRC-8 (polynomial 0x07 / SMBus):
 *   1. Bit-by-bit (smallest code, slowest)
 *   2. Full table-driven (256-byte table, fastest)
 *   3. Nibble table (16-byte table, good compromise)
 *
 * All three produce identical results for the same input.
 *
 * Compile: gcc -Wall -Wextra -std=c99 -O2 -o solution_05 solution_05.c
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/* -----------------------------------------------------------------------
 * CRC-8 polynomial: x^8 + x^2 + x + 1  =  0x07
 * Initial value:    0x00
 * Used by SMBus, many sensor protocols (e.g., SHT3x, CCS811).
 * ----------------------------------------------------------------------- */
#define CRC8_POLY       0x07U
#define CRC8_INIT       0x00U

/* Performance test parameters */
#define PERF_BUFFER_SIZE  1024
#define PERF_ITERATIONS   10000

/* -----------------------------------------------------------------------
 * Version 1: Bit-by-bit CRC-8
 *
 * For each byte, XOR it into the CRC register, then process 8 bits.
 * If the MSB is set after the XOR, shift left and XOR with the
 * polynomial. Otherwise, just shift left.
 *
 * Code size:  ~20 bytes of machine code (very small)
 * Speed:      Slow — 8 iterations per byte
 * ----------------------------------------------------------------------- */
uint8_t crc8_bitwise(const uint8_t *data, uint16_t length)
{
    uint8_t crc = CRC8_INIT;

    for (uint16_t i = 0; i < length; i++) {
        crc ^= data[i];

        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ CRC8_POLY;
            } else {
                crc = crc << 1;
            }
        }
    }

    return crc;
}

/* -----------------------------------------------------------------------
 * Version 2: Table-driven CRC-8
 *
 * Pre-compute the CRC of every possible byte value (0x00..0xFF).
 * Then, for each input byte, the per-byte operation is just:
 *     crc = table[crc ^ byte]
 *
 * Table size: 256 bytes (stored in flash on embedded targets)
 * Speed:      Fastest — 1 table lookup + 1 XOR per byte
 * ----------------------------------------------------------------------- */

/**
 * Generate the full 256-entry CRC-8 lookup table.
 *
 * Each entry table[i] is the CRC of the single byte i, computed using
 * the bit-by-bit method.
 */
void crc8_generate_table(uint8_t table[256], uint8_t polynomial)
{
    for (uint16_t i = 0; i < 256; i++) {
        uint8_t crc = (uint8_t)i;

        for (uint8_t bit = 0; bit < 8; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc = crc << 1;
            }
        }

        table[i] = crc;
    }
}

/**
 * Compute CRC-8 using the full lookup table.
 */
uint8_t crc8_table(const uint8_t *data, uint16_t length,
                   const uint8_t table[256])
{
    uint8_t crc = CRC8_INIT;

    for (uint16_t i = 0; i < length; i++) {
        crc = table[crc ^ data[i]];
    }

    return crc;
}

/* -----------------------------------------------------------------------
 * Version 3: Nibble (half-byte) table CRC-8
 *
 * A compromise between code size and speed. Uses a 16-entry table
 * (one entry per 4-bit nibble). Each byte is processed in two steps:
 * high nibble first, then low nibble.
 *
 * Table size: 16 bytes
 * Speed:      Medium — 2 lookups per byte
 * ----------------------------------------------------------------------- */

/**
 * Generate the 16-entry nibble CRC-8 lookup table.
 *
 * Each entry table[i] is the CRC of a 4-bit value i, with that value
 * placed in the top 4 bits of a byte (shifted left by 4).
 */
void crc8_generate_nibble_table(uint8_t table[16], uint8_t polynomial)
{
    for (uint8_t i = 0; i < 16; i++) {
        uint8_t crc = i << 4;  /* Place nibble in upper 4 bits */

        for (uint8_t bit = 0; bit < 4; bit++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ polynomial;
            } else {
                crc = crc << 1;
            }
        }

        table[i] = crc;
    }
}

/**
 * Compute CRC-8 using the nibble lookup table.
 *
 * For each byte:
 *   1. Process the high nibble: XOR the upper 4 bits of crc with the
 *      high nibble of the data byte, look up in the table, XOR with
 *      the remaining (shifted) CRC bits.
 *   2. Process the low nibble:  Same but for the lower 4 bits.
 */
uint8_t crc8_nibble(const uint8_t *data, uint16_t length,
                    const uint8_t table[16])
{
    uint8_t crc = CRC8_INIT;

    for (uint16_t i = 0; i < length; i++) {
        /* High nibble */
        crc = table[(crc >> 4) ^ (data[i] >> 4)] ^ (crc << 4);
        /* Low nibble */
        crc = table[(crc >> 4) ^ (data[i] & 0x0F)] ^ (crc << 4);
    }

    return crc;
}

/* -----------------------------------------------------------------------
 * Print the full 256-entry table in 16 columns
 * ----------------------------------------------------------------------- */
static void print_table(const uint8_t table[256])
{
    printf("CRC-8 Lookup Table (polynomial 0x%02X):\n", CRC8_POLY);
    for (int row = 0; row < 16; row++) {
        printf("  ");
        for (int col = 0; col < 16; col++) {
            printf("%02X ", table[row * 16 + col]);
        }
        printf("\n");
    }
}

/* -----------------------------------------------------------------------
 * Test a CRC value against an expected result
 * ----------------------------------------------------------------------- */
static int test_crc(const char *label, uint8_t actual, uint8_t expected)
{
    int pass = (actual == expected);
    printf("  %-10s 0x%02X [%s]\n", label, actual, pass ? "PASS" : "FAIL");
    return pass;
}

/* ======================================================================= */
int main(void)
{
    printf("=== CRC-8 Calculator ===\n\n");

    /* -----------------------------------------------------------
     * Generate lookup tables
     * ----------------------------------------------------------- */
    uint8_t full_table[256];
    uint8_t nibble_table[16];

    crc8_generate_table(full_table, CRC8_POLY);
    crc8_generate_nibble_table(nibble_table, CRC8_POLY);

    /* Print the full table */
    print_table(full_table);
    printf("\n");

    /* -----------------------------------------------------------
     * Test vector 1: "123456789" -> 0xF4 (CRC-8/SMBus standard)
     * ----------------------------------------------------------- */
    int all_pass = 1;

    const uint8_t test_str[] = "123456789";
    uint16_t test_len = 9;

    printf("Test vector: \"123456789\"\n");
    all_pass &= test_crc("Bitwise:", crc8_bitwise(test_str, test_len), 0xF4);
    all_pass &= test_crc("Table:", crc8_table(test_str, test_len, full_table), 0xF4);
    all_pass &= test_crc("Nibble:", crc8_nibble(test_str, test_len, nibble_table), 0xF4);
    printf("\n");

    /* -----------------------------------------------------------
     * Test vector 2: Empty input -> 0x00
     * ----------------------------------------------------------- */
    printf("Test: empty input\n");
    all_pass &= test_crc("CRC =", crc8_bitwise(NULL, 0), 0x00);
    printf("\n");

    /* -----------------------------------------------------------
     * Test vector 3: {0xFF} -> 0xF1
     * ----------------------------------------------------------- */
    const uint8_t test_ff[] = { 0xFF };
    printf("Test: {0xFF}\n");
    all_pass &= test_crc("CRC =", crc8_bitwise(test_ff, 1), 0xF1);
    printf("\n");

    /* -----------------------------------------------------------
     * Consistency check: all three methods on varied data
     * ----------------------------------------------------------- */
    printf("Consistency check (256 single-byte values):\n");
    int consistent = 1;
    for (uint16_t b = 0; b < 256; b++) {
        uint8_t byte_val = (uint8_t)b;
        uint8_t c1 = crc8_bitwise(&byte_val, 1);
        uint8_t c2 = crc8_table(&byte_val, 1, full_table);
        uint8_t c3 = crc8_nibble(&byte_val, 1, nibble_table);
        if (c1 != c2 || c1 != c3) {
            printf("  MISMATCH at 0x%02X: bitwise=0x%02X table=0x%02X nibble=0x%02X\n",
                   b, c1, c2, c3);
            consistent = 0;
        }
    }
    printf("  All methods agree: [%s]\n\n", consistent ? "PASS" : "FAIL");
    all_pass &= consistent;

    /* -----------------------------------------------------------
     * Performance comparison
     * ----------------------------------------------------------- */
    printf("Performance (%d bytes, %d iterations):\n",
           PERF_BUFFER_SIZE, PERF_ITERATIONS);

    /* Fill a test buffer with pseudo-random data */
    uint8_t perf_buf[PERF_BUFFER_SIZE];
    for (int i = 0; i < PERF_BUFFER_SIZE; i++) {
        perf_buf[i] = (uint8_t)(i * 37 + 17);
    }

    volatile uint8_t result;  /* volatile prevents optimizer from eliding */
    clock_t start, end;

    /* Bitwise */
    start = clock();
    for (int iter = 0; iter < PERF_ITERATIONS; iter++) {
        result = crc8_bitwise(perf_buf, PERF_BUFFER_SIZE);
    }
    end = clock();
    printf("  Bitwise: %lu us\n",
           (unsigned long)((end - start) * 1000000 / CLOCKS_PER_SEC));

    /* Table */
    start = clock();
    for (int iter = 0; iter < PERF_ITERATIONS; iter++) {
        result = crc8_table(perf_buf, PERF_BUFFER_SIZE, full_table);
    }
    end = clock();
    printf("  Table:   %lu us\n",
           (unsigned long)((end - start) * 1000000 / CLOCKS_PER_SEC));

    /* Nibble */
    start = clock();
    for (int iter = 0; iter < PERF_ITERATIONS; iter++) {
        result = crc8_nibble(perf_buf, PERF_BUFFER_SIZE, nibble_table);
    }
    end = clock();
    printf("  Nibble:  %lu us\n",
           (unsigned long)((end - start) * 1000000 / CLOCKS_PER_SEC));

    (void)result;  /* Suppress unused-variable warning */

    /* -----------------------------------------------------------
     * Summary
     * ----------------------------------------------------------- */
    printf("\n%s\n", all_pass ? "All tests passed!" : "Some tests FAILED!");
    printf("\n=== End of Exercise 05 ===\n");
    return 0;
}
