#include <stdio.h>
#include <stdlib.h>
#include "serialF0.h"

#include "baseStation.h"
#include "iso.h"
#include "friendList.h"

#define FRIENDS_LIST        0x01
#define RECEIVED_PAYLOAD    0x02

#define WAIT_FOR_RPY        'c'


#ifdef DEBUG
    #define DEBUG_PRINTF(fmt, args...) printf(fmt, args)
    #define DEBUG_PRINT(str) printf(str)
#else
    #define DEBUG_PRINTF(fmt, args...) {}
    #define DEBUG_PRINT(fmt, args...) {}
#endif // DEBUG

// Callback for when received data is meant for this node
static void messageReceive(uint8_t *payload);

// Send the friendslist to the RPI
static void sendFriendsList(void);

// Initialization of the baseStation program 
void baseStationInit(void) {
    DEBUG_PRINT("I am a base-station\n");

    isoInit(messageReceive);

    DEBUG_PRINTF("Waiting till %c is pressed\n", WAIT_FOR_RPY);
}

// The continues loop of the baseStation program 
void baseStationLoop(void) {
    uint8_t count = 0;
    uint8_t doSend = 1;

    while (1) {
        char inChar = uartF0_getc();
        if(inChar == 'e') {
            PORTF.OUTCLR = PIN0_bm;
            doSend = 0;
        }
        else if(inChar == 'c') {
            PORTF.OUTSET = PIN0_bm;
            doSend = 1;
        }

        isoUpdate();
        count++;
        if(!count && doSend) {
            sendFriendsList();
        }
    }
}

static void messageReceive(uint8_t *payload) {
    uartF0_putc(RECEIVED_PAYLOAD);
    for (uint8_t i = 0; i < PAYLOAD_SIZE; i++)
        uartF0_putc(payload[i]);
}

static void sendFriendsList(void) {
    uint8_t friendAmount = getFriendAmount();
    uint8_t listLength = 0;
    friend_t *friends = getFriendsList(&listLength);

    uartF0_putc(FRIENDS_LIST);
    uartF0_putc(friendAmount);

    for (uint8_t i = 0; i < listLength; i++) {
        if(friends[i].id == 0) continue;
        uartF0_putc(friends[i].id);
        uartF0_putc(friends[i].hops);
        uartF0_putc(friends[i].via);
        uartF0_putc(friends[i].trust);
        uartF0_putc(friends[i].active);
    }
}