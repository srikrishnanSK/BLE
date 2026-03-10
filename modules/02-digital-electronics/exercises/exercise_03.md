# Exercise 03: Karnaugh Map Simplification

## Objective

Implement a program that takes a Boolean function specified as a list of
minterms and:

1. Displays the K-map layout
2. Identifies prime implicants
3. Produces a simplified Boolean expression

## Background

Karnaugh maps let you visually simplify Boolean expressions that would be
tedious to reduce algebraically.  While modern synthesis tools automate this
for FPGA/ASIC design, understanding K-maps helps you:

- Manually simplify conditional logic in firmware
- Verify that automated tools produce correct results
- Reason about don't-care conditions in protocol decoders

## Requirements

### Part A -- K-Map Display

```c
/**
 * Display a K-map for a function of 2, 3, or 4 variables.
 *
 * @param num_vars   Number of variables (2-4)
 * @param minterms   Array of minterm indices where F=1
 * @param num_minterms Number of minterms
 * @param dontcares  Array of don't-care minterm indices
 * @param num_dc     Number of don't-cares
 */
void display_kmap(int num_vars, const int *minterms, int num_minterms,
                  const int *dontcares, int num_dc);
```

The display must use Gray code ordering for rows and columns:
- 2-var: rows=A(0,1), cols=B(0,1)
- 3-var: rows=A(0,1), cols=BC(00,01,11,10)
- 4-var: rows=AB(00,01,11,10), cols=CD(00,01,11,10)

### Part B -- Simplification (Quine-McCluskey or manual grouping)

Implement at least a basic simplification that handles:
- Groups of 1, 2, 4, and 8 cells
- Wrapping around K-map edges
- Don't-care terms (can be included in groups but don't require coverage)

```c
void simplify_kmap(int num_vars, const int *minterms, int num_minterms,
                   const int *dontcares, int num_dc);
```

### Part C -- Expression Output

Print the simplified expression using variable names A, B, C, D.
Use prime notation for complements: `A'B + CD'`

## Test Cases

**Test 1 (3 variables):**
```
F(A,B,C) = sum(1,2,5,6,7)
```
K-map:
```
        BC
     00  01  11  10
A=0 | 0 | 1 | 0 | 1 |
A=1 | 0 | 1 | 1 | 1 |
```
Expected simplification: `C' + AB` ... (actually reconsider the minterms)
- m1=001, m2=010, m5=101, m6=110, m7=111
```
     00  01  11  10
A=0 | 0 | 1 | 0 | 1 |
A=1 | 0 | 1 | 1 | 1 |
```
Groups: {m1,m5}=B'C, {m2,m6}=BC', {m5,m7}=AC, {m6,m7}=AB
Simplified: `B'C + BC' + AB` or equivalently `B^C + AB`

**Test 2 (4 variables):**
```
F(A,B,C,D) = sum(0,1,2,5,8,9,10) with don't-cares d(3,7)
```
Expected: See if your program produces `B' + A'C'D`

**Test 3 (4 variables):**
```
F(A,B,C,D) = sum(4,5,6,7,12,13,14,15)
```
This should simplify to just `B`.

## Hints

1. Use a 2D array to represent the K-map, indexed by Gray code.
2. Gray code for 2 bits: {0, 1, 3, 2} (binary: 00, 01, 11, 10).
3. For grouping: start with the largest possible groups (size 8) and
   work down to size 1.
4. Two cells are "adjacent" if they differ in exactly one variable.
5. The Quine-McCluskey algorithm is a systematic approach if you want
   full automation.

## Deliverable

A single C file `solution_03.c`.
