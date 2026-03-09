/**
 * @file 04_functions.c
 * @brief Functions: declaration, definition, pass by value, prototypes, inline.
 *
 * Functions are the building blocks of embedded firmware. This example covers
 * function prototypes, pass-by-value semantics, returning values, and patterns
 * commonly used in embedded systems.
 *
 * Compile: gcc -Wall -Wextra -std=c99 -o 04_functions 04_functions.c
 */

#include <stdint.h>
#include <stdio.h>

/* =======================================================================
 * Section 1: Function Prototypes (Forward Declarations)
 *
 * In embedded C, function prototypes are placed in header files or at
 * the top of source files. This allows the compiler to check argument
 * types and return types at call sites.
 * ======================================================================= */

/* Prototypes — these allow main() to call functions defined later */
static uint16_t adc_to_millivolts(uint16_t raw, uint16_t ref_mv, uint8_t bits);
static int16_t  clamp(int16_t value, int16_t min_val, int16_t max_val);
static void     swap_by_pointer(uint32_t *a, uint32_t *b);
static uint8_t  checksum(const uint8_t *data, uint8_t length);
static void     print_binary(uint8_t value);

/* =======================================================================
 * Section 2: Simple Functions — Pass by Value, Return Values
 * ======================================================================= */

/**
 * @brief Convert a raw ADC reading to millivolts.
 *
 * @param raw     Raw ADC reading
 * @param ref_mv  Reference voltage in millivolts (e.g., 3300 for 3.3V)
 * @param bits    ADC resolution in bits (e.g., 10 for 10-bit, 12 for 12-bit)
 * @return Voltage in millivolts
 *
 * This demonstrates:
 * - Multiple parameters
 * - Return value
 * - Integer-only math (no floats, important for MCUs without FPU)
 */
static uint16_t adc_to_millivolts(uint16_t raw, uint16_t ref_mv, uint8_t bits)
{
    uint32_t max_count = (1UL << bits) - 1;  /* e.g., 1023 for 10-bit */

    /* Use 32-bit intermediate to avoid overflow */
    uint32_t result = ((uint32_t)raw * ref_mv) / max_count;

    return (uint16_t)result;
}

/**
 * @brief Clamp a value to a range [min_val, max_val].
 *
 * Common in embedded for limiting actuator commands, PWM values, etc.
 */
