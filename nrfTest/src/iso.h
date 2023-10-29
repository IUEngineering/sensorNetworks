#ifndef _ISO_H_
#define _ISO_H_

#include <avr/io.h>

#define DEFAULT_CHANNEL 54
#define PAYLOAD_SIZE 31
#define PACKET_SIZE 32

// Shift team ID 5 bits to the left so that it can be the first(most segnificant) 3 bits of the ID.
#define TEAM_ID 0x02 << 5
#define TEAM_ID_BITMASK 0x1f

void isoInit(void (*callback)(uint8_t *payload));
uint8_t isoSendPacket(uint8_t dest, uint8_t *payload, uint8_t len);
uint8_t isoUpdate(void);
uint8_t isoGetId(void);

void isoSetRelayCallback(void (*callback)(uint8_t *package));
void isoSetBroadcastCallback(void (*callback)(uint8_t *package));

#endif