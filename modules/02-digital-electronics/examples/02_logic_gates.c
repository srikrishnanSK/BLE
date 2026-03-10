/**
 * Module 02 - Example 02: Logic Gate Truth Table Generators
 *
 * Generates and displays truth tables for all fundamental logic gates
 * (AND, OR, NOT, NAND, NOR, XOR, XNOR).  Also demonstrates building
 * combinational circuits from gates: multiplexer, full adder, and a
 * user-defined expression evaluator.
 *
 * Key embedded concepts:
 *   - Bitwise operators in C map directly to logic gates in hardware
 *   - NAND and NOR are "universal" gates (any circuit can be built from
 *     NAND-only or NOR-only)
 *   - Understanding truth tables helps you read/write register bitmasks
 *
 * Build:  gcc -Wall -Wextra -std=c99 -o logic_gates 02_logic_gates.c
 * Run:    ./logic_gates
 */

#include <stdio.h>
#include <stdint.h>

/* -----------------------------------------------------------------------
 * Section 1 : Basic gate functions (single-bit, using uint8_t)
 * -----------------------------------------------------------------------*/

uint8_t gate_and (uint8_t a, uint8_t b) { return a & b; }
uint8_t gate_or  (uint8_t a, uint8_t b) { return a | b; }
uint8_t gate_not (uint8_t a)            { return (~a) & 1; }
uint8_t gate_nand(uint8_t a, uint8_t b) { return (~(a & b)) & 1; }
uint8_t gate_nor (uint8_t a, uint8_t b) { return (~(a | b)) & 1; }
uint8_t gate_xor (uint8_t a, uint8_t b) { return a ^ b; }
uint8_t gate_xnor(uint8_t a, uint8_t b) { return (~(a ^ b)) & 1; }

/* -----------------------------------------------------------------------
 * Section 2 : Truth table generation
 * -----------------------------------------------------------------------*/

/**
 * Print a 2-input gate truth table.
 */
typedef uint8_t (*gate_fn)(uint8_t, uint8_t);

void print_truth_table_2input(const char *name, gate_fn fn)
{
    printf("+---+---+-----+\n");
    printf("| A | B | %4s|\n", name);
    printf("+---+---+-----+\n");
    for (int a = 0; a <= 1; a++) {
        for (int b = 0; b <= 1; b++) {
            printf("| %d | %d |  %d  |\n", a, b, fn(a, b));
        }
    }
    printf("+---+---+-----+\n\n");
}

/**
 * Print a 3-input gate truth table using a user-supplied function.
 */
typedef uint8_t (*gate3_fn)(uint8_t, uint8_t, uint8_t);

void print_truth_table_3input(const char *name, const char *cols,
                              gate3_fn fn)
{
    printf("+---+---+---+%s+\n", "------");
    printf("| A | B | C | %4s |\n", name);
    printf("+---+---+---+%s+\n", "------");
    for (int a = 0; a <= 1; a++) {
        for (int b = 0; b <= 1; b++) {
            for (int c = 0; c <= 1; c++) {
                printf("| %d | %d | %d |  %d   |\n", a, b, c, fn(a, b, c));
            }
        }
    }
    printf("+---+---+---+%s+\n\n", "------");
    (void)cols; /* suppress unused warning */
}

/* -----------------------------------------------------------------------
 * Section 3 : Combinational circuits built from gates
 * -----------------------------------------------------------------------*/

/**
 * 2-to-1 Multiplexer:  OUT = (NOT SEL AND A) OR (SEL AND B)
 *
 * When SEL=0, output follows A.  When SEL=1, output follows B.
 */
uint8_t mux_2to1(uint8_t sel, uint8_t a, uint8_t b)
{
    return gate_or(gate_and(gate_not(sel), a),
                   gate_and(sel, b));
}

/**
 * Full Adder:  Sum = A XOR B XOR Cin
 *              Cout = (A AND B) OR (Cin AND (A XOR B))
 */
typedef struct {
    uint8_t sum;
    uint8_t carry;
} adder_result_t;

adder_result_t full_adder(uint8_t a, uint8_t b, uint8_t cin)
{
    adder_result_t r;
    uint8_t axorb = gate_xor(a, b);

    r.sum   = gate_xor(axorb, cin);
    r.carry = gate_or(gate_and(a, b), gate_and(cin, axorb));

    return r;
}

