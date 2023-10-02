#include <stdio.h>
#include <avr/interrupt.h>
#include "serialF0.h"

#include "baseStation.h"
#include "iso.h"
#include "friendList.h"

#define FRIENDS_LIST        0x01
#define RECEIVED_PAYLOAD    0x02

// Callback for when received data is meant for this node
static void messageReceive(uint8_t *payload, uint8_t length);

static void printFriendsList(void);

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

static void printFriendsList(void) {
    cli(); // Disable global interrupts
    
    uint16_t numFriends = getNumFriends();
    uint16_t len = 5 * numFriends;

    friend_t *friends = (friend_t*)malloc(sizeof(friend_t) * numFriends);

    getFriends(friends);

    uartF0_putc(FRIENDS_LIST);
    uartF0_putc((uint8_t) len >> 8);
    uartF0_putc((uint8_t) len);

    for (uint16_t i = 0; i < numFriends; i++) {
        uartF0_putc(friends[i].id);
        uartF0_putc(friends[i].hops);
        uartF0_putc(friends[i].via);
        uartF0_putc(friends[i].trust);
        uartF0_putc(friends[i].active);
    }
    sei(); // Enable global interrupts
}