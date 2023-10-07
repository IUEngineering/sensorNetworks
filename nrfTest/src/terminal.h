#ifndef _TERMINAL_H_
#define _TERMINAL_H_

#define PRINT_BUFFER_SIZE 256

// Printf variant which prints to the terminal without interrupting current user input.
#define terminalPrintf(fmt, args...) { \
    char __printBuf[PRINT_BUFFER_SIZE]; \
    sprintf(__printBuf, fmt, args); \
    terminalPrintString(__printBuf); \
}



// Set the receiving callback. Runs when enter is pressed.
void terminalSetCallback(void (*callback)(char *inputBuffer));

// Store and interpret a newly typed character.
void terminalInterpretChar(char inChar);


// Prints str without interrupting the current input.
// Requires the buffer to be printable by printf, ending with a \0.
// Automatically appends a newline.
void terminalPrintString(char *str);

// Print the buffer as a line of hex values without interrupting the current input.
// Example:
// ```lua
// Title
// 0a 02 03 34 38 6f
// ```
// You can use title to print a line of text above the hex values,
// which is useful for identifying one line of characters from another.
// Title can be NULL, in which case no title will be printed.
void terminalPrintHex(uint8_t *buf, uint8_t length, const char *title);

// Print the buffer as a line of hex values followed by a line of characters, without interrupting the current input.
// Very useful for printing and analyzing half-printable strings.
// Example:
// ```lua
// Title
// 0a 02 03 34 38 6f
// \n       4  8  o
// ```
// You can use title to print a line of text above the hex values,
// which is useful for identifying one line of characters from another.
// Title can be NULL, in which case no title will be printed.
//
//
// PrintStrex is a cool ass name, and Strex would be a banger username tbh. Sadly it is already taken on minecraft :(.
void terminalPrintStrex(uint8_t *buf, uint8_t length, const char *title);

#endif // _TERMINAL_H_