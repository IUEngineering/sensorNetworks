#include <stdio.h>
#include "serialF0.h"

#include "baseStation.h"
#include "iso.h"

#define FRIENDS_LIST        0x01
#define RECEIVED_PAYLOAD    0x02

// Callback for when received data is meant for this node
static void messageReceive(uint8_t *payload, uint8_t length);

void baseStationInit(void) {
    isoInit(messageReceive);
}

void baseStationLoop(void) {
    while (1) {
        isoUpdate();
    }
    
}

static void messageReceive(uint8_t *payload, uint8_t length) {
    uartF0_putc(RECEIVED_PAYLOAD);
    uartF0_putc(length);
    for (uint8_t i = 0; i < length; i++)
        uartF0_putc(payload[i]);
}