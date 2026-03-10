/**
 * @file solution_06.c
 * @brief Solution for Exercise 06: Bit-Banded Register Access
 *
 * Simulates ARM Cortex-M bit-banding using arrays and pointer arithmetic.
 * Bit-banding maps each bit of a peripheral register to a unique 32-bit
 * word address, enabling atomic single-bit access without read-modify-write.
 *
 * Compile: gcc -Wall -Wextra -std=c99 -o solution_06 solution_06.c
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * Bit-band address definitions (ARM Cortex-M3/M4)
 *
 * Peripheral bit-band region:  0x4000_0000 .. 0x400F_FFFF  (1 MB)
 * Peripheral bit-band alias:   0x4200_0000 .. 0x43FF_FFFF  (32 MB)
 *
 * Each bit in the region maps to a 4-byte word in the alias region:
 *   alias_addr = alias_base + (byte_offset * 32) + (bit_number * 4)
 * ----------------------------------------------------------------------- */
#define PERIPH_BASE       0x40000000UL
#define PERIPH_BB_BASE    0x42000000UL

/* GPIO register offsets (STM32-like) */
#define GPIO_IDR_OFFSET   0x10   /* Input Data Register */
#define GPIO_ODR_OFFSET   0x14   /* Output Data Register */

/* Simulated peripheral memory (256 bytes) */
static uint8_t periph_mem[256];

/* -----------------------------------------------------------------------
 * bitband_alias
 *
 * Calculate the bit-band alias address for a given register offset and
 * bit number, using the ARM Cortex-M formula:
 *
 *   alias_addr = PERIPH_BB_BASE + (reg_offset * 32) + (bit_number * 4)
 *
 * @param reg_offset  Byte offset from PERIPH_BASE
 * @param bit_number  Bit within the byte (0-7)
 * @return            The full alias address
 * ----------------------------------------------------------------------- */
uint32_t bitband_alias(uint32_t reg_offset, uint8_t bit_number)
{
    return PERIPH_BB_BASE + (reg_offset * 32) + (bit_number * 4);
}

/* -----------------------------------------------------------------------
 * bitband_set / bitband_clear / bitband_read
 *
 * These functions simulate what the hardware does when you write to
 * a bit-band alias address. In hardware, the write is atomic.
 * In simulation, we compute which byte and bit the alias maps to,
 * then set/clear/read that bit.
 *
 * From alias offset, recover byte and bit:
 *   alias_offset = reg_offset * 32 + bit_number * 4
 *   byte_offset  = alias_offset / 32
 *   bit_number   = (alias_offset % 32) / 4
 * ----------------------------------------------------------------------- */
void bitband_set(uint8_t *mem, uint32_t reg_offset, uint8_t bit)
{
    mem[reg_offset] |= (1U << bit);
}

void bitband_clear(uint8_t *mem, uint32_t reg_offset, uint8_t bit)
{
    mem[reg_offset] &= ~(1U << bit);
}

uint8_t bitband_read(const uint8_t *mem, uint32_t reg_offset, uint8_t bit)
{
    return (mem[reg_offset] >> bit) & 1U;
}

/* -----------------------------------------------------------------------
 * Traditional Read-Modify-Write (RMW) for comparison
 *
 * On a real system without bit-banding, setting a single bit requires:
 *   1. Read the register
 *   2. Modify the bit
 *   3. Write the register back
 *
 * This is NOT atomic -- an interrupt between step 1 and step 3 can
 * cause a race condition.
 * ----------------------------------------------------------------------- */
void rmw_set_bit(uint8_t *mem, uint32_t reg_offset, uint8_t bit)
{
    uint8_t val = mem[reg_offset];   /* Read  */
    val |= (1U << bit);             /* Modify */
    mem[reg_offset] = val;           /* Write  */
}

void rmw_clear_bit(uint8_t *mem, uint32_t reg_offset, uint8_t bit)
{
    uint8_t val = mem[reg_offset];
    val &= ~(1U << bit);
    mem[reg_offset] = val;
}

uint8_t rmw_read_bit(const uint8_t *mem, uint32_t reg_offset, uint8_t bit)
{
    return (mem[reg_offset] >> bit) & 1U;
}

