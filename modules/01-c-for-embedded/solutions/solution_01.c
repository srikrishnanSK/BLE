/**
 * @file solution_01.c
 * @brief Solution for Exercise 01: Bitfield Extraction
 *
 * Demonstrates extracting and inserting bitfields from/into a 32-bit
 * hardware register value using shift-and-mask operations.
 *
 * Compile: gcc -Wall -Wextra -std=c99 -o solution_01 solution_01.c
 */

#include <stdint.h>
#include <stdio.h>

/* -----------------------------------------------------------------------
 * Register field definitions
 *
 * Status Register Layout (32 bits):
 *   [31:24]  Device ID        (8 bits)
 *   [23:20]  Hardware revision (4 bits)
 *   [19:16]  Error code        (4 bits)
 *   [15:8]   Data count        (8 bits)
 *   [7]      Overflow flag     (1 bit)
 *   [6]      Underflow flag    (1 bit)
 *   [5:4]    Operating mode    (2 bits)
 *   [3:0]    Channel select    (4 bits)
 * ----------------------------------------------------------------------- */

/* Shift amounts (bit position of the LSB of each field) */
#define DEVICE_ID_SHIFT     24
#define HW_REV_SHIFT        20
#define ERROR_CODE_SHIFT    16
#define DATA_COUNT_SHIFT    8
#define OVERFLOW_SHIFT      7
#define UNDERFLOW_SHIFT     6
#define OP_MODE_SHIFT       4
#define CHANNEL_SEL_SHIFT   0

/* Masks (width of each field in ones, NOT shifted) */
#define DEVICE_ID_MASK      0xFFU    /* 8 bits */
#define HW_REV_MASK         0x0FU    /* 4 bits */
#define ERROR_CODE_MASK     0x0FU    /* 4 bits */
#define DATA_COUNT_MASK     0xFFU    /* 8 bits */
#define OVERFLOW_MASK       0x01U    /* 1 bit  */
#define UNDERFLOW_MASK      0x01U    /* 1 bit  */
#define OP_MODE_MASK        0x03U    /* 2 bits */
#define CHANNEL_SEL_MASK    0x0FU    /* 4 bits */

/* -----------------------------------------------------------------------
 * extract_field
 *
 * Extracts a bitfield from a register value.
 *
 * Steps:
 * 1. Shift right to move the field to bit 0
 * 2. Mask off the upper bits to isolate the field
 *
 * @param reg   The full 32-bit register value
 * @param shift Bit position of the field's LSB
 * @param mask  Mask with 1s for each bit in the field (unshifted)
 * @return      The extracted field value
 * ----------------------------------------------------------------------- */
uint32_t extract_field(uint32_t reg, uint8_t shift, uint32_t mask)
{
    return (reg >> shift) & mask;
}

/* -----------------------------------------------------------------------
 * insert_field
 *
 * Inserts a value into a bitfield of a register.
 *
 * Steps:
 * 1. Clear the field in the register using AND with inverted mask
 * 2. Shift the new value into position
 * 3. OR it into the register
 *
 * @param reg   The original register value
 * @param shift Bit position of the field's LSB
 * @param mask  Mask with 1s for each bit in the field (unshifted)
 * @param value The new value to insert (must fit within the mask)
 * @return      The modified register value
 * ----------------------------------------------------------------------- */
uint32_t insert_field(uint32_t reg, uint8_t shift, uint32_t mask, uint32_t value)
{
    /* Step 1: Clear the field */
    reg &= ~(mask << shift);

    /* Step 2 & 3: Mask the value (safety), shift it, and OR into register */
    reg |= (value & mask) << shift;

    return reg;
}

/* -----------------------------------------------------------------------
 * decode_status_register
 *
 * Extracts and prints every field from the status register.
 * ----------------------------------------------------------------------- */
void decode_status_register(uint32_t reg)
{
    printf("Status Register: 0x%08X\n", reg);
    printf("  Device ID:        0x%02X\n",
           extract_field(reg, DEVICE_ID_SHIFT, DEVICE_ID_MASK));
    printf("  Hardware Rev:     0x%X\n",
           extract_field(reg, HW_REV_SHIFT, HW_REV_MASK));
    printf("  Error Code:       0x%X\n",
           extract_field(reg, ERROR_CODE_SHIFT, ERROR_CODE_MASK));
    printf("  Data Count:       0x%02X\n",
           extract_field(reg, DATA_COUNT_SHIFT, DATA_COUNT_MASK));
    printf("  Overflow:         %u\n",
           extract_field(reg, OVERFLOW_SHIFT, OVERFLOW_MASK));
    printf("  Underflow:        %u\n",
           extract_field(reg, UNDERFLOW_SHIFT, UNDERFLOW_MASK));
    printf("  Operating Mode:   0x%X\n",
           extract_field(reg, OP_MODE_SHIFT, OP_MODE_MASK));
    printf("  Channel Select:   0x%X\n",
           extract_field(reg, CHANNEL_SEL_SHIFT, CHANNEL_SEL_MASK));
}

