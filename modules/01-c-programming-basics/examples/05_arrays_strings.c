/**
 * @file 05_arrays_strings.c
 * @brief Arrays, strings, and manual string operations without stdlib.
 *
 * In embedded systems, we often cannot or should not use standard library
 * string functions (they may pull in large code, use malloc, etc.). This
 * example shows how to work with arrays and implement string functions
 * from scratch.
 *
 * Compile: gcc -Wall -Wextra -std=c99 -o 05_arrays_strings 05_arrays_strings.c
 */

#include <stdint.h>
#include <stdio.h>

/* =======================================================================
 * Section 1: Array Declaration, Initialization, and Iteration
 * ======================================================================= */
static void demonstrate_arrays(void)
{
    printf("\n=== Array Declaration and Initialization ===\n\n");

    /* Explicit initialization */
    uint8_t pins[4] = {2, 5, 7, 13};
    printf("Explicit init: ");
    for (int i = 0; i < 4; i++) printf("%u ", pins[i]);
    printf("\n");

    /* Partial initialization — remaining elements are zero */
    uint16_t readings[8] = {100, 200, 300};
    printf("Partial init:  ");
    for (int i = 0; i < 8; i++) printf("%u ", readings[i]);
    printf("\n");

    /* Zero initialization */
    uint32_t counters[4] = {0};
    printf("Zero init:     ");
    for (int i = 0; i < 4; i++) printf("%u ", counters[i]);
    printf("\n");

    /* Size determined by initializer */
    int16_t temps[] = {-10, 0, 15, 23, 37};
    int num_temps = sizeof(temps) / sizeof(temps[0]);
    printf("Auto-sized (%d elements): ", num_temps);
    for (int i = 0; i < num_temps; i++) printf("%d ", temps[i]);
    printf("\n");

    /* Const array (placed in flash on many embedded targets) */
    static const uint8_t sine_quarter[9] = {
        0, 25, 49, 71, 90, 106, 117, 125, 127
    };
    printf("Sine quarter table: ");
    for (int i = 0; i < 9; i++) printf("%u ", sine_quarter[i]);
    printf("\n");

    /* Multi-dimensional array */
    printf("\n2D array (3x3 identity-like matrix):\n");
    uint8_t matrix[3][3] = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };
    for (int r = 0; r < 3; r++) {
        printf("  ");
        for (int c = 0; c < 3; c++) {
            printf("%u ", matrix[r][c]);
        }
        printf("\n");
    }
}

/* =======================================================================
 * Section 2: Array Operations
 * ======================================================================= */

/** @brief Copy src array to dst. */
static void array_copy(uint8_t *dst, const uint8_t *src, uint8_t len)
{
    for (uint8_t i = 0; i < len; i++) {
        dst[i] = src[i];
    }
}

/** @brief Reverse an array in place. */
static void array_reverse(uint8_t *arr, uint8_t len)
{
    for (uint8_t i = 0; i < len / 2; i++) {
        uint8_t temp = arr[i];
        arr[i] = arr[len - 1 - i];
        arr[len - 1 - i] = temp;
    }
}

/** @brief Linear search. Returns index or -1 if not found. */
static int8_t array_find(const uint8_t *arr, uint8_t len, uint8_t target)
{
    for (uint8_t i = 0; i < len; i++) {
        if (arr[i] == target) return (int8_t)i;
    }
    return -1;
}

