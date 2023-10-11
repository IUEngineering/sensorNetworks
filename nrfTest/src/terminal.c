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


void (*commandCallback)(char *inputBuffer);
char inputBuffer[INPUT_BUFFER_SIZE];
uint8_t inputBufferIndex = 0;



// Returns 2 characters (+ a terminating \0):
// If c is printable, then c followed by a space.
// Else if c is of these: \n \b \r \e, it returns that.
// Else: 2 spaces.
//
// It returns a pointer to a static string, which changes when it is run.
// Very unsafe, but it makes using it much easier than writing to some buffer pointer.
// Also it is static so who cares.
static char* getPrintable(char c);

void terminalSetCallback(void (*callback)(char *inputBuffer)) {
    commandCallback = callback;
}

void terminalInterpretChar(char inChar) {

    // Handle backspaces.
    if(inChar == '\b' && inputBufferIndex) {
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
    // Erase the current line and go to the start of it.
    if(inputBufferIndex != 0) printf("\e[1K\r");

    printf("%s\n", str);

    // Reprint the current inputbuffer so the user can continue typing.
    // If inputBufferIndex == 0 there is nothing to be printed.
    if(inputBufferIndex != 0) {
        // Make sure to stop printing at the current cursor position.
        inputBuffer[inputBufferIndex] = '\0';
        printf("%s", inputBuffer);
    }
}


void terminalPrintHex(uint8_t *buf, uint8_t length, const char *title) {
    // I hate copypasting code, but it seems like this is the only way here.
    // I could of course make a function with a callback,
    // but that seems needlessly complicated for what's happening here.

    // Erase the current line and go to the start of it.
    if(inputBufferIndex != 0) printf("\e[1K\r");

    if(title != NULL) printf("%s\n", title);

    printf("\e[34m");
    for(uint8_t i = 0; i < length; i++) {
        printf("%02x ", buf[i]);
    }
    printf("\e[0m\n");


    // Reprint the current inputbuffer so the user can continue typing.
    // If inputBufferIndex == 0 there is nothing to be printed.
    if(inputBufferIndex != 0) {
        // Make sure to stop printing at the current cursor position.
        inputBuffer[inputBufferIndex] = '\0';
        printf("%s", inputBuffer);
    }
}
// As a string followed by a line of hex values.
void terminalPrintStrex(uint8_t *buf, uint8_t length, const char *title) {

    // Erase the current line and go to the start of it.
    if(inputBufferIndex != 0) printf("\e[1K\r");

    if(title != NULL) printf("%s\n", title);

    printf("\e[34m");
    for(uint8_t i = 0; i < length; i++) {
        printf("%02x ", buf[i]);
    }
    printf("\e[0m\n");

    for(uint8_t i = 0; i < length; i++) {
        printf("%s ", getPrintable(buf[i]));
    }

    printf("\n");

    // Reprint the current inputbuffer so the user can continue typing.
    // If inputBufferIndex == 0 there is nothing to be printed.
    if(inputBufferIndex != 0) {
        // Make sure to stop printing at the current cursor position.
        inputBuffer[inputBufferIndex] = '\0';
        printf("%s", inputBuffer);
    }
}

char* getPrintable(char c) {

    switch(c) {
        case '\n':
            return "\e[32m\\n\e[0m";
        case '\r':
            return "\e[32m\\r\e[0m";
        case '\b':
            return "\e[32m\\b\e[0m";
        case '\e':
            return "\e[32m\\e\e[0m";
        case '\t':
            return "\e[32m\\t\e[0m";
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