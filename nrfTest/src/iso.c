#include <string.h>
#include <stdlib.h>
#include <avr/interrupt.h>

#include "iso.h"
#include "friendList.h"
#include "nrf24L01.h"
#include "nrf24spiXM2.h"
#include "serialF0.h"
#include "terminal.h"

// Shift team ID 5 bits to the left so that it can be the first(most segnificant) 3 bits of the ID.
#define TEAM_ID 0x02 << 5

#define BROADCAST_PIPE "katje"
#define BROADCAST_PIPE_INDEX 0

#define PRIVATE_PIPE "420P"
#define PRIVATE_PIPE_INDEX 1

#define PRIVATE_MESSAGE_RETRIES     NRF_SETUP_ARC_NORETRANSMIT_gc
#define PRIVATE_MESSAGE_ACK_DELAY   NRF_SETUP_ARD_500US_gc

#define ISO_SEND_SUCCESS 0
#define ISO_FAILED_UNKNOWN_FRIEND -1
#define ISO_FAILED_NOT_TRUSTED -2

#define SNAPSHOT_SENDER_ID      0
#define SNAPSHOT_FIRST_ID       2
#define SNAPSHOT_TYPE           1
#define SNAPSHOT_TYPE_LENGTH_bm 0x7f
#define SNAPSHOT_TYPE_UPDATE_bm 0x80

#define MAX_MULTIPACKET_TIME_INTERVAL 256
#define MAX_MULTIPACKETS 3

#define MESSAGE_DESTINATION_ID 0

#define PACKET_SIZE 32

#define HOPS_LIMIT 16


// Counter every 250 ms formula:
// TC_CCA = ((t * F_CPU) / (2* N)) - 1 
#define TC_CCA  15624

// #define FILLIST 3



// Define list of neighbors (neighbors are friends :)
static uint16_t currentTime = 0;
static uint8_t myId = 0;
static void pingOfLife(void);
static void (*receiveCallback)(uint8_t *payload);
static void send(uint8_t *data);
static void openPrivateWritingPipe(uint8_t destId);
static void timerOverflow(void);
static void interpretPacket(uint8_t *packet, uint8_t receivePipe);

//TODO: optimize the init
void isoInit(void (*callback)(uint8_t *data)) {
    nrfspiInit();
    nrfBegin();
    
    nrfSetRetries(PRIVATE_MESSAGE_ACK_DELAY, NRF_SETUP_ARC_NORETRANSMIT_gc);
    nrfSetPALevel(NRF_RF_SETUP_PWR_18DBM_gc);
    nrfSetDataRate(NRF_RF_SETUP_RF_DR_2M_gc);
    nrfSetCRCLength(NRF_CONFIG_CRC_16_gc);
    nrfSetChannel(DEFAULT_CHANNEL);
    nrfSetAutoAck(0);
    
    nrfClearInterruptBits();
    nrfFlushRx();
    nrfFlushTx();
    
    // Initialize the receiving interrupt (Currently not in use)
    PORTF.INT0MASK |= PIN6_bm;
    PORTF.PIN6CTRL = PORT_ISC_FALLING_gc;
    // PORTF.INTCTRL |= (PORTF.INTCTRL & ~PORT_INT0LVL_gm) | PORT_INT0LVL_LO_gc;
    

    // Read the ID from the GPIO pins.
    // Enable pullup for the first 4 pins of PORT D
    // Also invert their inputs. (We're using pullup, with a button to ground.)
    PORTD.DIRCLR = 0b1111;
    PORTCFG.MPCMASK = 0b1111; 
    PORTD.PIN0CTRL = PORT_INVEN_bm | PORT_OPC_PULLUP_gc;

    // It apparently takes some time for MPCMASK to do its thing.
    _delay_loop_1(5);

    myId = PORTD.IN & 0b1111;
    myId |= TEAM_ID;
    uint8_t privatePipe[5] = PRIVATE_PIPE;
    privatePipe[4] = myId;
    
    nrfPowerUp();

    // TODO: Check if these delays are necessary.
    _delay_ms(5);
    nrfOpenReadingPipe(BROADCAST_PIPE_INDEX, (uint8_t *) BROADCAST_PIPE);
    _delay_ms(5);
    nrfOpenReadingPipe(PRIVATE_PIPE_INDEX, privatePipe);
    nrfStartListening();

    initFriendList();
#ifdef FILLIST
    for(uint8_t i = 1; i <= 20; i++) {
        updateFriend(i, 0, 0);
        updateFriend(i, 0, 0);
        updateFriend(i, 0, 0);
        updateFriend(i, 0, 0);
        updateFriend(i, 0, 0);
    }
    for(uint8_t i = 0; i < 20; i++) {
        updateFriend(i + 15, i % 3 + 1, i / 2);
    }

#endif

    // Initialize the timer/counter for sending POL's (snapshots) and removing friends.
    TCD0.CTRLA  = TC_CLKSEL_DIV1024_gc;
    TCD0.CTRLB  = TC0_CCAEN_bm | TC_WGMODE_FRQ_gc;
    TCD0.CCA    = TC_CCA;

    TCD1.CTRLA  = TC_CLKSEL_DIV256_gc;
    TCD1.CTRLB  = TC1_CCAEN_bm | TC_WGMODE_FRQ_gc;
    TCD1.CCA    = 16; // Random value I don't really care tbh. It has to be low enough to not fill a uint16_t within like 5 seconds.

    receiveCallback = callback;

    DEBUG_PRINT("\e[31mDebugging is enabled.\e[0m");
}