/** @brief Insertion sort (good for small embedded arrays). */
static void array_sort(uint8_t *arr, uint8_t len)
{
    for (uint8_t i = 1; i < len; i++) {
        uint8_t key = arr[i];
        int8_t j = (int8_t)i - 1;
        while (j >= 0 && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

static void demonstrate_array_operations(void)
{
    printf("\n=== Array Operations ===\n\n");

    uint8_t src[] = {5, 3, 8, 1, 9, 2, 7};
    uint8_t dst[7];
    uint8_t len = sizeof(src) / sizeof(src[0]);

    /* Copy */
    array_copy(dst, src, len);
    printf("Original: ");
    for (int i = 0; i < len; i++) printf("%u ", src[i]);
    printf("\nCopied:   ");
    for (int i = 0; i < len; i++) printf("%u ", dst[i]);
    printf("\n");

    /* Find */
    int8_t idx = array_find(src, len, 8);
    printf("\nFind 8: index = %d\n", idx);
    idx = array_find(src, len, 42);
    printf("Find 42: index = %d (not found)\n", idx);

    /* Sort */
    array_sort(dst, len);
    printf("\nSorted:   ");
    for (int i = 0; i < len; i++) printf("%u ", dst[i]);
    printf("\n");

    /* Reverse */
    array_reverse(dst, len);
    printf("Reversed: ");
    for (int i = 0; i < len; i++) printf("%u ", dst[i]);
    printf("\n");
}

/* =======================================================================
 * Section 3: Array as Circular Buffer (Preview)
 * ======================================================================= */
static void demonstrate_circular_buffer(void)
{
    printf("\n=== Array as Circular Buffer ===\n\n");

    #define CIRC_BUF_SIZE 4
    uint8_t buffer[CIRC_BUF_SIZE];
    uint8_t head = 0;  /* Write index */
    uint8_t count = 0; /* Number of items */

    /* Write values (simulating sensor samples arriving) */
    uint8_t samples[] = {10, 20, 30, 40, 50, 60};
    int num_samples = sizeof(samples) / sizeof(samples[0]);

    for (int i = 0; i < num_samples; i++) {
        buffer[head] = samples[i];
        head = (head + 1) % CIRC_BUF_SIZE;
        if (count < CIRC_BUF_SIZE) count++;

        printf("Added %u -> buffer: [", samples[i]);
        for (int j = 0; j < CIRC_BUF_SIZE; j++) {
            printf("%3u", buffer[j]);
            if (j < CIRC_BUF_SIZE - 1) printf(",");
        }
        printf("] head=%u count=%u\n", head, count);
    }
    #undef CIRC_BUF_SIZE
}

/* =======================================================================
 * Section 4: Strings — Null-Terminated Character Arrays
 * ======================================================================= */
static void demonstrate_strings(void)
{
    printf("\n=== Strings: Null-Terminated char Arrays ===\n\n");

    /* String as char array */
    char greeting[] = "Hello";
    printf("greeting[] = \"%s\"\n", greeting);
    printf("  sizeof   = %zu (includes null terminator)\n", sizeof(greeting));
    printf("  Bytes:     ");
    for (int i = 0; i < (int)sizeof(greeting); i++) {
        if (greeting[i] == '\0')
            printf("\\0 ");
        else
            printf("'%c' ", greeting[i]);
    }
    printf("\n");

    /* String pointer vs array */
    const char *ptr_str = "World";  /* Points to string literal (read-only!) */
    char arr_str[] = "World";       /* Copies string into mutable array */

    printf("\nPointer string: \"%s\" (read-only, in .rodata)\n", ptr_str);
    printf("Array string:   \"%s\" (mutable, on stack)\n", arr_str);

    /* Modify array string */
    arr_str[0] = 'w';
    printf("After modify:   \"%s\"\n", arr_str);
    /* ptr_str[0] = 'w';  <-- Would crash! Undefined behavior. */

    /* Fixed-size string buffer (embedded pattern) */
    char cmd_buffer[16] = {0};  /* Zero-filled, always null-terminated */
    cmd_buffer[0] = 'A';
    cmd_buffer[1] = 'T';
    cmd_buffer[2] = '+';
    cmd_buffer[3] = 'O';
    cmd_buffer[4] = 'K';
    printf("\nCommand buffer: \"%s\" (capacity: %zu)\n", cmd_buffer,
           sizeof(cmd_buffer));
}

/* =======================================================================
 * Section 5: Manual String Functions (No stdlib!)
 * ======================================================================= */

/**
 * @brief Calculate string length (equivalent to strlen).
 *
 * Count characters until the null terminator.
 */
static uint16_t my_strlen(const char *str)
{
    uint16_t len = 0;
    while (str[len] != '\0') {
        len++;
    }
    return len;
}

/**
 * @brief Compare two strings (equivalent to strcmp).
 *
 * Returns 0 if equal, negative if s1 < s2, positive if s1 > s2.
 */
static int16_t my_strcmp(const char *s1, const char *s2)
{
    while (*s1 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (int16_t)((uint8_t)*s1 - (uint8_t)*s2);
}

/**
 * @brief Copy src string to dst (equivalent to strcpy).
 *
 * WARNING: Caller must ensure dst has enough space!
 */
static void my_strcpy(char *dst, const char *src)
{
    while (*src != '\0') {
        *dst = *src;
        dst++;
        src++;
    }
    *dst = '\0';  /* Null-terminate */
}

/**
 * @brief Bounded string copy (equivalent to strncpy, but always null-terminates).
 *
 * This is the SAFE version — always use this in embedded code.
 */
static void my_strncpy(char *dst, const char *src, uint16_t max_len)
{
    uint16_t i;
    for (i = 0; i < max_len - 1 && src[i] != '\0'; i++) {
        dst[i] = src[i];
    }
    dst[i] = '\0';  /* Always null-terminate */
}

/**
 * @brief Concatenate src onto the end of dst.
 */
static void my_strcat(char *dst, const char *src)
{
    /* Find end of dst */
    while (*dst != '\0') {
        dst++;
    }
    /* Copy src */
    while (*src != '\0') {
        *dst = *src;
        dst++;
        src++;
    }
    *dst = '\0';
}

/**
 * @brief Find character in string. Returns pointer or NULL.
 */
static const char *my_strchr(const char *str, char c)
{
    while (*str != '\0') {
        if (*str == c) return str;
        str++;
    }
    return (c == '\0') ? str : NULL;
}

static void demonstrate_string_functions(void)
{
    printf("\n=== Manual String Functions ===\n\n");

    /* strlen */
    const char *test_str = "Embedded";
    printf("my_strlen(\"%s\") = %u\n", test_str, my_strlen(test_str));
    printf("my_strlen(\"\") = %u\n", my_strlen(""));

    /* strcmp */
    printf("\nmy_strcmp results:\n");
    printf("  \"abc\" vs \"abc\" = %d (equal)\n",       my_strcmp("abc", "abc"));
    printf("  \"abc\" vs \"abd\" = %d (less)\n",        my_strcmp("abc", "abd"));
    printf("  \"abd\" vs \"abc\" = %d (greater)\n",     my_strcmp("abd", "abc"));
    printf("  \"ab\"  vs \"abc\" = %d (shorter)\n",     my_strcmp("ab", "abc"));

    /* strcpy */
    char buf[32];
    my_strcpy(buf, "Hello");
    printf("\nmy_strcpy: \"%s\"\n", buf);

    /* strncpy (safe version) */
    char small_buf[6];
    my_strncpy(small_buf, "This is a very long string", sizeof(small_buf));
    printf("my_strncpy (truncated): \"%s\"\n", small_buf);

    /* strcat */
    char concat_buf[32] = "Hello";
    my_strcat(concat_buf, ", ");
    my_strcat(concat_buf, "World!");
    printf("my_strcat: \"%s\"\n", concat_buf);

    /* strchr */
    const char *found = my_strchr("test@email.com", '@');
    if (found) {
        printf("my_strchr('@' in \"test@email.com\"): found at offset %ld\n",
               (long)(found - "test@email.com"));
    }

    found = my_strchr("no-at-sign", '@');
    printf("my_strchr('@' in \"no-at-sign\"): %s\n", found ? "found" : "not found");
}

/* =======================================================================
 * Section 6: Practical String Patterns
 * ======================================================================= */

/**
 * @brief Convert an unsigned integer to decimal string.
 *
 * Useful when printf is not available (bare-metal UART output).
 */
static void uint_to_str(uint32_t value, char *buf, uint8_t buf_size)
{
    if (buf_size == 0) return;

    /* Handle zero */
    if (value == 0) {
        if (buf_size >= 2) {
            buf[0] = '0';
            buf[1] = '\0';
        }
        return;
    }

    /* Build digits in reverse */
    char tmp[11];  /* Max 10 digits for uint32_t + null */
    uint8_t pos = 0;
    while (value > 0 && pos < 10) {
        tmp[pos++] = '0' + (value % 10);
        value /= 10;
    }

    /* Copy reversed into output buffer */
    uint8_t i = 0;
    while (pos > 0 && i < buf_size - 1) {
        buf[i++] = tmp[--pos];
    }
    buf[i] = '\0';
}

/**
 * @brief Convert a hex byte to two ASCII characters.
 */
static void byte_to_hex(uint8_t byte, char *out)
{
    static const char hex_chars[] = "0123456789ABCDEF";
    out[0] = hex_chars[(byte >> 4) & 0x0F];
    out[1] = hex_chars[byte & 0x0F];
    out[2] = '\0';
}

static void demonstrate_practical_patterns(void)
{
    printf("\n=== Practical String Patterns ===\n\n");

    /* Integer to string */
    char buf[16];
    uint32_t values[] = {0, 42, 1234, 4294967295UL};
    printf("uint_to_str:\n");
    for (int i = 0; i < 4; i++) {
        uint_to_str(values[i], buf, sizeof(buf));
        printf("  %u -> \"%s\"\n", values[i], buf);
    }

    /* Byte to hex */
    printf("\nbyte_to_hex:\n");
    uint8_t bytes[] = {0x00, 0x0F, 0xA5, 0xFF};
    for (int i = 0; i < 4; i++) {
        char hex[3];
        byte_to_hex(bytes[i], hex);
        printf("  0x%02X -> \"%s\"\n", bytes[i], hex);
    }

    /* Build a formatted message without printf */
    printf("\nBuilding message without printf:\n");
    char msg[64] = "Temp=";
    char num[12];
    uint_to_str(237, num, sizeof(num));
    my_strcat(msg, num);
    my_strcat(msg, " ADC=");
    uint_to_str(1023, num, sizeof(num));
    my_strcat(msg, num);
    printf("  \"%s\"\n", msg);
}

/* ======================================================================= */
int main(void)
{
    printf("=== Module 01, Example 05: Arrays and Strings ===\n");

    demonstrate_arrays();
    demonstrate_array_operations();
    demonstrate_circular_buffer();
    demonstrate_strings();
    demonstrate_string_functions();
    demonstrate_practical_patterns();

    printf("\n=== End of Example ===\n");
    return 0;
}