/**
 * 4-bit ripple-carry adder built from full adders.
 *
 * Returns the 4-bit sum and carry-out.
 */
typedef struct {
    uint8_t sum;    /* 4-bit result */
    uint8_t cout;   /* carry out    */
} adder4_result_t;

adder4_result_t ripple_carry_adder_4bit(uint8_t a, uint8_t b, uint8_t cin)
{
    adder4_result_t result = {0, 0};
    uint8_t carry = cin;

    for (int i = 0; i < 4; i++) {
        uint8_t bit_a = (a >> i) & 1;
        uint8_t bit_b = (b >> i) & 1;

        adder_result_t fa = full_adder(bit_a, bit_b, carry);
        result.sum |= (fa.sum << i);
        carry = fa.carry;
    }

    result.cout = carry;
    return result;
}

/* -----------------------------------------------------------------------
 * Section 4 : De Morgan's theorem verification
 * -----------------------------------------------------------------------*/

/**
 * Verify De Morgan's Laws exhaustively for 2 inputs:
 *   NOT(A AND B) == (NOT A) OR (NOT B)
 *   NOT(A OR  B) == (NOT A) AND (NOT B)
 */
void verify_demorgan(void)
{
    printf("--- De Morgan's Theorem Verification ---\n\n");
    printf("  NOT(A AND B) == (NOT A) OR (NOT B)\n");
    printf("  A | B | NAND(A,B) | NOT_A OR NOT_B | Match?\n");
    printf("  --+---+-----------+----------------+-------\n");

    for (int a = 0; a <= 1; a++) {
        for (int b = 0; b <= 1; b++) {
            uint8_t lhs = gate_nand(a, b);
            uint8_t rhs = gate_or(gate_not(a), gate_not(b));
            printf("  %d | %d |     %d     |       %d        |  %s\n",
                   a, b, lhs, rhs, lhs == rhs ? "YES" : "NO");
        }
    }

    printf("\n  NOT(A OR B) == (NOT A) AND (NOT B)\n");
    printf("  A | B | NOR(A,B)  | NOT_A AND NOT_B | Match?\n");
    printf("  --+---+-----------+-----------------+-------\n");

    for (int a = 0; a <= 1; a++) {
        for (int b = 0; b <= 1; b++) {
            uint8_t lhs = gate_nor(a, b);
            uint8_t rhs = gate_and(gate_not(a), gate_not(b));
            printf("  %d | %d |     %d     |        %d        |  %s\n",
                   a, b, lhs, rhs, lhs == rhs ? "YES" : "NO");
        }
    }
    printf("\n");
}

/* -----------------------------------------------------------------------
 * Section 5 : NAND-only implementations (universality demo)
 * -----------------------------------------------------------------------*/

/*
 * Any logic gate can be built using only NAND gates.
 * This is important in VLSI design and helps explain why NAND flash
 * memory uses that particular gate structure.
 */

/* NOT from NAND: tie both inputs together */
uint8_t not_from_nand(uint8_t a)
{
    return gate_nand(a, a);
}

/* AND from NAND: NAND followed by NOT(NAND) */
uint8_t and_from_nand(uint8_t a, uint8_t b)
{
    return not_from_nand(gate_nand(a, b));
}

/* OR from NAND: NOT each input, then NAND the results */
uint8_t or_from_nand(uint8_t a, uint8_t b)
{
    return gate_nand(not_from_nand(a), not_from_nand(b));
}

/* XOR from NAND: 4 NAND gates */
uint8_t xor_from_nand(uint8_t a, uint8_t b)
{
    uint8_t n1 = gate_nand(a, b);
    uint8_t n2 = gate_nand(a, n1);
    uint8_t n3 = gate_nand(b, n1);
    return gate_nand(n2, n3);
}

void verify_nand_universality(void)
{
    printf("--- NAND Universality Verification ---\n\n");
    printf("  A | B | AND(nand) | OR(nand)  | XOR(nand) | NOT(nand)\n");
    printf("  --+---+-----------+-----------+-----------+----------\n");

    for (int a = 0; a <= 1; a++) {
        for (int b = 0; b <= 1; b++) {
            printf("  %d | %d |     %d     |     %d     |     %d     |  A'=%d B'=%d\n",
                   a, b,
                   and_from_nand(a, b),
                   or_from_nand(a, b),
                   xor_from_nand(a, b),
                   not_from_nand(a),
                   not_from_nand(b));
        }
    }
    printf("\n");
}

