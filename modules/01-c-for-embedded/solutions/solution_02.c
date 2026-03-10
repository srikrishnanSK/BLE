/**
 * @file solution_02.c
 * @brief Solution for Exercise 02: Endianness Conversion
 *
 * Implements byte-swap and endianness conversion functions, detects host
 * byte order, and demonstrates parsing big-endian sensor data.
 *
 * Compile: gcc -Wall -Wextra -std=c99 -o solution_02 solution_02.c
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * detect_endianness
 *
 * Uses a union to examine how a multi-byte value is stored in memory.
 * If the least significant byte is at the lowest address, the system
 * is little-endian.
 *
 * @return 1 if little-endian, 0 if big-endian
 * ----------------------------------------------------------------------- */
int detect_endianness(void)
{
    /*
     * Alternative approach using a pointer cast:
     *   uint16_t val = 0x0001;
     *   uint8_t *byte = (uint8_t *)&val;
     *   return byte[0] == 0x01;
     *
     * The union approach avoids strict aliasing concerns:
     */
    union {
        uint16_t word;
        uint8_t  bytes[2];
    } test;

    test.word = 0x0001;

    /* If byte[0] is 0x01, the LSB is at the lowest address = little-endian */
    return test.bytes[0] == 0x01;
}

/* -----------------------------------------------------------------------
 * swap16 - Reverse the byte order of a 16-bit value
 *
 * Before: [byte1][byte0]
 * After:  [byte0][byte1]
 *
 * Example: 0x1234 -> 0x3412
 * ----------------------------------------------------------------------- */
uint16_t swap16(uint16_t val)
{
    return (uint16_t)((val << 8) | (val >> 8));
}

/* -----------------------------------------------------------------------
 * swap32 - Reverse the byte order of a 32-bit value
 *
 * Before: [byte3][byte2][byte1][byte0]
 * After:  [byte0][byte1][byte2][byte3]
 *
 * Example: 0x12345678 -> 0x78563412
 *
 * We shift each byte to its new position and combine with OR.
 * ----------------------------------------------------------------------- */
uint32_t swap32(uint32_t val)
{
    return ((val & 0xFF000000U) >> 24) |   /* byte 3 -> byte 0 */
           ((val & 0x00FF0000U) >>  8) |   /* byte 2 -> byte 1 */
           ((val & 0x0000FF00U) <<  8) |   /* byte 1 -> byte 2 */
           ((val & 0x000000FFU) << 24);    /* byte 0 -> byte 3 */
}

/* -----------------------------------------------------------------------
 * Host-to-big-endian conversion
 *
 * If the host is already big-endian, these are no-ops.
 * If the host is little-endian, these swap bytes.
 * ----------------------------------------------------------------------- */
static int is_little_endian = -1;   /* Cached result; -1 = not yet tested */

static void init_endian_check(void)
{
    if (is_little_endian < 0) {
        is_little_endian = detect_endianness();
    }
}

uint16_t host_to_be16(uint16_t val)
{
    init_endian_check();
    return is_little_endian ? swap16(val) : val;
}

uint32_t host_to_be32(uint32_t val)
{
    init_endian_check();
    return is_little_endian ? swap32(val) : val;
}

uint16_t be_to_host16(uint16_t val)
{
    /* Conversion is symmetric: the same swap works in both directions */
    return host_to_be16(val);
}

uint32_t be_to_host32(uint32_t val)
{
    return host_to_be32(val);
}

/* -----------------------------------------------------------------------
 * print_memory - Show the byte-level layout of a value in memory
 * ----------------------------------------------------------------------- */
void print_memory(const char *label, const void *ptr, size_t size)
{
    const uint8_t *bytes = (const uint8_t *)ptr;
    printf("%s\n", label);
    for (size_t i = 0; i < size; i++) {
        printf("  Address+%zu: 0x%02X\n", i, bytes[i]);
    }
}

/* -----------------------------------------------------------------------
 * parse_sensor_bytes - Convert big-endian sensor data to host order
 *
 * Many sensors transmit data MSB-first (big-endian). This function
 * assembles individual bytes into a host-order 32-bit value.
 * ----------------------------------------------------------------------- */
uint32_t parse_sensor_bytes(const uint8_t *bytes)
{
    /* Build the value assuming big-endian byte order */
    uint32_t value = ((uint32_t)bytes[0] << 24) |
                     ((uint32_t)bytes[1] << 16) |
                     ((uint32_t)bytes[2] <<  8) |
                     ((uint32_t)bytes[3]);
    return value;
}

/* ======================================================================= */
int main(void)
{
    printf("=== Exercise 02: Endianness Conversion ===\n\n");

    /* -----------------------------------------------------------
     * Part 1: Detect host endianness
     * ----------------------------------------------------------- */
    int le = detect_endianness();
    printf("Host byte order: %s\n\n", le ? "Little-Endian" : "Big-Endian");

    /* -----------------------------------------------------------
     * Part 2: Byte swap functions
     * ----------------------------------------------------------- */
    printf("swap16(0x1234) = 0x%04X\n", swap16(0x1234));
    printf("swap32(0x12345678) = 0x%08X\n\n", swap32(0x12345678));

    /* -----------------------------------------------------------
     * Part 3: Parse big-endian sensor data
     *
     * A sensor sends 4 bytes in big-endian order.
     * Bytes: 0x00, 0x01, 0xC2, 0x08
     * Value: 0x0001C208 = 115,208
     * ----------------------------------------------------------- */
    uint8_t sensor_data[] = {0x00, 0x01, 0xC2, 0x08};
    uint32_t sensor_value = parse_sensor_bytes(sensor_data);

    printf("Sensor bytes (big-endian): ");
    for (int i = 0; i < 4; i++) {
        printf("%02X ", sensor_data[i]);
    }
    printf("\n");
    printf("Parsed value (host order): 0x%08X = %u\n\n", sensor_value, sensor_value);

    /* -----------------------------------------------------------
     * Part 4: Show memory layout
     * ----------------------------------------------------------- */
    uint32_t test_val = 0x12345678;
    print_memory("Memory layout of 0x12345678:", &test_val, sizeof(test_val));
    printf("\n");

    /* -----------------------------------------------------------
     * Part 5: Round-trip verification
     *
     * Converting to big-endian and back should yield the original.
     * ----------------------------------------------------------- */
    uint32_t original = 0x12345678;
    uint32_t to_be    = host_to_be32(original);
    uint32_t back     = be_to_host32(to_be);

    printf("Round-trip check: 0x%08X -> BE(0x%08X) -> host(0x%08X) %s\n",
           original, to_be, back,
           (back == original) ? "[PASS]" : "[FAIL]");

    /* Also test 16-bit */
    uint16_t orig16 = 0xABCD;
    uint16_t rt16   = be_to_host16(host_to_be16(orig16));
    printf("Round-trip 16-bit: 0x%04X -> 0x%04X %s\n",
           orig16, rt16, (rt16 == orig16) ? "[PASS]" : "[FAIL]");

    /* -----------------------------------------------------------
     * Part 6: Additional test - byte array to uint16_t
     * ----------------------------------------------------------- */
    printf("\n--- Bonus: 16-bit big-endian parse ---\n");
    uint8_t be_bytes[] = {0xFE, 0xDC};
    uint16_t parsed16 = ((uint16_t)be_bytes[0] << 8) | be_bytes[1];
    printf("Bytes: %02X %02X -> 0x%04X\n", be_bytes[0], be_bytes[1], parsed16);

    printf("\n=== End of Exercise 02 ===\n");
    return 0;
}
