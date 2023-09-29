#ifndef _TERMINAL_H_
#define _TERMINAL_H_

// Set the receiving callback. Runs when enter is pressed.
void terminalSetCallback(void (*callback)(char *inputBuffer));

// Get the buffer to sprintf to.
char *terminalGetBuffer(void);

// Print the buffer.
void terminalPrint(void);

#endif // _TERMINAL_H_