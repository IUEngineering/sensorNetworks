#ifndef _ISO_H_
#define _ISO_H_

#include <avr/io.h>

#define DEFAULT_CHANNEL 54
#define PAYLOAD_SIZE 31

void isoInit(void (*callback)(uint8_t *payload));
uint8_t isoSendPacket(uint8_t dest, uint8_t *payload, uint8_t len);
void isoUpdate(void);
uint8_t isoGetId(void);


#endif