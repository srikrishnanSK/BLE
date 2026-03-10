# Exercise 02: Truth Table to Boolean Expression

## Objective

Write a program that takes a truth table as input and produces:
1. The canonical Sum of Products (SOP / minterms) expression
2. The canonical Product of Sums (POS / maxterms) expression
3. A simplified expression (optional bonus)

## Background

Converting between truth tables and Boolean expressions is a fundamental
skill for embedded engineers.  When you need to decode a combination of
GPIO inputs to decide an action, you are effectively implementing a truth
table in software.

## Requirements

### Part A -- Truth Table Representation

Define a truth table for up to 4 input variables using an array:

```c
#define MAX_VARS 4

typedef struct {
    int      num_vars;              /* 2, 3, or 4          */
    char     var_names[MAX_VARS];   /* e.g., {'A','B','C'} */
    uint8_t  outputs[16];          /* output for each row  */
} truth_table_t;
```

### Part B -- Sum of Products (SOP)

Implement a function that prints the SOP form:

```c
void print_sop(const truth_table_t *tt);
```

For each row where output = 1, generate a minterm (product of all variables,
where a 0-input is complemented).

Example for 3 variables {A, B, C} with outputs {0,0,1,0,1,0,1,1}:
```
Minterms: m2, m4, m6, m7
SOP: A'BC' + AB'C' + ABC' + ABC
     simplified: BC' + AB
```

### Part C -- Product of Sums (POS)

```c
void print_pos(const truth_table_t *tt);
```

For each row where output = 0, generate a maxterm (sum of all variables,
where a 1-input is complemented).

### Part D -- Print Full Truth Table

```c
void print_truth_table(const truth_table_t *tt);
```

Nicely formatted table with all inputs and the output.

## Test Cases

Test with these truth tables:

**Table 1: 2-input AND gate**
| A | B | F |
|---|---|---|
| 0 | 0 | 0 |
| 0 | 1 | 0 |
| 1 | 0 | 0 |
| 1 | 1 | 1 |

Expected SOP: `AB`
Expected POS: `(A+B)(A+B')(A'+B)`

**Table 2: 2-to-1 MUX**
| S | A | B | F |
|---|---|---|---|
| 0 | 0 | 0 | 0 |
| 0 | 0 | 1 | 0 |
| 0 | 1 | 0 | 1 |
| 0 | 1 | 1 | 1 |
| 1 | 0 | 0 | 0 |
| 1 | 0 | 1 | 1 |
| 1 | 1 | 0 | 0 |
| 1 | 1 | 1 | 1 |

Expected SOP: `S'AB' + S'AB + SA'B + SAB = S'A + SB`

**Table 3: Majority voter (3 inputs)**
| A | B | C | F |
|---|---|---|---|
| 0 | 0 | 0 | 0 |
| 0 | 0 | 1 | 0 |
| 0 | 1 | 0 | 0 |
| 0 | 1 | 1 | 1 |
| 1 | 0 | 0 | 0 |
| 1 | 0 | 1 | 1 |
| 1 | 1 | 0 | 1 |
| 1 | 1 | 1 | 1 |

Expected SOP: `A'BC + AB'C + ABC' + ABC = AB + AC + BC`

## Hints

1. To generate a minterm for row `i` of `n` variables: iterate over each
   variable; if the variable's bit in `i` is 0, it is complemented.
2. Use `(i >> (num_vars - 1 - v)) & 1` to extract variable `v` from row `i`.
3. For the POS maxterm: it is the dual -- complement variables that are 1.

## Deliverable

A single C file `solution_02.c`.