static int16_t clamp(int16_t value, int16_t min_val, int16_t max_val)
{
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

static void demonstrate_basic_functions(void)
{
    printf("\n=== Basic Functions: Pass by Value, Return Values ===\n\n");

    /* ADC conversion examples */
    printf("ADC to Millivolts Conversion:\n");
    printf("  10-bit ADC, 3.3V ref:\n");
    uint16_t test_values[] = {0, 512, 1023};
    for (int i = 0; i < 3; i++) {
        uint16_t mv = adc_to_millivolts(test_values[i], 3300, 10);
        printf("    Raw %4u -> %4u mV\n", test_values[i], mv);
    }

    printf("  12-bit ADC, 3.3V ref:\n");
    uint16_t test12[] = {0, 2048, 4095};
    for (int i = 0; i < 3; i++) {
        uint16_t mv = adc_to_millivolts(test12[i], 3300, 12);
        printf("    Raw %4u -> %4u mV\n", test12[i], mv);
    }

    /* Clamp example */
    printf("\nClamping values to [-100, 100]:\n");
    int16_t inputs[] = {-200, -50, 0, 75, 300};
    for (int i = 0; i < 5; i++) {
        int16_t result = clamp(inputs[i], -100, 100);
        printf("  clamp(%4d) = %4d\n", inputs[i], result);
    }
}

/* =======================================================================
 * Section 3: Pass by Pointer (Simulating Pass by Reference)
 * ======================================================================= */

/**
 * @brief Swap two values via pointers.
 *
 * C only has pass-by-value. To modify the caller's variables,
 * we pass pointers to them.
 */
static void swap_by_pointer(uint32_t *a, uint32_t *b)
{
    uint32_t temp = *a;
    *a = *b;
    *b = temp;
}

/**
 * @brief Parse a 16-bit value into high and low bytes.
 *
 * Uses output parameters (pointers) to return multiple values.
 * This is a very common pattern in embedded drivers.
 */
static void split_uint16(uint16_t value, uint8_t *high, uint8_t *low)
{
    *high = (uint8_t)(value >> 8);
    *low  = (uint8_t)(value & 0xFF);
}

/**
 * @brief Read simulated sensor data, return status.
 *
 * Pattern: function returns error code, data via pointer.
 * This is the standard pattern for embedded driver APIs.
 */
static uint8_t read_sensor(uint16_t *out_value)
{
    /* Simulate reading hardware */
    static uint8_t call_count = 0;
    call_count++;

    if (call_count % 3 == 0) {
        /* Simulate error every 3rd call */
        return 1;  /* Error code */
    }

    *out_value = 1024 + call_count * 100;
    return 0;  /* Success */
}

static void demonstrate_pass_by_pointer(void)
{
    printf("\n=== Pass by Pointer ===\n\n");

    /* Swap demonstration */
    uint32_t x = 0xAAAA, y = 0x5555;
    printf("Before swap: x=0x%04X, y=0x%04X\n", x, y);
    swap_by_pointer(&x, &y);
    printf("After swap:  x=0x%04X, y=0x%04X\n", x, y);

    /* Multiple return values via pointers */
    printf("\nSplit uint16_t into bytes:\n");
    uint16_t value = 0xABCD;
    uint8_t hi, lo;
    split_uint16(value, &hi, &lo);
    printf("  0x%04X -> high=0x%02X, low=0x%02X\n", value, hi, lo);

    /* Error code pattern */
    printf("\nSensor read with error code pattern:\n");
    for (int i = 0; i < 5; i++) {
        uint16_t sensor_val;
        uint8_t err = read_sensor(&sensor_val);
        if (err == 0) {
            printf("  Read %d: Success, value = %u\n", i + 1, sensor_val);
        } else {
            printf("  Read %d: ERROR (code %u)\n", i + 1, err);
        }
    }
}

/* =======================================================================
 * Section 4: Functions with Array Parameters
 * ======================================================================= */

/**
 * @brief Compute XOR checksum of a byte array.
 *
 * When an array is passed to a function, it decays to a pointer.
 * The function has NO way to know the array size — it must be
 * passed as a separate parameter.
 */
static uint8_t checksum(const uint8_t *data, uint8_t length)
{
    uint8_t sum = 0;
    for (uint8_t i = 0; i < length; i++) {
        sum ^= data[i];
    }
    return sum;
}

/**
 * @brief Find minimum value in an array.
 */
static uint16_t find_min(const uint16_t *arr, uint8_t length)
{
    uint16_t min_val = arr[0];
    for (uint8_t i = 1; i < length; i++) {
        if (arr[i] < min_val) {
            min_val = arr[i];
        }
    }
    return min_val;
}

/**
 * @brief Find maximum value in an array.
 */
static uint16_t find_max(const uint16_t *arr, uint8_t length)
{
    uint16_t max_val = arr[0];
    for (uint8_t i = 1; i < length; i++) {
        if (arr[i] > max_val) {
            max_val = arr[i];
        }
    }
    return max_val;
}

/**
 * @brief Fill a buffer with a pattern.
 */
static void fill_buffer(uint8_t *buf, uint8_t length, uint8_t value)
{
    for (uint8_t i = 0; i < length; i++) {
        buf[i] = value;
    }
}

static void demonstrate_array_functions(void)
{
    printf("\n=== Functions with Array Parameters ===\n\n");

    /* Checksum calculation */
    uint8_t packet[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint8_t chk = checksum(packet, sizeof(packet));
    printf("Packet: ");
    for (int i = 0; i < 5; i++) printf("0x%02X ", packet[i]);
    printf("\nChecksum: 0x%02X\n", chk);

    /* Min/Max */
    uint16_t readings[] = {300, 150, 720, 430, 210, 890, 110};
    uint8_t n = sizeof(readings) / sizeof(readings[0]);
    printf("\nSensor readings: ");
    for (int i = 0; i < n; i++) printf("%u ", readings[i]);
    printf("\n  Min: %u\n", find_min(readings, n));
    printf("  Max: %u\n", find_max(readings, n));

    /* Fill buffer */
    uint8_t buf[8];
    fill_buffer(buf, sizeof(buf), 0xAA);
    printf("\nFilled buffer: ");
    for (int i = 0; i < 8; i++) printf("0x%02X ", buf[i]);
    printf("\n");
}

/* =======================================================================
 * Section 5: Static and Helper Functions
 * ======================================================================= */

/**
 * @brief Print a byte in binary format.
 *
 * Helper function — declared static to limit scope to this file.
 * In embedded projects, static functions are "private" to their
 * translation unit.
 */
static void print_binary(uint8_t value)
{
    for (int i = 7; i >= 0; i--) {
        printf("%c", (value & (1 << i)) ? '1' : '0');
    }
}

/**
 * @brief Static local variable: retain state between calls.
 *
 * Useful for counting events, tracking state without global variables.
 */
static uint32_t get_next_id(void)
{
    static uint32_t next_id = 0;  /* Initialized once, persists across calls */
    return next_id++;
}

static void demonstrate_static_functions(void)
{
    printf("\n=== Static Functions and Static Locals ===\n\n");

    /* print_binary is a static helper */
    printf("Binary representation:\n");
    uint8_t vals[] = {0x00, 0x0F, 0xF0, 0xFF, 0xA5};
    for (int i = 0; i < 5; i++) {
        printf("  0x%02X = ", vals[i]);
        print_binary(vals[i]);
        printf("\n");
    }

    /* Static local variable retains state */
    printf("\nID generator (static local variable):\n");
    for (int i = 0; i < 5; i++) {
        printf("  ID: %u\n", get_next_id());
    }
}

/* =======================================================================
 * Section 6: Function Design Best Practices for Embedded
 * ======================================================================= */

/**
 * @brief Map a value from one range to another (like Arduino's map()).
 *
 * Demonstrates a well-designed utility function:
 * - Clear parameter names
 * - No side effects (pure function)
 * - Works with integer-only math
 */
static int32_t map_value(int32_t x,
                         int32_t in_min, int32_t in_max,
                         int32_t out_min, int32_t out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

/**
 * @brief Saturating add — prevents unsigned overflow.
 */
static uint8_t saturating_add_u8(uint8_t a, uint8_t b)
{
    uint16_t sum = (uint16_t)a + (uint16_t)b;
    return (sum > UINT8_MAX) ? UINT8_MAX : (uint8_t)sum;
}

/**
 * @brief Absolute value without branching.
 *
 * Branchless code can be faster on pipelined processors (ARM).
 */
static int32_t abs_branchless(int32_t x)
{
    int32_t mask = x >> 31;  /* All 1s if negative, all 0s if positive */
    return (x ^ mask) - mask;
}

static void demonstrate_best_practices(void)
{
    printf("\n=== Function Design Best Practices ===\n\n");

    /* map_value */
    printf("Map ADC (0-1023) to PWM (0-255):\n");
    uint16_t adc_vals[] = {0, 256, 512, 768, 1023};
    for (int i = 0; i < 5; i++) {
        int32_t pwm = map_value(adc_vals[i], 0, 1023, 0, 255);
        printf("  ADC %4u -> PWM %3d\n", adc_vals[i], (int)pwm);
    }

    /* Saturating arithmetic */
    printf("\nSaturating add (uint8_t):\n");
    printf("  200 + 30  = %u\n", saturating_add_u8(200, 30));
    printf("  200 + 100 = %u (saturated)\n", saturating_add_u8(200, 100));
    printf("  255 + 1   = %u (saturated)\n", saturating_add_u8(255, 1));

    /* Branchless absolute value */
    printf("\nBranchless absolute value:\n");
    int32_t test[] = {-100, -1, 0, 1, 100};
    for (int i = 0; i < 5; i++) {
        printf("  abs(%4d) = %d\n", test[i], abs_branchless(test[i]));
    }
}

/* ======================================================================= */
int main(void)
{
    printf("=== Module 01, Example 04: Functions ===\n");

    demonstrate_basic_functions();
    demonstrate_pass_by_pointer();
    demonstrate_array_functions();
    demonstrate_static_functions();
    demonstrate_best_practices();

    printf("\n=== End of Example ===\n");
    return 0;
}
