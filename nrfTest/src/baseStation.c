#define F_CPU 32000000UL

#include <stdio.h>
#include <stdlib.h>
#include "serialF0.h"

#include "baseStation.h"
#include "iso.h"
#include "friendList.h"
#include "encrypt.h"

#define SEND_ID             0x00
#define FRIENDS_LIST        0x01
#define RECEIVED_PAYLOAD    0x02
#define RELAYED_PAYLOAD     0x03
#define RECEIVED_BROADCAST  0x04


#define START_SENDING       'c'
#define STOP_SENDING        'e'
#define TRANSMIT_SOMETHING  0x01
// Replace this with something that toggles on the blue LED of a node.
#define SOMETHING           (uint8_t*) "yeah something quack" 
#define SOMETHING_SIZE      21


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

static void sendByteWithParity(uint8_t sendByte);
static void sendParity(void);
static uint8_t parity = 0;

// Enabled by receiving a START_SENDING byte from the PI.
// We will not send anything over the uart if this is 0.
static uint8_t sending = 0;

static char key1[] = "VERON Zendamateur";
static char key2[] = "PI5VLE";


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

    while (1) {
        char inChar = uartF0_getc();

        switch(inChar) {
            case STOP_SENDING:
                PORTF.OUTSET = PIN0_bm;
                sending = 0;
                break;
            
            case START_SENDING:
                if(sending) break;
                PORTF.OUTCLR = PIN0_bm;
                sending = 1;

                sendByteWithParity(SEND_ID);
                sendByteWithParity(SEND_ID);
                sendByteWithParity(isoGetId());
                sendParity();

                break;

            case TRANSMIT_SOMETHING: {
                // Wait for the next byte, which is the destination ID.
                uint16_t receivedByte = uartF0_getc();
                while(receivedByte == UART_NO_DATA) receivedByte = uartF0_getc();

                // Send something.
                isoSendPacket(receivedByte, SOMETHING, SOMETHING_SIZE);
                break;
            }
        }

        if(isoUpdate()) sendFriendsList();
        // isoUpdate();
    }
}

static void sendFriendsList(void) {
    if(!sending) return;

    uint8_t friendAmount = getFriendAmount();
    friend_t *friends = getFriendsList();

    sendByteWithParity(FRIENDS_LIST);
    sendByteWithParity(FRIENDS_LIST);
    sendByteWithParity(friendAmount);
    
    for(uint8_t i = 0, friend = 0; friend < friendAmount; i++) {
        if(friends[i].id == 0) continue;
        friend++;
        sendByteWithParity(friends[i].id);
        sendByteWithParity(friends[i].hops);
        sendByteWithParity(friends[i].via);
        sendByteWithParity(friends[i].trust);
        sendByteWithParity(friends[i].active);
    }

    sendParity();
}

static void sendReceivedPayload(uint8_t *payload) {
    if(!sending) return;

    sendByteWithParity(RECEIVED_PAYLOAD);
    sendByteWithParity(RECEIVED_PAYLOAD);

    uint8_t *encryptedPayload = keysEncrypt(payload, PAYLOAD_SIZE, key1, strlen(key1), key2, strlen(key2));

    for (uint8_t i = 0; i < PAYLOAD_SIZE; i++)
        sendByteWithParity(encryptedPayload[i]);
    
    sendParity();
}

static void sendRelayedPacket(uint8_t *packet) {
    if(!sending) return;

    sendByteWithParity(RELAYED_PAYLOAD);
    sendByteWithParity(RELAYED_PAYLOAD);
    
    for (uint8_t i = 0; i < PACKET_SIZE; i++)
        sendByteWithParity(packet[i]);

    sendParity();
}

// I hate copy pasting code but this is such a small bit of code.
// I don't know how to do it elegantly without defining a third function.
static void sendBroadcastPacket(uint8_t *packet) {
    if(!sending) return;

    sendByteWithParity(RECEIVED_BROADCAST);
    sendByteWithParity(RECEIVED_BROADCAST);

    for (uint8_t i = 0; i < PACKET_SIZE; i++) {
        sendByteWithParity(packet[i]);
    }

    sendParity();

    // When we receive a broadcast, something must've changed about the friend list.
    // This function is always run AFTER updating the friends list, so we get the newest data right to our screen!
    sendFriendsList();
}

void sendByteWithParity(uint8_t sendByte) {
    uartF0_putc(sendByte);
    parity += sendByte + 1;
}

void sendParity(void) {
    uartF0_putc(parity);
    parity = 0;
}