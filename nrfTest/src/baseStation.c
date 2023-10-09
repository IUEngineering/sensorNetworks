#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include "serialF0.h"

#include "baseStation.h"
#include "iso.h"
#include "friendList.h"

#define FRIENDS_LIST        0x01
#define RECEIVED_PAYLOAD    0x02

#define WAIT_FOR_RPY        'c'

#define DEBUG

#ifdef DEBUG
    #define DEBUG_PRINTF(fmt, args...) printf(fmt, args)
    #define DEBUG_PRINT(str) printf(str)
#else
    #define DEBUG_PRINTF(fmt, args...) {}
    #define DEBUG_PRINT(fmt, args...) {}
#endif // DEBUG

// Callback for when received data is meant for this node
static void messageReceive(uint8_t *payload, uint8_t length);

// Send the friendslist to the RPI
static void sendFriendsList(void);

// Initialization of the baseStation program 
void baseStationInit(void) {
    DEBUG_PRINT("I am a base-station\n");

    isoInit(messageReceive);

    // wait until the RPI is ready for data from the Xmega
    while (!(uartF0_getc() == WAIT_FOR_RPY));
}

// The continues loop of the baseStation program 
void baseStationLoop(void) {
    
    while (1) {
        isoUpdate();
        sendFriendsList();
    }
}

static void messageReceive(uint8_t *payload, uint8_t length) {
    uartF0_putc(RECEIVED_PAYLOAD);
    uartF0_putc(length);
    for (uint8_t i = 0; i < length; i++)
        uartF0_putc(payload[i]);
}

static void sendFriendsList(void) {
    // Disable global interrupts to prevent interrupts from 
    // adding friends halve way through this function
    cli(); 

    uint16_t friendAmount = getFriendAmount();
    uint16_t len = 5 * friendAmount;

    friend_t *friends = (friend_t*)malloc(sizeof(friend_t) * friendAmount);

    getFriends(friends);

    uartF0_putc(FRIENDS_LIST);
    uartF0_putc((uint8_t) len >> 8);
    uartF0_putc((uint8_t) len);

    for (uint16_t i = 0; i < friendAmount; i++) {
        uartF0_putc(friends[i].id);
        uartF0_putc(friends[i].hops);
        uartF0_putc(friends[i].via);
        uartF0_putc(friends[i].trust);
        uartF0_putc(friends[i].active);
    }
    sei(); // Enable global interrupts
}