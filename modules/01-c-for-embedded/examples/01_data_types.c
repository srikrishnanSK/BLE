/**
 * @file 01_data_types.c
 * @brief Demonstrate fixed-width integer types, sizeof, limits, and overflow behavior.
 *
 * Embedded systems require precise control over data sizes. This example shows
 * how to use stdint.h types, query their sizes, understand their limits, and
 * observe what happens during signed and unsigned overflow.
 *
 * Compile: gcc -Wall -Wextra -std=c99 -o 01_data_types 01_data_types.c
 * Run:     ./01_data_types
 */

#include <stdint.h>
#include <stdio.h>
#include <limits.h>

/*
 * On embedded targets, include <stdint.h> instead of relying on
 * platform-specific sizes for int, long, etc.
 */

/* --------------------------------------------------------------------------
 * Helper: Print a separator line
 * -------------------------------------------------------------------------- */
static void print_section(const char *title)
{
    printf("\n========================================\n");
    printf("  %s\n", title);
    printf("========================================\n\n");
}

/* --------------------------------------------------------------------------
 * Section 1: sizeof for all fixed-width types
 * -------------------------------------------------------------------------- */
static void demonstrate_sizeof(void)
{
    print_section("sizeof Fixed-Width Types");

    printf("Type          | Size (bytes)\n");
    printf("--------------+-------------\n");
    printf("int8_t        | %zu\n", sizeof(int8_t));
    printf("uint8_t       | %zu\n", sizeof(uint8_t));
    printf("int16_t       | %zu\n", sizeof(int16_t));
    printf("uint16_t      | %zu\n", sizeof(uint16_t));
    printf("int32_t       | %zu\n", sizeof(int32_t));
    printf("uint32_t      | %zu\n", sizeof(uint32_t));
    printf("int64_t       | %zu\n", sizeof(int64_t));
    printf("uint64_t      | %zu\n", sizeof(uint64_t));

    printf("\n--- Platform-dependent types (may vary!) ---\n");
    printf("char          | %zu\n", sizeof(char));
    printf("short         | %zu\n", sizeof(short));
    printf("int           | %zu\n", sizeof(int));
    printf("long          | %zu\n", sizeof(long));
    printf("long long     | %zu\n", sizeof(long long));
    printf("float         | %zu\n", sizeof(float));
    printf("double        | %zu\n", sizeof(double));
    printf("void *        | %zu\n", sizeof(void *));

    /*
     * NOTE: On MSP430 (16-bit), sizeof(int) == 2.
     * On AVR (8-bit Arduino Uno), sizeof(int) == 2.
     * On ARM Cortex-M (STM32), sizeof(int) == 4.
     * This is why we ALWAYS use stdint.h types in embedded code.
     */
}

/* --------------------------------------------------------------------------
 * Section 2: Limits of each type
 * -------------------------------------------------------------------------- */
static void demonstrate_limits(void)
{
    print_section("Type Limits");

    printf("int8_t  : %d to %d\n", INT8_MIN, INT8_MAX);
    printf("uint8_t : 0 to %u\n", UINT8_MAX);
    printf("int16_t : %d to %d\n", INT16_MIN, INT16_MAX);
    printf("uint16_t: 0 to %u\n", UINT16_MAX);
    printf("int32_t : %d to %d\n", INT32_MIN, INT32_MAX);
    printf("uint32_t: 0 to %u\n", UINT32_MAX);
    printf("int64_t : %lld to %lld\n", (long long)INT64_MIN, (long long)INT64_MAX);
    printf("uint64_t: 0 to %llu\n", (unsigned long long)UINT64_MAX);

    /*
     * In embedded systems, knowing these limits helps prevent:
     * - ADC values exceeding uint16_t range
     * - Timer counters rolling over unexpectedly
     * - Sensor data clipping
     */
}

/* --------------------------------------------------------------------------
 * Section 3: Unsigned overflow (well-defined in C)
 * -------------------------------------------------------------------------- */
static void demonstrate_unsigned_overflow(void)
{
    print_section("Unsigned Overflow (Well-Defined Wrap-Around)");

    uint8_t counter = 250;
    printf("Starting value: %u\n", counter);

    for (int i = 0; i < 10; i++) {
        counter++;
        printf("After increment %d: %u\n", i + 1, counter);
    }
    /*
     * Unsigned overflow wraps around: 255 + 1 = 0.
     * This is WELL-DEFINED behavior in C and is commonly used
     * for timer counters, sequence numbers, and checksums.
     */

    printf("\nPractical use: Timer tick difference\n");
    uint8_t start_tick = 240;
    uint8_t end_tick   = 10;
    /* Even though end < start, unsigned subtraction gives correct elapsed: */
    uint8_t elapsed = end_tick - start_tick;  /* 10 - 240 = 26 (mod 256) */
    printf("Start: %u, End: %u, Elapsed: %u\n", start_tick, end_tick, elapsed);
}

/* --------------------------------------------------------------------------
 * Section 4: Signed overflow (UNDEFINED behavior in C!)
 * -------------------------------------------------------------------------- */