void isoUpdate(void) {
    // Read the first timer counter overflow interrupt flag.
    if(TCD0.INTFLAGS & TC0_OVFIF_bm) timerOverflow();
    TCD0.INTFLAGS |= TC0_OVFIF_bm; // Reset the flag.

    // Read the second timer counter overflow interrupt flag.
    if(TCD1.INTFLAGS & TC1_OVFIF_bm) currentTime++;
    TCD1.INTFLAGS |= TC1_OVFIF_bm; // Reset the flag.

    uint8_t receivePipe = 0xff;

    // Did I receive something? 
    if(nrfAvailable(&receivePipe)) {
        uint8_t packet[PACKET_SIZE];

        // Put received data into a buffer.
        nrfRead(packet, PACKET_SIZE);
        interpretPacket(packet, receivePipe);
    }
}

void timerOverflow(void) {
    // Cut the
    pingOfLife();
    // you've been feeding my veins.
    #ifdef FILLIST
    return;
    #endif
    friendTimeTick();
}

uint8_t isoSendPacket(uint8_t dest, uint8_t *payload, uint8_t len) {

    // Prevent segfault
    if(len > PAYLOAD_SIZE) len = PAYLOAD_SIZE;

    // Build packet
    uint8_t packet[PACKET_SIZE];
    memset(packet, 0, PACKET_SIZE);
    
    packet[MESSAGE_DESTINATION_ID] = dest;
    memcpy(packet + 1, payload, len);

    // Find the destination
    friend_t *sendFriend = findFriend(dest);
    if(sendFriend == NULL) {
        return ISO_FAILED_UNKNOWN_FRIEND;
    }

    // Check if the friend is active. If it isn't, send via another friend.
    if(sendFriend->active == 0) sendFriend = findFriend(sendFriend->via);

    // Does the via friend exist?
    if(sendFriend == NULL) {
        return ISO_FAILED_UNKNOWN_FRIEND;
    }       

    // Turn on acknowledgements and send the packet.
    openPrivateWritingPipe(sendFriend->id);
    send(packet);

    return 0;
}

void send(uint8_t *data) {
    nrfStopListening();
    // The datasheet says it takes 130 us to switch out of listening mode.
    _delay_us(130);

    // The data is always 32 bytes because for some reason it was decided to not use dynamic payload.
    nrfWrite(data, PACKET_SIZE);
    nrfStartListening();
}

