#ifndef _ISO_H_
#define _ISO_H_

#include <avr/io.h>

#define STANDARD_CHANNEL 54

void isoInitNrf(void);
uint8_t isoInitId(void);
void isoSendChat(char *message);
char *isoGetReceivedChat(uint8_t *pipe);


#endif

