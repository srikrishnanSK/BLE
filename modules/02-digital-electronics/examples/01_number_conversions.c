/**
 * Module 02 - Example 01: Number Conversion Utilities
 *
 * Demonstrates conversion between binary, octal, decimal, and hexadecimal
 * number systems, two's complement encoding, and IEEE 754 floating-point
 * inspection. All routines are implemented from scratch so you can see the
 * algorithms at work -- no printf("%x") shortcuts.
 *
 * Build:  gcc -Wall -Wextra -std=c99 -o number_conversions 01_number_conversions.c
 * Run:    ./number_conversions
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * Section 1 : Integer-to-string in arbitrary base (2-16)
 * -----------------------------------------------------------------------*/

/**
 * Convert an unsigned 32-bit integer to a string in the given base.
 *
 * @param value   The number to convert.
 * @param base    Radix (2 = binary, 8 = octal, 10 = decimal, 16 = hex).
 * @param buf     Output buffer (must be >= 33 bytes for base-2).
 * @param bufsize Size of the output buffer.
 * @return        Pointer to buf on success, NULL on error.
 */
char *uint32_to_str(uint32_t value, int base, char *buf, size_t bufsize)
{
    static const char digits[] = "0123456789ABCDEF";

    if (base < 2 || base > 16 || bufsize < 2) {
        return NULL;
    }

    /* Build the string in reverse order */
    char tmp[33];
    int  pos = 0;

    if (value == 0) {
        tmp[pos++] = '0';
    } else {
        while (value > 0 && pos < 32) {
            tmp[pos++] = digits[value % base];
            value /= base;
        }
    }

    /* Check that the destination buffer is large enough */
    if ((size_t)pos >= bufsize) {
        return NULL;
    }

    /* Reverse into the caller's buffer */
    for (int i = 0; i < pos; i++) {
        buf[i] = tmp[pos - 1 - i];
    }
    buf[pos] = '\0';

    return buf;
}

/**
 * Print an unsigned 32-bit value in binary with nibble separators.
 * Example output: "0000 1010 0011 1100"
 */
void print_binary_grouped(uint32_t value, int bits)
{
    for (int i = bits - 1; i >= 0; i--) {
        putchar((value >> i) & 1 ? '1' : '0');
        if (i > 0 && i % 4 == 0) {
            putchar(' ');
        }
    }
}

/* -----------------------------------------------------------------------
 * Section 2 : String-to-integer parser (base 2, 8, 10, 16)
 * -----------------------------------------------------------------------*/

/**
 * Parse a numeric string in the given base and return its 32-bit value.
 *
 * Accepts uppercase or lowercase hex digits.  Returns 0 on error and
 * sets *ok to false.
 */
uint32_t str_to_uint32(const char *str, int base, bool *ok)
{
    uint32_t result = 0;
    *ok = true;

    while (*str) {
        int digit;
        char c = *str;

        if (c >= '0' && c <= '9')      digit = c - '0';
        else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
        else { *ok = false; return 0; }

        if (digit >= base) { *ok = false; return 0; }

        result = result * (uint32_t)base + (uint32_t)digit;
        str++;
    }

    return result;
}

/* -----------------------------------------------------------------------
 * Section 3 : Two's complement utilities
 * -----------------------------------------------------------------------*/

/**
 * Negate using two's complement: invert all bits, then add 1.
 *
 * @param value  The value to negate.
 * @param bits   The word size (8, 16, 32).
 * @return       The two's complement negation, masked to `bits` width.
 */
uint32_t twos_complement_negate(uint32_t value, int bits)
{
    uint32_t mask = (bits == 32) ? 0xFFFFFFFF : (1u << bits) - 1;
    return (~value + 1) & mask;
}

/**
 * Sign-extend a value from `bits`-wide to 32-bit signed.
 */
int32_t sign_extend(uint32_t value, int bits)
{
    uint32_t sign_bit = 1u << (bits - 1);
    /* If the sign bit is set, fill the upper bits with 1s */
    if (value & sign_bit) {
        uint32_t mask = (bits == 32) ? 0 : ~((1u << bits) - 1);
        return (int32_t)(value | mask);
    }
    return (int32_t)value;
}

