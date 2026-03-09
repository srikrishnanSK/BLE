/**
 * @file 02_bitwise_operators.c
 * @brief Bitwise operators with practical embedded use cases.
 *
 * Bitwise operations are the foundation of embedded programming. Every
 * hardware register interaction uses AND, OR, XOR, NOT, and shifts.
 * This example demonstrates each operator and common embedded patterns.
 *
 * Compile: gcc -Wall -Wextra -std=c99 -o 02_bitwise 02_bitwise_operators.c
 */

#include <stdint.h>
#include <stdio.h>

/* -----------------------------------------------------------------------
 * Helper: Print an 8-bit value in binary
 * ----------------------------------------------------------------------- */
static void print_bin8(const char *label, uint8_t val)
{
    printf("%-30s 0x%02X = ", label, val);
    for (int i = 7; i >= 0; i--) {
        printf("%d", (val >> i) & 1);
        if (i == 4) printf("_");  /* Nibble separator */
    }
    printf("\n");
}

/* -----------------------------------------------------------------------
 * Helper: Print a 32-bit value in binary
 * ----------------------------------------------------------------------- */
static void print_bin32(const char *label, uint32_t val)
{
    printf("%-30s 0x%08X = ", label, val);
    for (int i = 31; i >= 0; i--) {
        printf("%d", (val >> i) & 1);
        if (i > 0 && (i % 4) == 0) printf("_");
    }
    printf("\n");
}

/* =======================================================================
 * Section 1: Basic Bitwise Operators
 * ======================================================================= */
static void demonstrate_basic_operators(void)
{
    printf("\n=== Basic Bitwise Operators ===\n\n");

    uint8_t a = 0b11001010;  /* 0xCA */
    uint8_t b = 0b10110101;  /* 0xB5 */

    print_bin8("a", a);
    print_bin8("b", b);
    printf("\n");

    /* AND: result bit is 1 only if BOTH input bits are 1 */
    print_bin8("a & b  (AND)", a & b);

    /* OR: result bit is 1 if EITHER input bit is 1 */
    print_bin8("a | b  (OR)", a | b);

    /* XOR: result bit is 1 if inputs DIFFER */
    print_bin8("a ^ b  (XOR)", a ^ b);

    /* NOT: inverts all bits */
    print_bin8("~a     (NOT)", ~a);
    print_bin8("~b     (NOT)", ~b);
}

/* =======================================================================
 * Section 2: Shift Operators
 * ======================================================================= */
static void demonstrate_shifts(void)
{
    printf("\n=== Shift Operators ===\n\n");

    uint8_t val = 0x01;

    printf("Left shifts (multiply by 2):\n");
    for (int i = 0; i < 8; i++) {
        print_bin8("  1 << n", (uint8_t)(val << i));
    }

    printf("\nRight shifts (divide by 2):\n");
    val = 0x80;
    for (int i = 0; i < 8; i++) {
        print_bin8("  0x80 >> n", (uint8_t)(val >> i));
    }

    /* Arithmetic vs logical right shift */
    printf("\nArithmetic right shift (signed):\n");
    int8_t signed_val = -16;  /* 0xF0 = 1111_0000 */
    print_bin8("  (int8_t)-16", (uint8_t)signed_val);
    print_bin8("  >> 1 (sign extends)", (uint8_t)(signed_val >> 1));
    print_bin8("  >> 2 (sign extends)", (uint8_t)(signed_val >> 2));
    printf("  Note: Right-shifting signed values sign-extends (fills with 1s).\n");
    printf("  This is implementation-defined! Use unsigned for portable shifts.\n");
}

/* =======================================================================
 * Section 3: Setting, Clearing, Toggling, Checking Bits
 * ======================================================================= */
