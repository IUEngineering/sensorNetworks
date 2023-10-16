#include <stddef.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <avr/io.h>

#include "terminal.h"
#include "iso.h"
#include "serialF0.h"


// Only has to be 38 for "/send (31 character message)\n"
#define INPUT_BUFFER_SIZE 38

// Define some colors to make things look nicer.
#define HEX_COLOR "\e[34m"      // Blue for hex values.
#define ESCAPE_COLOR "\e[32m"   // Green for escape characters.
#define NO_COLOR "\e[0m"


void (*commandCallback)(char *inputBuffer);
char inputBuffer[INPUT_BUFFER_SIZE];
uint8_t inputBufferIndex = 0;


static char* getPrintable(char c);
static void removeInput(void);
static void rePrintInput(void);

void terminalSetCallback(void (*callback)(char *inputBuffer)) {
    commandCallback = callback;
}

void terminalInterpretChar(char inChar) {

    // Handle backspaces.
    if(inChar == '\b' && inputBufferIndex) {
        // Go back 1, make the char empty, and go back again.
        // If we only print \b the cursor would simply move
        // to the left without removing any characters.
        printf("\b \b");
        inputBufferIndex--;
        return;
    }

    // Handle an enter press.
    if(inChar == '\r') {
        if(inputBufferIndex != 0) inputBuffer[inputBufferIndex] = '\0';
        printf("\n");
        inputBufferIndex = 0;

        // Run the callback function.
        commandCallback(inputBuffer);
        return;
    }

    // Store and echo the character if it's printable and there aren't too many characters already.
    if(inputBufferIndex < INPUT_BUFFER_SIZE && isprint(inChar) && (inputBufferIndex < PAYLOAD_SIZE || inputBuffer[0] == '/')) {
        inputBuffer[inputBufferIndex++] = inChar;
        uartF0_putc(inChar);
    }
}


// This function assumes the string is printable with a printf %s.
void terminalPrint(char *str) {
    removeInput();
    printf("%s\n", str);
    rePrintInput();
}


void terminalPrintHex(uint8_t *buf, uint8_t length, const char *title) {
    removeInput();

    if(title != NULL) printf("%s\n", title);

    printf(HEX_COLOR);
    for(uint8_t i = 0; i < length; i++) {
        printf("%02x ", buf[i]);
    }

    // Print double newline because it looks better.
    printf(NO_COLOR "\n\n");

    rePrintInput();
}
// As a string followed by a line of hex values.
void terminalPrintStrex(uint8_t *buf, uint8_t length, const char *title) {
    removeInput();

    if(title != NULL) printf("%s\n", title);

    // Print the buffer as hex values.
    printf(HEX_COLOR);
    for(uint8_t i = 0; i < length; i++) {
        printf("%02x ", buf[i]);
    }

    // Print the buffer as characters (if possible).
    printf(NO_COLOR "\n");
    for(uint8_t i = 0; i < length; i++) {
        printf("%s ", getPrintable(buf[i]));
    }

    // Print double newline because it looks better.
    printf("\n\n");

    rePrintInput();
}

// Removes the input as preparation for printing something else.
void removeInput(void) {
    // \e[1K = Erase the current line.
    // \r = Go to the start of the current line.
    if(inputBufferIndex != 0) printf("\e[1K\r");
}

// Reprints the input.
// To be used after something else was printed to the terminal.
void rePrintInput(void) {
    // Reprint the current inputbuffer so the user can continue typing.
    // If inputBufferIndex == 0 there is nothing to be printed.
    if(inputBufferIndex != 0) {
        // Make sure to stop printing at the current cursor position.
        inputBuffer[inputBufferIndex] = '\0';
        printf("%s", inputBuffer);
    }
}


// Returns a maximum of 11 characters (including \0 and colors):
// If c is printable, then c followed by a space.
// Else if c is of these: \n \b \r \e \t, it returns that in ESCAPE_COLOR.
// Else: 2 spaces.
//
// It returns a pointer to a static string, which changes when it is run.
// Slightly unsafe, but it makes using it much easier than writing to some buffer pointer.
// Just make sure to only use it once in the same printf.
// This could of course be done using a struct with a fixed array,
// but honestly the function is static and it works fine the way we are using it in this file, so I can't be bothered.
char *getPrintable(char c) {

    switch(c) {
        case '\n':
            return ESCAPE_COLOR "\\n" NO_COLOR;
        case '\r':
            return ESCAPE_COLOR "\\r" NO_COLOR;
        case '\b':
            return ESCAPE_COLOR "\\b" NO_COLOR;
        case '\e':
            return ESCAPE_COLOR "\\e" NO_COLOR;
        case '\t':
            return ESCAPE_COLOR "\\t" NO_COLOR;
        default:
            if(isprint(c)) {
                // Make buffer of 3 chars (2 chars + \0)
                static char ret[3] = "  ";
                ret[0] = c; ret[1] = ' ';
                return ret;
            }
            else return "  ";
    }
}