/* -----------------------------------------------------------------------
 * Section 6 : Bitwise operations on registers (practical example)
 * -----------------------------------------------------------------------*/

void register_operations_demo(void)
{
    printf("--- Bitwise Register Operations ---\n\n");

    uint8_t reg = 0x00;
    printf("Initial register value: 0x%02X\n", reg);

    /* Set bits 2 and 5 */
    reg |= (1 << 2) | (1 << 5);
    printf("After setting bits 2,5: 0x%02X  (OR mask)\n", reg);

    /* Clear bit 2 */
    reg &= ~(1 << 2);
    printf("After clearing bit 2:   0x%02X  (AND inverted mask)\n", reg);

    /* Toggle bit 5 */
    reg ^= (1 << 5);
    printf("After toggling bit 5:   0x%02X  (XOR mask)\n", reg);

    /* Test bit 5 */
    printf("Bit 5 is %s\n", (reg & (1 << 5)) ? "SET" : "CLEAR");

    /* Toggle bit 5 again */
    reg ^= (1 << 5);
    printf("After toggling bit 5:   0x%02X\n", reg);
    printf("Bit 5 is %s\n\n", (reg & (1 << 5)) ? "SET" : "CLEAR");
}

/* -----------------------------------------------------------------------
 * Main
 * -----------------------------------------------------------------------*/

int main(void)
{
    printf("========================================\n");
    printf("  Logic Gate Truth Table Generator\n");
    printf("========================================\n\n");

    /* -- Basic 2-input gates -- */
    printf("--- 2-Input Gate Truth Tables ---\n\n");
    print_truth_table_2input("AND ", gate_and);
    print_truth_table_2input("OR  ", gate_or);
    print_truth_table_2input("NAND", gate_nand);
    print_truth_table_2input("NOR ", gate_nor);
    print_truth_table_2input("XOR ", gate_xor);
    print_truth_table_2input("XNOR", gate_xnor);

    /* -- NOT gate -- */
    printf("+---+-----+\n");
    printf("| A | NOT |\n");
    printf("+---+-----+\n");
    printf("| 0 |  1  |\n");
    printf("| 1 |  0  |\n");
    printf("+---+-----+\n\n");

    /* -- 2-to-1 MUX -- */
    printf("--- 2-to-1 Multiplexer ---\n");
    printf("OUT = (NOT SEL AND A) OR (SEL AND B)\n\n");
    printf("SEL | A | B | OUT\n");
    printf("----+---+---+----\n");
    for (int sel = 0; sel <= 1; sel++) {
        for (int a = 0; a <= 1; a++) {
            for (int b = 0; b <= 1; b++) {
                printf("  %d | %d | %d |  %d\n", sel, a, b,
                       mux_2to1(sel, a, b));
            }
        }
    }
    printf("\n");

    /* -- Full adder -- */
    printf("--- Full Adder ---\n");
    printf("Sum  = A XOR B XOR Cin\n");
    printf("Cout = (A AND B) OR (Cin AND (A XOR B))\n\n");
    printf("  A | B | Cin | Sum | Cout\n");
    printf("----+---+-----+-----+-----\n");
    for (int a = 0; a <= 1; a++) {
        for (int b = 0; b <= 1; b++) {
            for (int cin = 0; cin <= 1; cin++) {
                adder_result_t r = full_adder(a, b, cin);
                printf("  %d | %d |  %d  |  %d  |  %d\n",
                       a, b, cin, r.sum, r.carry);
            }
        }
    }
    printf("\n");

    /* -- 4-bit ripple carry adder -- */
    printf("--- 4-bit Ripple Carry Adder ---\n\n");
    struct { uint8_t a, b; } add_tests[] = {
        {0x3, 0x5}, {0x7, 0x1}, {0xF, 0x1}, {0xA, 0x5}, {0x0, 0x0}
    };
    for (int i = 0; i < 5; i++) {
        uint8_t a = add_tests[i].a;
        uint8_t b = add_tests[i].b;
        adder4_result_t r = ripple_carry_adder_4bit(a, b, 0);
        printf("  %u + %u = %u (carry=%u)\n",
               a, b, r.sum, r.cout);
    }
    printf("\n");

    /* -- De Morgan's -- */
    verify_demorgan();

    /* -- NAND universality -- */
    verify_nand_universality();

    /* -- Register operations -- */
    register_operations_demo();

    return 0;
}