void pingOfLife(void) {
    // Ping build:
    //* Index:  0    1    2    3    4    5    6    7    8    8    9    8
    //* Type:   myID Typ  ID0  Hop0 ID1  Hop1 ID2  Hop2 ID3  Hop3 ID4  Hop4
    
    // TODO: Handle max hops.

    // Get the list of friends.
    friend_t friends[MAX_FRIENDS + 1]; // One larger for id=0 terminator. 
    getFriends(friends);

    uint8_t friendsIndex = 0;
    uint8_t pings[MAX_MULTIPACKETS][PACKET_SIZE] = {0};
    uint8_t multipacketIndex = 0;
    
    // The getFriends() function sets the ID of the friend after the last friend to 0, so we loop until that friend is encountered.
    // Every time this loop loops, 1 ping packet is built and sent.
    do {
        // Put own id at the start.
        pings[multipacketIndex][0] = myId;

        // Build a ping:
        uint8_t pingIndex = 2;
        for(; pingIndex < PACKET_SIZE && friends[friendsIndex].id != 0; pingIndex += 2) {
            // Put the ID into the ping.
            pings[multipacketIndex][pingIndex] = friends[friendsIndex].id;

            // Put the hops into the ping.
            // If we know the friend directly, the hops are 0.
            if(friends[friendsIndex].active) 
                pings[multipacketIndex][pingIndex + 1] = 0;
            else 
                pings[multipacketIndex][pingIndex + 1] = friends[friendsIndex].hops;

            friendsIndex++;
        }

        // Set the length byte of the ping which is required because I was forced to turn off dynamic length (yes I'm still salty).
        pings[multipacketIndex][SNAPSHOT_TYPE] = friendsIndex;

        // Onto the next packet.
        multipacketIndex++;
    }
    while(friends[friendsIndex].id && multipacketIndex <= MAX_MULTIPACKETS);

    // Open the public pipe.
    nrfOpenWritingPipe((uint8_t *) BROADCAST_PIPE);
    
    // With only a maximum of 3 multipackets, I'm pretty doing it this way is easier to read than doing it with a for loop.
    // I'm also pretty sure it's not that much less efficient (if at all).
    send(pings[0]);
    if(multipacketIndex > 1) send(pings[1]);
    if(multipacketIndex > 2) send(pings[2]);
}


void openPrivateWritingPipe(uint8_t destId) {
    static uint8_t writingPipe[5] = PRIVATE_PIPE;
    writingPipe[4] = destId;
    nrfOpenWritingPipe(writingPipe);
}

void interpretPacket(uint8_t *packet, uint8_t receivePipe) {


    // If it's a Ping of Life (snapshot):
    if(receivePipe == BROADCAST_PIPE_INDEX) {
        #ifdef FILLIST
        return;
        #endif

        // Add the sender as a new direct neighbor friend.
        friend_t *directFriend = updateFriend(packet[SNAPSHOT_SENDER_ID], 0, 0);

        uint8_t isMultiPacket = currentTime - directFriend->lastPingTime < MAX_MULTIPACKET_TIME_INTERVAL;

        // Update the friend's timer.
        directFriend->lastPingTime = currentTime;


        // Do we trust this friend enough?
        if(!directFriend->active) return;

        // Remove all the friend's previous via links if this is a brand new packet.
        // There will be a small period in which no data can be sent to these, which we'll just have to accept for now.
        if(!isMultiPacket) {
            // Delete all friends of the sender if it's not.
            removeViaReferences(packet[SNAPSHOT_SENDER_ID]);
        }

        // The packet length is the header length (2) plus the friend data.
        // Every friend takes 2 bytes: ID and Hops, so the friend data is 2 times the amount of friends.
        uint8_t packetLength = 2 + 2 * (packet[SNAPSHOT_TYPE] & SNAPSHOT_TYPE_LENGTH_bm);
        
        // Prevent easily being bricked by array index overflow, caused by data from a malicious or incompetent source.
        if(packetLength > PACKET_SIZE) packetLength = PACKET_SIZE;

        for(uint8_t i = 2; i < packetLength; i += 2)
            // Make sure to not add yourself as friend (prevents becoming schizophrenic).
            if(packet[i] != myId)
                updateFriend(packet[i], packet[i + 1] + 1, packet[SNAPSHOT_SENDER_ID]);
    
        return;
    }

    PORTF.OUTTGL = PIN1_bm;

    // Is for me???
    if(packet[MESSAGE_DESTINATION_ID] == myId) {
        receiveCallback(packet + 1);
    }

    // If it's a message for someone else.
    else {
        // Relay packet
        isoSendPacket(packet[0], packet + 1, PACKET_SIZE - 1);
    }
}


uint8_t isoGetId(void) {
    return myId;
}