/**
 * Detect whether adding two N-bit signed values overflows.
 * Overflow occurs when both operands have the same sign and the result
 * has a different sign.
 */
bool signed_add_overflows(int32_t a, int32_t b, int bits)
{
    int32_t  sum      = a + b;
    uint32_t sign_bit = 1u << (bits - 1);
    uint32_t mask     = (bits == 32) ? 0xFFFFFFFF : (1u << bits) - 1;

    uint32_t ua = (uint32_t)a & mask;
    uint32_t ub = (uint32_t)b & mask;
    uint32_t us = (uint32_t)sum & mask;

    /* Same sign inputs, different sign result => overflow */
    bool same_sign = !((ua ^ ub) & sign_bit);
    bool diff_result = (ua ^ us) & sign_bit;

    return same_sign && diff_result;
}

/* -----------------------------------------------------------------------
 * Section 4 : IEEE 754 single-precision inspector
 * -----------------------------------------------------------------------*/

/**
 * Break a float into its IEEE 754 components and print them.
 */
void inspect_float(float f)
{
    uint32_t bits;
    memcpy(&bits, &f, sizeof(bits));

    uint32_t sign     = (bits >> 31) & 1;
    uint32_t exponent = (bits >> 23) & 0xFF;
    uint32_t mantissa = bits & 0x7FFFFF;

    printf("Float value:  %g\n", (double)f);
    printf("Hex (raw):    0x%08X\n", bits);
    printf("Binary:       ");
    print_binary_grouped(bits, 32);
    printf("\n");
    printf("Sign:         %u (%s)\n", sign, sign ? "negative" : "positive");
    printf("Exponent:     %u (biased), %d (unbiased)\n",
           exponent, (int)exponent - 127);
    printf("Mantissa:     0x%06X = ", mantissa);
    print_binary_grouped(mantissa, 23);
    printf("\n");

    if (exponent == 0xFF && mantissa != 0)
        printf("Special:      NaN\n");
    else if (exponent == 0xFF && mantissa == 0)
        printf("Special:      %sInfinity\n", sign ? "-" : "+");
    else if (exponent == 0)
        printf("Special:      Denormalized\n");
}

/* -----------------------------------------------------------------------
 * Section 5 : Hex dump utility (common in embedded debugging)
 * -----------------------------------------------------------------------*/

/**
 * Print a memory region as a hex dump with ASCII sidebar.
 */
void hex_dump(const void *data, size_t length)
{
    const uint8_t *p = (const uint8_t *)data;

    for (size_t offset = 0; offset < length; offset += 16) {
        /* Address column */
        printf("%08zX  ", offset);

        /* Hex bytes */
        for (size_t i = 0; i < 16; i++) {
            if (offset + i < length)
                printf("%02X ", p[offset + i]);
            else
                printf("   ");
            if (i == 7) putchar(' ');
        }

        /* ASCII sidebar */
        printf(" |");
        for (size_t i = 0; i < 16 && (offset + i) < length; i++) {
            uint8_t c = p[offset + i];
            putchar((c >= 0x20 && c <= 0x7E) ? c : '.');
        }
        printf("|\n");
    }
}

/* -----------------------------------------------------------------------
 * Section 6 : Gray code conversion
 * -----------------------------------------------------------------------*/

/** Convert binary to Gray code: G = B ^ (B >> 1) */
uint32_t binary_to_gray(uint32_t binary)
{
    return binary ^ (binary >> 1);
}

/** Convert Gray code back to binary */
uint32_t gray_to_binary(uint32_t gray)
{
    uint32_t binary = gray;
    while (gray >>= 1) {
        binary ^= gray;
    }
    return binary;
}

/* -----------------------------------------------------------------------
 * Main : Demonstrate everything
 * -----------------------------------------------------------------------*/