static void demonstrate_bit_manipulation(void)
{
    printf("\n=== Setting / Clearing / Toggling / Checking Bits ===\n\n");

    uint8_t reg = 0x00;
    print_bin8("Initial register", reg);

    /* SET bit 3: use OR with a mask that has only bit 3 set */
    reg |= (1 << 3);
    print_bin8("After SET bit 3", reg);

    /* SET bit 7 */
    reg |= (1 << 7);
    print_bin8("After SET bit 7", reg);

    /* CLEAR bit 3: AND with the inverse of the bit mask */
    reg &= ~(1 << 3);
    print_bin8("After CLEAR bit 3", reg);

    /* TOGGLE bit 7: XOR with the bit mask */
    reg ^= (1 << 7);
    print_bin8("After TOGGLE bit 7", reg);

    reg ^= (1 << 7);
    print_bin8("After TOGGLE bit 7 again", reg);

    /* CHECK bit 7 */
    reg = 0x85;  /* 1000_0101 */
    print_bin8("Register value", reg);
    printf("  Bit 0 is %s\n", (reg & (1 << 0)) ? "SET" : "CLEAR");
    printf("  Bit 1 is %s\n", (reg & (1 << 1)) ? "SET" : "CLEAR");
    printf("  Bit 2 is %s\n", (reg & (1 << 2)) ? "SET" : "CLEAR");
    printf("  Bit 7 is %s\n", (reg & (1 << 7)) ? "SET" : "CLEAR");
}

/* =======================================================================
 * Section 4: Setting/Clearing Multiple Bits at Once
 * ======================================================================= */
static void demonstrate_multi_bit(void)
{
    printf("\n=== Multi-Bit Operations ===\n\n");

    uint8_t reg = 0xFF;
    print_bin8("Initial", reg);

    /* Clear bits 3:0 (lower nibble) */
    reg &= 0xF0;
    print_bin8("Clear lower nibble", reg);

    /* Set bits 3:0 to 0101 */
    reg |= 0x05;
    print_bin8("Set lower nibble to 0101", reg);

    /* Modify a bit field: change bits [5:4] to value 0b10 */
    printf("\nModifying a 2-bit field [5:4]:\n");
    reg = 0b11001100;
    print_bin8("  Before", reg);

    /* Step 1: Clear the field */
    reg &= ~(0x03 << 4);   /* Clear bits 5:4 */
    print_bin8("  After clear field", reg);

    /* Step 2: Set the new value */
    reg |= (0x02 << 4);    /* Set bits 5:4 to 0b10 */
    print_bin8("  After set field to 0b10", reg);
}

/* =======================================================================
 * Section 5: Practical Embedded Macros
 * ======================================================================= */

/* These macros are used in virtually every embedded project */
#define BIT(n)              (1UL << (n))
#define SET_BIT(reg, n)     ((reg) |= BIT(n))
#define CLEAR_BIT(reg, n)   ((reg) &= ~BIT(n))
#define TOGGLE_BIT(reg, n)  ((reg) ^= BIT(n))
#define CHECK_BIT(reg, n)   (((reg) >> (n)) & 1UL)

/* Bit field manipulation macros */
#define SET_FIELD(reg, mask, shift, val) \
    do { (reg) = ((reg) & ~((mask) << (shift))) | (((val) & (mask)) << (shift)); } while(0)
#define GET_FIELD(reg, mask, shift) \
    (((reg) >> (shift)) & (mask))