/* ======================================================================= */
int main(void)
{
    printf("=== Bit-Band Simulation ===\n\n");

    /* -----------------------------------------------------------
     * Part 1: Show alias address calculation
     * ----------------------------------------------------------- */
    uint32_t alias = bitband_alias(GPIO_ODR_OFFSET, 5);
    uint32_t alias_offset = alias - PERIPH_BB_BASE;

    printf("Alias address for reg_offset=0x%02X, bit=5:\n", GPIO_ODR_OFFSET);
    printf("  Base addr:  0x%08X\n", (unsigned)(PERIPH_BASE + GPIO_ODR_OFFSET));
    printf("  Alias addr: 0x%08X\n", (unsigned)alias);
    printf("  Alias offset: 0x%03X  (byte 0x%02X * 32 + bit 5 * 4)\n\n",
           (unsigned)alias_offset, GPIO_ODR_OFFSET);

    /* -----------------------------------------------------------
     * Part 2: GPIO ODR test using bit-banding
     * ----------------------------------------------------------- */
    memset(periph_mem, 0, sizeof(periph_mem));

    printf("GPIO ODR test (bit-banded):\n");
    printf("  Initial ODR: 0x%02X\n", periph_mem[GPIO_ODR_OFFSET]);

    /* Set pin 5 */
    bitband_set(periph_mem, GPIO_ODR_OFFSET, 5);
    printf("  Set pin 5:   0x%02X\n", periph_mem[GPIO_ODR_OFFSET]);

    /* Set pin 0 */
    bitband_set(periph_mem, GPIO_ODR_OFFSET, 0);
    printf("  Set pin 0:   0x%02X\n", periph_mem[GPIO_ODR_OFFSET]);

    /* Clear pin 5 */
    bitband_clear(periph_mem, GPIO_ODR_OFFSET, 5);
    printf("  Clear pin 5: 0x%02X\n", periph_mem[GPIO_ODR_OFFSET]);

    /* Toggle pin 0: read, flip, write */
    uint8_t pin0_val = bitband_read(periph_mem, GPIO_ODR_OFFSET, 0);
    if (pin0_val) {
        bitband_clear(periph_mem, GPIO_ODR_OFFSET, 0);
    } else {
        bitband_set(periph_mem, GPIO_ODR_OFFSET, 0);
    }
    printf("  Toggle pin 0 (read + flip + write): 0x%02X\n\n",
           periph_mem[GPIO_ODR_OFFSET]);

    /* -----------------------------------------------------------
     * Part 3: Verify bit-band matches RMW for all 8 bits
     * ----------------------------------------------------------- */
    printf("Verification: bit-band matches RMW for all 8 bits ");

    int all_pass = 1;

    for (uint8_t bit = 0; bit < 8; bit++) {
        /* Reset both test bytes */
        periph_mem[0x00] = 0x00;
        periph_mem[0x01] = 0x00;

        /* Set via bit-band */
        bitband_set(periph_mem, 0x00, bit);
        /* Set via RMW */
        rmw_set_bit(periph_mem, 0x01, bit);

        if (periph_mem[0x00] != periph_mem[0x01]) {
            all_pass = 0;
            printf("\n  SET mismatch at bit %d: bb=0x%02X rmw=0x%02X",
                   bit, periph_mem[0x00], periph_mem[0x01]);
        }

        /* Read via both */
        uint8_t bb_read  = bitband_read(periph_mem, 0x00, bit);
        uint8_t rmw_read = rmw_read_bit(periph_mem, 0x01, bit);
        if (bb_read != rmw_read) {
            all_pass = 0;
            printf("\n  READ mismatch at bit %d: bb=%d rmw=%d",
                   bit, bb_read, rmw_read);
        }

        /* Clear via both */
        bitband_clear(periph_mem, 0x00, bit);
        rmw_clear_bit(periph_mem, 0x01, bit);

        if (periph_mem[0x00] != periph_mem[0x01]) {
            all_pass = 0;
            printf("\n  CLEAR mismatch at bit %d: bb=0x%02X rmw=0x%02X",
                   bit, periph_mem[0x00], periph_mem[0x01]);
        }
    }

    printf("[%s]\n\n", all_pass ? "PASS" : "FAIL");

    /* -----------------------------------------------------------
     * Part 4: Multi-bit alias address table
     * ----------------------------------------------------------- */
    printf("Alias addresses for GPIO_ODR (offset 0x%02X):\n", GPIO_ODR_OFFSET);
    printf("  Bit  | Base Address | Alias Address\n");
    printf("  -----+--------------+--------------\n");
    for (uint8_t bit = 0; bit < 8; bit++) {
        uint32_t a = bitband_alias(GPIO_ODR_OFFSET, bit);
        printf("   %d   | 0x%08X   | 0x%08X\n",
               bit,
               (unsigned)(PERIPH_BASE + GPIO_ODR_OFFSET),
               (unsigned)a);
    }

    /* -----------------------------------------------------------
     * Part 5: Simulate button read via IDR
     * ----------------------------------------------------------- */
    printf("\nGPIO IDR test (simulated button on pin 3):\n");
    memset(periph_mem, 0, sizeof(periph_mem));

    /* Simulate external hardware setting pin 3 high (button pressed) */
    periph_mem[GPIO_IDR_OFFSET] = 0x08;  /* bit 3 set */

    printf("  IDR = 0x%02X\n", periph_mem[GPIO_IDR_OFFSET]);
    printf("  Button (pin 3) = %d (pressed)\n",
           bitband_read(periph_mem, GPIO_IDR_OFFSET, 3));
    printf("  Pin 0 = %d (not pressed)\n",
           bitband_read(periph_mem, GPIO_IDR_OFFSET, 0));

    printf("\n=== End of Exercise 06 ===\n");
    return 0;
}