static void demonstrate_signed_overflow(void)
{
    print_section("Signed Overflow (UNDEFINED BEHAVIOR!)");

    /*
     * WARNING: Signed integer overflow is UNDEFINED BEHAVIOR in C.
     * The compiler may optimize based on the assumption it never happens.
     * We demonstrate it here for educational purposes only.
     * NEVER rely on signed overflow behavior in production code!
     */

    int8_t value = 120;
    printf("Starting value: %d\n", value);

    for (int i = 0; i < 10; i++) {
        /* On most platforms, this wraps 127 -> -128, but it is NOT guaranteed */
        value++;
        printf("After increment %d: %d\n", i + 1, value);
    }

    printf("\n*** Signed overflow is UNDEFINED BEHAVIOR ***\n");
    printf("*** Never rely on it! Check before arithmetic. ***\n");

    /* Safe pattern: check before incrementing */
    int8_t safe_val = 126;
    if (safe_val < INT8_MAX) {
        safe_val++;
        printf("\nSafe increment: %d\n", safe_val);
    } else {
        printf("\nWould overflow! Clamping at %d\n", safe_val);
    }
}

/* --------------------------------------------------------------------------
 * Section 5: Signed/unsigned comparison trap
 * -------------------------------------------------------------------------- */
static void demonstrate_comparison_trap(void)
{
    print_section("Signed vs Unsigned Comparison Trap");

    int8_t  signed_val   = -1;
    uint8_t unsigned_val =  1;

    /*
     * When comparing signed and unsigned values of the same size,
     * C converts the signed value to unsigned first.
     * -1 as uint8_t becomes 255, so -1 > 1 becomes 255 > 1 == true!
     */
    printf("signed_val   = %d\n", signed_val);
    printf("unsigned_val = %u\n", unsigned_val);

    if (signed_val < unsigned_val) {
        printf("Result: signed_val < unsigned_val (mathematically correct)\n");
    } else {
        printf("Result: signed_val >= unsigned_val (TRAP! -1 became 255)\n");
    }

    /* With 32-bit values, the trap is more subtle: */
    int32_t  neg = -1;
    uint32_t pos =  1;
    printf("\nint32_t(-1) compared to uint32_t(1):\n");
    if (neg < pos) {
        printf("  -1 < 1 (correct on this platform)\n");
    } else {
        printf("  -1 >= 1 (TRAP! -1 became 4294967295)\n");
    }

    printf("\n*** Always cast explicitly when comparing signed/unsigned ***\n");
}

/* --------------------------------------------------------------------------
 * Section 6: Practical embedded data types
 * -------------------------------------------------------------------------- */
static void demonstrate_practical_types(void)
{
    print_section("Practical Embedded Data Types");

    /* Boolean-like flag (no _Bool on some embedded compilers) */
    uint8_t is_initialized = 0;  /* false */
    is_initialized = 1;           /* true */
    printf("Flag: %s\n", is_initialized ? "true" : "false");

    /* ADC reading (12-bit ADC on STM32) */
    uint16_t adc_raw = 2048;  /* Mid-scale for 12-bit */
    printf("ADC raw: %u (%.1f%%)\n", adc_raw, (adc_raw / 4095.0) * 100.0);

    /* Temperature in fixed-point (0.1 degree resolution, no floats!) */
    int16_t temp_decicelsius = 237;  /* Represents 23.7 degrees C */
    printf("Temperature: %d.%d C\n", temp_decicelsius / 10, temp_decicelsius % 10);

    /* Bitmask for GPIO pins */
    uint8_t active_pins = 0;
    active_pins |= (1 << 0);  /* Pin 0 */
    active_pins |= (1 << 3);  /* Pin 3 */
    active_pins |= (1 << 7);  /* Pin 7 */
    printf("Active pins mask: 0x%02X (binary: ", active_pins);
    for (int i = 7; i >= 0; i--) {
        printf("%d", (active_pins >> i) & 1);
    }
    printf(")\n");

    /* 32-bit register value */
    uint32_t reg = 0xDEADBEEF;
    printf("Register value: 0x%08X\n", reg);
    printf("  Byte 0 (LSB): 0x%02X\n", (uint8_t)(reg & 0xFF));
    printf("  Byte 1:       0x%02X\n", (uint8_t)((reg >> 8) & 0xFF));
    printf("  Byte 2:       0x%02X\n", (uint8_t)((reg >> 16) & 0xFF));
    printf("  Byte 3 (MSB): 0x%02X\n", (uint8_t)((reg >> 24) & 0xFF));
}

/* --------------------------------------------------------------------------
 * Main
 * -------------------------------------------------------------------------- */
int main(void)
{
    printf("=== Module 01, Example 01: Data Types for Embedded Systems ===\n");

    demonstrate_sizeof();
    demonstrate_limits();
    demonstrate_unsigned_overflow();
    demonstrate_signed_overflow();
    demonstrate_comparison_trap();
    demonstrate_practical_types();

    printf("\n=== End of Example ===\n");
    return 0;
}
