/**
 * Module 02 - Solution 02: Truth Table to Boolean Expression
 *
 * Takes truth tables as input and produces:
 *   1. Canonical Sum of Products (SOP / minterms) expression
 *   2. Canonical Product of Sums (POS / maxterms) expression
 *   3. Nicely formatted truth table display
 *
 * Build:  gcc -Wall -Wextra -std=c99 -o solution_02 solution_02.c
 * Run:    ./solution_02
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* -----------------------------------------------------------------------
 * Part A: Truth Table Representation
 * -----------------------------------------------------------------------*/

#define MAX_VARS 4

typedef struct {
    int      num_vars;              /* 2, 3, or 4          */
    char     var_names[MAX_VARS];   /* e.g., {'A','B','C'} */
    uint8_t  outputs[16];           /* output for each row */
} truth_table_t;

/* -----------------------------------------------------------------------
 * Part D: Print Full Truth Table
 * -----------------------------------------------------------------------*/

void print_truth_table(const truth_table_t *tt)
{
    int num_rows = 1 << tt->num_vars;

    /* Header */
    printf("  ");
    for (int v = 0; v < tt->num_vars; v++) {
        printf("| %c ", tt->var_names[v]);
    }
    printf("| F |\n");

    /* Separator */
    printf("  ");
    for (int v = 0; v < tt->num_vars; v++) {
        printf("+---");
    }
    printf("+---+\n");

    /* Rows */
    for (int row = 0; row < num_rows; row++) {
        printf("  ");
        for (int v = 0; v < tt->num_vars; v++) {
            int bit = (row >> (tt->num_vars - 1 - v)) & 1;
            printf("| %d ", bit);
        }
        printf("| %d |\n", tt->outputs[row]);
    }
    printf("\n");
}

/* -----------------------------------------------------------------------
 * Part B: Sum of Products (SOP)
 *
 * For each row where output = 1, generate a minterm.  A minterm is the
 * product (AND) of all variables, where a variable is complemented if
 * its bit in the row index is 0.
 * -----------------------------------------------------------------------*/

void print_sop(const truth_table_t *tt)
{
    int num_rows = 1 << tt->num_vars;
    bool first = true;

    printf("  SOP: F = ");

    for (int row = 0; row < num_rows; row++) {
        if (tt->outputs[row] != 1) continue;

        if (!first) {
            printf(" + ");
        }
        first = false;

        /* Build the minterm */
        for (int v = 0; v < tt->num_vars; v++) {
            int bit = (row >> (tt->num_vars - 1 - v)) & 1;
            printf("%c", tt->var_names[v]);
            if (!bit) {
                printf("'");
            }
        }
    }

    if (first) {
        printf("0");  /* No minterms -- function is always 0 */
    }

    /* Also print minterm indices */
    printf("\n  Minterms: F = sum(");
    first = true;
    for (int row = 0; row < num_rows; row++) {
        if (tt->outputs[row] != 1) continue;
        if (!first) printf(",");
        printf("%d", row);
        first = false;
    }
    printf(")\n");
}

/* -----------------------------------------------------------------------
 * Part C: Product of Sums (POS)
 *
 * For each row where output = 0, generate a maxterm.  A maxterm is the
 * sum (OR) of all variables, where a variable is complemented if its bit
 * in the row index is 1.
 * -----------------------------------------------------------------------*/

void print_pos(const truth_table_t *tt)
{
    int num_rows = 1 << tt->num_vars;
    bool first = true;

    printf("  POS: F = ");

    for (int row = 0; row < num_rows; row++) {
        if (tt->outputs[row] != 0) continue;

        if (!first) {
            /* No separator needed -- juxtaposition implies AND */
        }
        first = false;

        printf("(");
        for (int v = 0; v < tt->num_vars; v++) {
            int bit = (row >> (tt->num_vars - 1 - v)) & 1;
            if (v > 0) printf("+");
            if (bit) {
                printf("%c'", tt->var_names[v]);
            } else {
                printf("%c", tt->var_names[v]);
            }
        }
        printf(")");
    }

    if (first) {
        printf("1");  /* No maxterms -- function is always 1 */
    }

    /* Also print maxterm indices */
    printf("\n  Maxterms: F = prod(");
    first = true;
    for (int row = 0; row < num_rows; row++) {
        if (tt->outputs[row] != 0) continue;
        if (!first) printf(",");
        printf("%d", row);
        first = false;
    }
    printf(")\n");
}

/* -----------------------------------------------------------------------
 * Main: Test with the three truth tables from the exercise
 * -----------------------------------------------------------------------*/

int main(void)
{
    printf("==============================================\n");
    printf("  Solution 02: Truth Table to Boolean Expression\n");
    printf("==============================================\n\n");

    /* --- Table 1: 2-input AND gate --- */
    printf("--- Table 1: 2-input AND Gate ---\n\n");
    truth_table_t and_gate = {
        .num_vars  = 2,
        .var_names = {'A', 'B'},
        .outputs   = {0, 0, 0, 1}
    };
    print_truth_table(&and_gate);
    print_sop(&and_gate);
    print_pos(&and_gate);
    printf("\n  Expected SOP: AB\n");
    printf("  Expected POS: (A+B)(A+B')(A'+B)\n\n");

    /* --- Table 2: 2-to-1 MUX --- */
    printf("--- Table 2: 2-to-1 MUX ---\n\n");
    truth_table_t mux = {
        .num_vars  = 3,
        .var_names = {'S', 'A', 'B'},
        .outputs   = {0, 0, 1, 1, 0, 1, 0, 1}
    };
    print_truth_table(&mux);
    print_sop(&mux);
    print_pos(&mux);
    printf("\n  Expected simplified SOP: S'A + SB\n\n");

    /* --- Table 3: Majority voter (3 inputs) --- */
    printf("--- Table 3: Majority Voter (3 inputs) ---\n\n");
    truth_table_t majority = {
        .num_vars  = 3,
        .var_names = {'A', 'B', 'C'},
        .outputs   = {0, 0, 0, 1, 0, 1, 1, 1}
    };
    print_truth_table(&majority);
    print_sop(&majority);
    print_pos(&majority);
    printf("\n  Expected simplified SOP: AB + AC + BC\n\n");

    /* --- Bonus: 2-input OR gate (verification) --- */
    printf("--- Bonus: 2-input OR Gate ---\n\n");
    truth_table_t or_gate = {
        .num_vars  = 2,
        .var_names = {'A', 'B'},
        .outputs   = {0, 1, 1, 1}
    };
    print_truth_table(&or_gate);
    print_sop(&or_gate);
    print_pos(&or_gate);
    printf("\n  Expected SOP: A'B + AB' + AB  (simplifies to A+B)\n");
    printf("  Expected POS: (A+B)\n\n");

    /* --- Bonus: XOR gate --- */
    printf("--- Bonus: 2-input XOR Gate ---\n\n");
    truth_table_t xor_gate = {
        .num_vars  = 2,
        .var_names = {'A', 'B'},
        .outputs   = {0, 1, 1, 0}
    };
    print_truth_table(&xor_gate);
    print_sop(&xor_gate);
    print_pos(&xor_gate);
    printf("\n  Expected SOP: A'B + AB'  (= A XOR B)\n");
    printf("  Expected POS: (A+B)(A'+B')\n\n");

    printf("All tests complete.\n");
    return 0;
}
