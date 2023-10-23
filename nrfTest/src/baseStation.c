#include <stdio.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include "serialF0.h"

#include "baseStation.h"
#include "iso.h"
#include "friendList.h"

#define FRIENDS_LIST        0x01
#define RECEIVED_PAYLOAD    0x02
#define RELAYED_PAYLOAD     0x03
#define RECEIVED_BROADCAST  0x04


#define START_SENDING       'c'
#define STOP_SENDING        'e'
#define SEND_SOMETHING      0x01
// Replace this with something that toggles on the blue LED of a node.
#define SOMETHING           "yeah something quack" 


#ifdef DEBUG
    #define DEBUG_PRINTF(fmt, args...) printf(fmt, args)
    #define DEBUG_PRINT(str) printf(str)
#else
    #define DEBUG_PRINTF(fmt, args...) {}
    #define DEBUG_PRINT(fmt, args...) {}
#endif // DEBUG

// Send the friendslist to the RPI
static void sendFriendsList(void);

// Callback for when received data is meant for this node.
static void sendReceivedPayload(uint8_t *payload);

// Callback for when received data is meant for another node.
static void sendRelayedPacket(uint8_t *packet);

// Callback for pings as well as updates.
static void sendBroadcastPacket(uint8_t *packet);

// Initialization of the baseStation program 
void baseStationInit(void) {
    DEBUG_PRINT("I am a base-station\n");

    isoInit(sendReceivedPayload);
    isoSetRelayCallback(sendRelayedPacket);
    isoSetBroadcastCallback(sendBroadcastPacket);

    DEBUG_PRINTF("Waiting till %c is pressed\n", WAIT_FOR_RPY);

    // Turn on green LED. Turned off when sending is enabled. 
    PORTF.OUTSET = PIN0_bm;
}

// The continues loop of the baseStation program 
void baseStationLoop(void) {
    uint8_t count = 0;
    uint8_t doSend = 1;

    while (1) {
        char inChar = uartF0_getc();

        switch(inChar) {
            case STOP_SENDING:
                PORTF.OUTCLR = PIN0_bm;
                doSend = 0;
                break;
            
            case START_SENDING:
                PORTF.OUTCLR = PIN0_bm;
                doSend = 1;
                break;

            case SEND_SOMETHING:
                // Wait for the next byte, which is the destination ID.
                uint16_t receivedByte = uartF0_getc();
                while(receivedByte == UART_NO_DATA) receivedByte = uartF0_getc();

                // Send something.
                isoSendPacket(receivedByte, SOMETHING, sizeof(SOMETHING));
                break;
        }

        isoUpdate();
        count++;
        if(!count && doSend) {
            sendFriendsList();
        }
    }
}

static void sendReceivedPayload(uint8_t *payload) {
    uartF0_putc(RECEIVED_PAYLOAD);
    for (uint8_t i = 0; i < PAYLOAD_SIZE; i++)
        uartF0_putc(payload[i]);
}

static void sendFriendsList(void) {
    uint8_t friendAmount = getFriendAmount();
    friend_t *friends = getFriendsList();

    uartF0_putc(FRIENDS_LIST);
    uartF0_putc(friendAmount);
    
    for(uint8_t i = 0, friend = 0; friend < friendAmount; friend += !!friends[i].id, i++) {
        if(friends[i].id == 0) continue;
        uartF0_putc(friends[i].id);
        uartF0_putc(friends[i].hops);
        uartF0_putc(friends[i].via);
        uartF0_putc(friends[i].trust);
        uartF0_putc(friends[i].active);
    }
}

static void sendRelayedPacket(uint8_t *packet) {
    uartF0_putc(RELAYED_PAYLOAD);
    for (uint8_t i = 0; i < PACKET_SIZE; i++)
        uartF0_putc(packet[i]);
}

static void sendBroadcastPacket(uint8_t *packet) {
    uartF0_putc(RECEIVED_BROADCAST);
    for (uint8_t i = 0; i < PACKET_SIZE; i++)
        uartF0_putc(packet[i]);
}