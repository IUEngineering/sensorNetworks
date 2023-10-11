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

#define BROADCAST_PIPE "BROAD"
#define BROADCAST_PIPE_INDEX 0

#define PRIVATE_PIPE "TYCH"
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

#define MESSAGE_DESTINATION_ID 0

#define PACKET_SIZE 32

#define HOPS_LIMIT 16


// Counter every 250 ms formula:
// TC_CCA = ((t * F_CPU) / (2* N)) - 1 
#define TC_CCA  15624


// Define list of neighbors (neighbors are friends :)
static uint8_t myId = 0;
static void pingOfLife(void);
static void (*receiveCallback)(uint8_t *payload);
static void send(uint8_t *data);
static void openPrivateWritingPipe(uint8_t destId);
static void timerOverflow(void);
static void interpretPacket(uint8_t *packet, uint8_t receivePipe);

// Nieuwe liedjes :)
//  lighthouse eyezic remix quix
//  kill to fail van IM

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
    nrfOpenWritingPipe((uint8_t *) "HVA01");
    _delay_ms(5);
    nrfOpenReadingPipe(BROADCAST_PIPE_INDEX, (uint8_t *) BROADCAST_PIPE);
    _delay_ms(5);
    nrfOpenReadingPipe(PRIVATE_PIPE_INDEX, privatePipe);
    nrfStartListening();

    initFriendList();

    // Initialize the timer/counter for sending POL's and removing friends.
    TCD0.CTRLB    = TC0_CCAEN_bm | TC_WGMODE_FRQ_gc;
    TCD0.CTRLA    = TC_CLKSEL_DIV1024_gc;
    TCD0.CCA      = TC_CCA;

    receiveCallback = callback;

    printf("My ID is \e[34m0x%02x\e[0m, my pipe is \e[34m%.4s\e[0;31m%c\e[0m, I am set to channel \e[34m%d\e[0m\n",
        myId, (char*)privatePipe, privatePipe[4], DEFAULT_CHANNEL);

    DEBUG_PRINT("\e[31mDebugging is enabled.\e[0m");
}

void isoUpdate(void) {
    // Read the timer counter overflow interrupt flag.
    if(TCD0.INTFLAGS & TC0_OVFIF_bm) timerOverflow();
    // Reset said flag.
    TCD0.INTFLAGS |= TC0_OVFIF_bm;

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

    terminalPrintStrex(packet, PACKET_SIZE, "Sending:");

    if(sendFriend->id == dest) printf("Sending directly to %02x\n\n", dest);
    else printf("Sending to %02x via %02x.\n\n", dest, sendFriend->id);

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
    // Define the list of friends.
    friend_t friends[32];

    // Get the list of friends.
    getFriends(friends);

    // Define and intialize the ping.
    uint8_t ping[32];
    memset(ping, 0, 32);
    ping[0] = myId;
    // ping[1] is set after the while loop.

    // Keep track of indeces.
    uint8_t friendsIndex = 0;

    // Ping build:
    //* Index:      0   1   2   3   4   5   6   7   8   9   10  11
    //* Type:       mID Typ ID0 Hp0 ID1 Hp1 ID2 Hp2 ID3 Hp3 ID4 Hp4
    //
    //  mID = My ID.
    //  Typ = Type (snapshot/update) and length. Byte: [tlllllll].
    //  IDn = nth friend ID.
    //  Hpn = Hops of nth friend.

    // TODO: Handle max hops.
    // Build ping message.
    // The getFriends() function sets the ID of the friend after the last friend to 0,
    // which is how we know when the friend list has ended.
    for(uint8_t i = 2; i < PACKET_SIZE && friends[friendsIndex].id != 0; i += 2) {

        ping[i] = friends[friendsIndex].id;

        // If we know the friend directly, the hops are 0.
        if(friends[friendsIndex].active) ping[i + 1] = 0;
        else ping[i + 1] = friends[friendsIndex].hops;

        friendsIndex++;
    }

    ping[SNAPSHOT_TYPE] = friendsIndex;

    nrfOpenWritingPipe((uint8_t *) BROADCAST_PIPE);
    send(ping);
}


void openPrivateWritingPipe(uint8_t destId) {
    static uint8_t writingPipe[5] = PRIVATE_PIPE;
    writingPipe[4] = destId;
    nrfOpenWritingPipe(writingPipe);
}

void interpretPacket(uint8_t *packet, uint8_t receivePipe) {

    // If it's a Ping of Life:
    if(receivePipe == BROADCAST_PIPE_INDEX) {
        terminalPrintStrex(packet, PACKET_SIZE, "Packet:");

        // Add the sender as a new direct neighbor friend.
        DEBUG_PRINT("Direct:");
        friend_t *directFriend = updateFriend(packet[SNAPSHOT_SENDER_ID], 0, 0);

        // Do we trust this friend enough?
        if(!directFriend->active) return;

        // Delete all friends of the sender.
        removeViaReferences(packet[SNAPSHOT_SENDER_ID]);

        // The packet length is the header length (2) plus the friend data.
        // Every friend takes 2 bytes: ID and Hops, so the friend data is 2 times the amount of friends.
        uint8_t packetLength = 2 + 2 * (packet[SNAPSHOT_TYPE] & SNAPSHOT_TYPE_LENGTH_bm);
        
        // Prevent easily being bricked by array index overflow, caused by data from a malicious or incompetent source.
        if(packetLength > PACKET_SIZE) packetLength = PACKET_SIZE;

        for(uint8_t i = 2; i < packetLength; i += 2)
            // Make sure to not add yourself as friend (prevent becoming schizophrenic).
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
