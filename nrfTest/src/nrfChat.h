#ifndef _NRFCHAT_H_
#define _NRFCHAT_H_

#include <avr/io.h>
#include <avr/interrupt.h>

// Define the received packet globally so that the received text can be printed
// without interrupting user's the current text input.
extern char receivedPacket[];

void nrfInit(uint16_t channel);
void interpretNewChar(char newChar);
uint8_t getUserInputLength();
char *getCurrentInputBuffer();

#endif // _NRFCHAT_H_