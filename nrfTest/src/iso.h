#ifndef _ISO_H_
#define _ISO_H_

#include <avr/io.h>

uint8_t initId(void);
uint8_t isoInterpret(char *message);
void chatSend(char *message);


#endif