static void demonstrate_embedded_macros(void)
{
    printf("\n=== Practical Embedded Bit Macros ===\n\n");

    uint32_t gpio_reg = 0x00000000;

    printf("Using BIT(), SET_BIT(), CLEAR_BIT(), TOGGLE_BIT(), CHECK_BIT():\n\n");
    print_bin32("Initial GPIO register", gpio_reg);

    SET_BIT(gpio_reg, 0);
    print_bin32("SET_BIT(reg, 0)", gpio_reg);

    SET_BIT(gpio_reg, 15);
    print_bin32("SET_BIT(reg, 15)", gpio_reg);

    SET_BIT(gpio_reg, 31);
    print_bin32("SET_BIT(reg, 31)", gpio_reg);

    TOGGLE_BIT(gpio_reg, 15);
    print_bin32("TOGGLE_BIT(reg, 15)", gpio_reg);

    CLEAR_BIT(gpio_reg, 0);
    print_bin32("CLEAR_BIT(reg, 0)", gpio_reg);

    printf("\nChecking individual bits:\n");
    printf("  Bit 0:  %lu\n", CHECK_BIT(gpio_reg, 0));
    printf("  Bit 15: %lu\n", CHECK_BIT(gpio_reg, 15));
    printf("  Bit 31: %lu\n", CHECK_BIT(gpio_reg, 31));

    /* Bit field example: configure a peripheral clock divider */
    printf("\nBit field example (clock divider in bits [10:8]):\n");
    uint32_t clk_reg = 0x00000000;
    #define CLK_DIV_MASK  0x07   /* 3 bits */
    #define CLK_DIV_SHIFT 8

    SET_FIELD(clk_reg, CLK_DIV_MASK, CLK_DIV_SHIFT, 5);
    print_bin32("  Set divider to 5", clk_reg);

    uint32_t divider = GET_FIELD(clk_reg, CLK_DIV_MASK, CLK_DIV_SHIFT);
    printf("  Read divider value: %u\n", divider);
}

/* =======================================================================
 * Section 6: Practical Use Cases
 * ======================================================================= */
static void demonstrate_practical_uses(void)
{
    printf("\n=== Practical Embedded Use Cases ===\n\n");

    /* Use Case 1: Check if a number is a power of 2 */
    printf("Power-of-2 check (n & (n-1) == 0):\n");
    for (uint32_t n = 0; n <= 16; n++) {
        if (n == 0) {
            printf("  %2u: not power of 2 (zero)\n", n);
        } else if ((n & (n - 1)) == 0) {
            printf("  %2u: IS power of 2\n", n);
        } else {
            printf("  %2u: not power of 2\n", n);
        }
    }

    /* Use Case 2: Swap without temporary variable using XOR */
    printf("\nXOR swap:\n");
    uint8_t x = 0xAA, y = 0x55;
    printf("  Before: x=0x%02X, y=0x%02X\n", x, y);
    x ^= y;
    y ^= x;
    x ^= y;
    printf("  After:  x=0x%02X, y=0x%02X\n", x, y);

    /* Use Case 3: Align address to N-byte boundary */
    printf("\nAddress alignment (align up to 4-byte boundary):\n");
    for (uint32_t addr = 0; addr <= 12; addr++) {
        uint32_t aligned = (addr + 3) & ~3UL;
        printf("  0x%02X -> 0x%02X\n", addr, aligned);
    }

    /* Use Case 4: Create a bitmask of N bits */
    printf("\nCreate bitmask of N bits:\n");
    for (int n = 1; n <= 8; n++) {
        uint8_t mask = (uint8_t)((1U << n) - 1);
        print_bin8("  ", mask);
    }

    /* Use Case 5: Isolate lowest set bit */
    printf("\nIsolate lowest set bit (x & -x):\n");
    uint8_t val = 0b01101000;
    print_bin8("  Value", val);
    print_bin8("  Lowest set bit", val & (uint8_t)(-(int8_t)val));

    /* Use Case 6: Clear lowest set bit */
    print_bin8("  Clear lowest set (x & (x-1))", val & (val - 1));
}

/* ======================================================================= */
int main(void)
{
    printf("=== Module 01, Example 02: Bitwise Operators ===\n");

    demonstrate_basic_operators();
    demonstrate_shifts();
    demonstrate_bit_manipulation();
    demonstrate_multi_bit();
    demonstrate_embedded_macros();
    demonstrate_practical_uses();

    printf("\n=== End of Example ===\n");
    return 0;
}
