#ifndef _ISO_H_
#define _ISO_H_

#include <avr/io.h>

#define DEFAULT_CHANNEL 54
#define MAX_PACKET_SIZE 31

void isoInit(void (*callback)(uint8_t *payload, uint8_t length));
void isoSendPacket(uint8_t dest, uint8_t *payload, uint8_t len);


#endif