int main(void)
{
    char buf[33];

    printf("=== Number Conversion Utilities ===\n\n");

    /* --- Base conversions --- */
    printf("--- Decimal to Other Bases ---\n");
    uint32_t test_values[] = {0, 1, 42, 127, 255, 1024, 65535, 0xDEADBEEF};
    int n = sizeof(test_values) / sizeof(test_values[0]);

    printf("%-12s %-34s %-12s %-10s\n", "Decimal", "Binary", "Hex", "Octal");
    printf("%-12s %-34s %-12s %-10s\n", "-------", "------", "---", "-----");

    for (int i = 0; i < n; i++) {
        uint32_t v = test_values[i];

        printf("%-12u ", v);

        /* Binary with grouping */
        print_binary_grouped(v, 32);
        printf("  ");

        /* Hex */
        uint32_to_str(v, 16, buf, sizeof(buf));
        printf("0x%-10s ", buf);

        /* Octal */
        uint32_to_str(v, 8, buf, sizeof(buf));
        printf("0%-10s", buf);

        printf("\n");
    }

    /* --- String parsing --- */
    printf("\n--- Parsing Strings to Numbers ---\n");
    bool ok;
    const char *hex_str = "AF3C";
    uint32_t parsed = str_to_uint32(hex_str, 16, &ok);
    printf("Hex \"%s\" = %u (decimal)\n", hex_str, parsed);

    const char *bin_str = "10101010";
    parsed = str_to_uint32(bin_str, 2, &ok);
    printf("Binary \"%s\" = %u (decimal) = 0x%s (hex)\n",
           bin_str, parsed, uint32_to_str(parsed, 16, buf, sizeof(buf)));

    /* --- Two's complement --- */
    printf("\n--- Two's Complement (8-bit) ---\n");
    for (int val = -5; val <= 5; val++) {
        uint32_t raw = (uint32_t)val & 0xFF;
        printf("  %+3d  =>  ", val);
        print_binary_grouped(raw, 8);
        printf("  (0x%s)\n", uint32_to_str(raw, 16, buf, sizeof(buf)));
    }

    printf("\nNegate  42 (8-bit): ");
    uint32_t neg = twos_complement_negate(42, 8);
    print_binary_grouped(neg, 8);
    printf(" = %d\n", sign_extend(neg, 8));

    printf("Negate 100 (8-bit): ");
    neg = twos_complement_negate(100, 8);
    print_binary_grouped(neg, 8);
    printf(" = %d\n", sign_extend(neg, 8));

    /* Overflow detection */
    printf("\n--- Overflow Detection (8-bit signed) ---\n");
    struct { int32_t a, b; } overflow_tests[] = {
        {100, 50}, {-100, -50}, {100, 27}, {-100, -29}, {50, 30}, {-50, -30}
    };
    for (int i = 0; i < 6; i++) {
        int32_t a = overflow_tests[i].a;
        int32_t b = overflow_tests[i].b;
        bool ov = signed_add_overflows(a, b, 8);
        printf("  %+4d + %+4d = %+4d  %s\n",
               (int)a, (int)b, (int)(a + b),
               ov ? "*** OVERFLOW ***" : "(ok)");
    }

    /* --- IEEE 754 --- */
    printf("\n--- IEEE 754 Float Inspection ---\n\n");
    float floats[] = {0.0f, 1.0f, -1.0f, 6.75f, 0.1f, 1.0f/0.0f};
    for (int i = 0; i < 6; i++) {
        inspect_float(floats[i]);
        printf("\n");
    }

    /* --- Gray code --- */
    printf("--- Gray Code ---\n");
    printf("%-10s %-10s %-10s %-10s\n",
           "Decimal", "Binary", "Gray", "Back");
    for (uint32_t i = 0; i < 16; i++) {
        uint32_t g = binary_to_gray(i);
        uint32_t b = gray_to_binary(g);

        printf("  %-8u ", i);
        print_binary_grouped(i, 4);
        printf("      ");
        print_binary_grouped(g, 4);
        printf("      ");
        print_binary_grouped(b, 4);
        printf("\n");
    }

    /* --- Hex dump --- */
    printf("\n--- Hex Dump Example ---\n");
    const char *msg = "Hello, Embedded World!\x00\x01\x02\xFF";
    hex_dump(msg, 27);

    return 0;
}