/* ======================================================================= */
int main(void)
{
    printf("=== Exercise 01: Bitfield Extraction ===\n\n");

    /* -----------------------------------------------------------
     * Part 1: Decode the register
     *
     * 0xA3F540B7 in binary:
     * 1010_0011  1111_0101  0100_0000  1011_0111
     * [DevID=A3] [Rev=F][Err=5] [Count=40] [OV=1][UF=0][Mode=3][Ch=7]
     * ----------------------------------------------------------- */
    uint32_t status_reg = 0xA3F540B7;
    decode_status_register(status_reg);

    /* -----------------------------------------------------------
     * Part 2: Modify specific fields
     * ----------------------------------------------------------- */
    printf("\n--- Modifying register ---\n\n");

    /* Change operating mode to 0x3 (already 0x3 in this value, but
     * let's demonstrate the operation anyway) */
    status_reg = insert_field(status_reg, OP_MODE_SHIFT, OP_MODE_MASK, 0x3);
    printf("After setting mode to 0x3: 0x%08X\n", status_reg);

    /* Clear the overflow flag (set it to 0) */
    status_reg = insert_field(status_reg, OVERFLOW_SHIFT, OVERFLOW_MASK, 0x0);
    printf("After clearing overflow:   0x%08X\n", status_reg);

    /* Set channel select to 0xC */
    status_reg = insert_field(status_reg, CHANNEL_SEL_SHIFT, CHANNEL_SEL_MASK, 0xC);
    printf("After setting channel=0xC: 0x%08X\n", status_reg);

    printf("\n");
    decode_status_register(status_reg);

    /* -----------------------------------------------------------
     * Part 3: Verify round-trip
     * ----------------------------------------------------------- */
    printf("\n--- Round-trip verification ---\n\n");
    uint32_t test_reg = 0x00000000;

    /* Build a register value field by field */
    test_reg = insert_field(test_reg, DEVICE_ID_SHIFT, DEVICE_ID_MASK, 0xAB);
    test_reg = insert_field(test_reg, HW_REV_SHIFT, HW_REV_MASK, 0x03);
    test_reg = insert_field(test_reg, ERROR_CODE_SHIFT, ERROR_CODE_MASK, 0x00);
    test_reg = insert_field(test_reg, DATA_COUNT_SHIFT, DATA_COUNT_MASK, 0xFF);
    test_reg = insert_field(test_reg, OVERFLOW_SHIFT, OVERFLOW_MASK, 0);
    test_reg = insert_field(test_reg, UNDERFLOW_SHIFT, UNDERFLOW_MASK, 1);
    test_reg = insert_field(test_reg, OP_MODE_SHIFT, OP_MODE_MASK, 0x02);
    test_reg = insert_field(test_reg, CHANNEL_SEL_SHIFT, CHANNEL_SEL_MASK, 0x09);

    printf("Built register: 0x%08X\n", test_reg);
    decode_status_register(test_reg);

    /* Verify extraction matches what we inserted */
    int pass = 1;
    pass &= (extract_field(test_reg, DEVICE_ID_SHIFT, DEVICE_ID_MASK) == 0xAB);
    pass &= (extract_field(test_reg, HW_REV_SHIFT, HW_REV_MASK) == 0x03);
    pass &= (extract_field(test_reg, ERROR_CODE_SHIFT, ERROR_CODE_MASK) == 0x00);
    pass &= (extract_field(test_reg, DATA_COUNT_SHIFT, DATA_COUNT_MASK) == 0xFF);
    pass &= (extract_field(test_reg, OVERFLOW_SHIFT, OVERFLOW_MASK) == 0);
    pass &= (extract_field(test_reg, UNDERFLOW_SHIFT, UNDERFLOW_MASK) == 1);
    pass &= (extract_field(test_reg, OP_MODE_SHIFT, OP_MODE_MASK) == 0x02);
    pass &= (extract_field(test_reg, CHANNEL_SEL_SHIFT, CHANNEL_SEL_MASK) == 0x09);

    printf("\nRound-trip check: %s\n", pass ? "[PASS]" : "[FAIL]");

    printf("\n=== End of Exercise 01 ===\n");
    return 0;
}
