# Exercise 08: Function Pointer Dispatch Table

## Objective

Implement a command dispatch table using function pointers. This is a core
pattern in embedded systems used for command handlers, state machines, menu
systems, protocol parsers, and callback registrations.

## Background

Instead of a long `switch-case` statement, a dispatch table maps command codes
(or strings) to handler functions through an array of structs containing
function pointers:

```c
switch (cmd) {            /* Hard to maintain, extend */
    case 0: do_a(); break;
    case 1: do_b(); break;
    case 2: do_c(); break;
    ...
}

/* vs. */

handler_table[cmd]();     /* Clean, extensible, data-driven */
```

Advantages of dispatch tables:
- Easy to add new commands without modifying the dispatch logic
- Can be stored in flash/ROM (const)
- Enables runtime registration of handlers
- Cleaner code for large command sets

## Task

Build a command processor with these features:

```c
/* Command handler function signature */
typedef int (*cmd_handler_t)(int argc, const char *argv[]);

/* Command table entry */
typedef struct {
    const char *name;           /* Command name string */
    cmd_handler_t handler;      /* Pointer to handler function */
    const char *help;           /* Help text */
    uint8_t min_args;           /* Minimum required arguments */
    uint8_t max_args;           /* Maximum allowed arguments */
} cmd_entry_t;

/* Register a command. Returns 0 on success, -1 if table full. */
int cmd_register(const char *name, cmd_handler_t handler,
                 const char *help, uint8_t min_args, uint8_t max_args);

/* Look up and execute a command by name. Returns handler's return value,
   or -1 if command not found, -2 if wrong argument count. */
int cmd_execute(const char *name, int argc, const char *argv[]);

/* Print all registered commands and their help text. */
void cmd_print_help(void);

/* Parse a command string into name + arguments and execute.
   The input string format is: "command arg1 arg2 ..." */
int cmd_parse_and_execute(char *input);
```

### Implement these example command handlers:

```c
int cmd_led(int argc, const char *argv[]);     /* "led on" / "led off" */
int cmd_gpio(int argc, const char *argv[]);    /* "gpio read 5" / "gpio write 5 1" */
int cmd_adc(int argc, const char *argv[]);     /* "adc read 0" */
int cmd_help(int argc, const char *argv[]);    /* "help" */
int cmd_version(int argc, const char *argv[]); /* "version" */
int cmd_reset(int argc, const char *argv[]);   /* "reset" */
```

## Requirements

1. The dispatch table must be stored in a static array (no dynamic allocation).
2. Command lookup must be case-insensitive.
3. Argument count validation must happen before calling the handler.
4. The string parser must handle multiple spaces between arguments.
5. `cmd_parse_and_execute` must tokenize the input string in-place (using
   `strtok` or manual parsing).
6. Include a `const` dispatch table variant that could be placed in flash.

## Hints

- Use `strcasecmp` (POSIX) or implement a case-insensitive compare.
- For tokenizing: `strtok(input, " \t\r\n")` splits on whitespace.
- Store `argv` as an array of `const char *` pointers into the tokenized input.
- Maximum arguments can be limited to a small number (8-16) for embedded use.
- A `const` table: `static const cmd_entry_t rom_table[] = { ... };`

## Expected Output

```
=== Function Pointer Dispatch Table ===

Registered commands:
  led      [1-2 args]  Control LED (on/off/toggle)
  gpio     [2-3 args]  GPIO read/write
  adc      [1-2 args]  Read ADC channel
  help     [0-0 args]  Show available commands
  version  [0-0 args]  Show firmware version
  reset    [0-0 args]  Reset the system

Executing: "led on"
  LED turned ON [OK]

Executing: "gpio read 5"
  GPIO pin 5 = 0 [OK]

Executing: "gpio write 5 1"
  GPIO pin 5 set to 1 [OK]

Executing: "adc read 0"
  ADC channel 0 = 2048 [OK]

Executing: "unknown_cmd"
  Error: command 'unknown_cmd' not found

Executing: "led"
  Error: 'led' requires 1-2 arguments, got 0

Executing: "help"
  Available commands:
    led      - Control LED (on/off/toggle)
    gpio     - GPIO read/write
    adc      - Read ADC channel
    help     - Show available commands
    version  - Show firmware version
    reset    - Reset the system
```

## Compilation

```bash
gcc -Wall -Wextra -std=c99 -o exercise_08 solution_08.c
```
