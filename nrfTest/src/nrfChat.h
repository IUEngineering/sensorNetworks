#ifndef _NRFCHAT_H_
#define _NRFCHAT_H_

#include <avr/io.h>
#include <avr/interrupt.h>

#include "iso.h"


// Define the received packet globally so that the received text can be printed
// without interrupting user's the current text input.
extern char receivedPacket[];

void nrfChatInit(void);

// This contains the while(1) of the nrfChat program
void nrfChatLoop(void);

uint8_t getUserInputLength(void);
char *getCurrentInputBuffer(void);
void printReceivedMessage(void);

#endif // _NRFCHAT_H_