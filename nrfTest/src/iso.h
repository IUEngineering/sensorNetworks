#ifndef _ISO_H_
#define _ISO_H_

#include <avr/io.h>

#define STANDARD_CHANNEL 54

void isoInit(void (*callback)(uint8_t *data, uint8_t length));
void isoSend(uint8_t dest, uint8_t *data, uint8_t len);


#endif

