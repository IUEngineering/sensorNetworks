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

    DEBUG_PRINTF("Waiting till %c is pressed\n", WAIT_FOR_RPY);

    // wait until the RPI is ready for data from the Xmega
    while (!(uartF0_getc() == WAIT_FOR_RPY));
}

// The continues loop of the baseStation program 
void baseStationLoop(void) {

    uint16_t debug = 0;
    
    while (1) {
        debug++;
        if(!(debug%200))
            printf("%d\n", debug);
        isoUpdate();
        sendFriendsList();
    }
}

static void messageReceive(uint8_t *payload, uint8_t length) {
    putchar(RECEIVED_PAYLOAD);
    putchar(length);
    for (uint8_t i = 0; i < length; i++)
        putchar(payload[i]);
}

static void sendFriendsList(void) {
    uint16_t friendAmount = getFriendAmount();
    uint16_t len = 5 * friendAmount;

    // friend_t *friends = (friend_t*)malloc(sizeof(friend_t) * friendAmount);
    friend_t friends[friendAmount];

    if (friends == NULL) {
        DEBUG_PRINT("Yo I need memory\n");
        return;
    }

    getFriends(friends);

    putchar(FRIENDS_LIST);
    putchar((uint8_t) (len >> 8));
    putchar((uint8_t) len);

    for (uint16_t i = 0; i < friendAmount; i++) {
        putchar(friends[i].id);
        putchar(friends[i].hops);
        putchar(friends[i].via);
        putchar(friends[i].trust);
        putchar(friends[i].active);
    }
}