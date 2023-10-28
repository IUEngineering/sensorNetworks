#ifndef _ISO_H_
#define _ISO_H_

#include <avr/io.h>

#define DEFAULT_CHANNEL 54
#define PAYLOAD_SIZE 31
#define PACKET_SIZE 32


void isoInit(void (*callback)(uint8_t *payload));
uint8_t isoSendPacket(uint8_t dest, uint8_t *payload, uint8_t len);
uint8_t isoUpdate(void);
uint8_t isoGetId(void);

void isoSetRelayCallback(void (*callback)(uint8_t *package));
void isoSetBroadcastCallback(void (*callback)(uint8_t *package));

#endif