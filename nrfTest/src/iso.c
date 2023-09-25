#include <string.h>
#include <stdlib.h>
#include <avr/interrupt.h>

#include "iso.h"
#include "friendList.h"
#include "nrf24L01.h"
#include "nrf24spiXM2.h"
#include "serialF0.h"

// Shift team ID 5 bits to the left so that it can be the first(most segnificant) 3 bits of the ID.
#define TEAM_ID 0x02 << 5

#define BROADCAST_PIPE "BROAD"
#define BROADCAST_PIPE_INDEX 0

#define PRIVATE_PIPE "TYCH"
#define PRIVATE_PIPE_INDEX 1

// Counter every 250 ms formula:
// TC_CCA = ((t * F_CPU) / (2* N)) - 1 
#define TC_CCA  15624


// Define list of neighbors (neighbors are friends :)
static uint8_t myId = 0;
static void pingOfLife(void);
static void (*receiveCallback)(uint8_t *data, uint8_t length);
static void send(uint8_t *data, uint8_t len);
static void openPrivateWritingPipe(uint8_t destId);


//TODO: optimize the init
void isoInit(void (*callback)(uint8_t *data, uint8_t length)) {
    nrfspiInit();
    nrfBegin();
    
    nrfSetRetries(NRF_SETUP_ARD_1000US_gc, NRF_SETUP_ARC_NORETRANSMIT_gc);
    nrfSetPALevel(NRF_RF_SETUP_PWR_0DBM_gc);
    nrfSetDataRate(NRF_RF_SETUP_RF_DR_2M_gc);
    nrfSetCRCLength(NRF_CONFIG_CRC_16_gc);
    nrfSetChannel(STANDARD_CHANNEL);
    nrfSetAutoAck(0);
    nrfEnableDynamicPayloads();
    
    nrfClearInterruptBits();
    nrfFlushRx();
    nrfFlushTx();
    
    // Initialize the receiving interrupt
    PORTF.INT0MASK |= PIN6_bm;
    PORTF.PIN6CTRL = PORT_ISC_FALLING_gc;
    PORTF.INTCTRL |= (PORTF.INTCTRL & ~PORT_INT0LVL_gm) | PORT_INT0LVL_LO_gc;
    

    // Read the ID from the GPIO pins.
    // Enable pullup for the first 4 pins of PORT D
    PORTCFG.MPCMASK = 0b1111; 
    PORTD.PIN0CTRL = PORT_OPC_PULLUP_gc;

    PORTD.DIRCLR = 0b1111;
    myId = ~PORTD.IN & 0b1111;
    myId |= TEAM_ID;
    uint8_t privatePipe[5] = PRIVATE_PIPE;
    privatePipe[4] = myId;
    
    nrfPowerUp();

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
    TCD0.CTRLA    = TC_CLKSEL_DIV256_gc;
    TCD0.CCA      = TC_CCA;
    TCD0.INTCTRLA |= TC_OVFINTLVL_LO_gc;

    receiveCallback = callback;

    printf("My ID is 0x%02x, my pipe is %c%c%c%c\e[0;31m%c\e[0m\n", \
        myId, privatePipe[0], privatePipe[1], privatePipe[2], privatePipe[3], privatePipe[4]);
}

void isoSend(uint8_t dest, uint8_t *data, uint8_t len) {
    // Prevent segfault
    if(len > 31) len = 31;

    uint8_t sendData[32];
    sendData[0] = dest;
    memcpy(sendData + 1, data, len);

    // Check if it's a direct neighbor. If it isn't, send it to the via neighbor.
    friend_t *sendFriend = findFriend(dest);
    if(sendFriend == NULL) {
        printf("I don't know friend %02x\n\n", dest);
        return;
    }
    if(sendFriend->hops != 0) sendFriend = findFriend(sendFriend->via);

    TCD0.CTRLA    = TC_CLKSEL_OFF_gc;
    openPrivateWritingPipe(sendFriend->id);
    send(sendData, len + 1);
    TCD0.CTRLA    = TC_CLKSEL_DIV256_gc;

    if(sendFriend->id == dest) printf("Sent to %02x\n\n", dest);
    else printf("Sent to %02x via %02x.\n\n", dest, sendFriend->id);
}

void send(uint8_t *data, uint8_t len) {
    nrfStopListening();
    // The datasheet says it takes 130 us to switch out of listening mode.
    _delay_us(130);
    nrfWrite(data, len);
    nrfStartListening();
}


void pingOfLife(void) {
    uint8_t listLength = 0;
    friend_t *friends = getFriendsList(&listLength);
    
    // Cringe iso implemention.
    uint8_t ping[32];
    uint8_t pingSize = 1;
    ping[0] = myId;

    // Build ping message.
    for(uint8_t i = 0; i < listLength; i++)
        if(friends[i].id != 0 && friends[i].hops == 0) 
            ping[pingSize++] = friends[i].id;


    nrfOpenWritingPipe((uint8_t *) BROADCAST_PIPE);
    send(ping, pingSize);
}


void openPrivateWritingPipe(uint8_t destId) {
    static uint8_t writingPipe[5] = PRIVATE_PIPE;
    writingPipe[4] = destId;
    nrfOpenWritingPipe(writingPipe);
}

static void interpretPacket(uint8_t *packet, uint8_t length, uint8_t receivePipe, uint8_t receivePower) {
    // If it's the broadcast pipe, it's probably a ping of life from another node.
    if(receivePipe == BROADCAST_PIPE_INDEX) {

        // Add the sender as a new direct neighbor friend.
        addFriend(packet[0], 0, packet[0]);

        // Add all the sender's friends.
        for(uint8_t i = 1; i < length; i++)
            // Make sure you're not adding yourself as friend (you can't be schizophrenic).
            if(packet[i] != myId) 
                addFriend(packet[i], 1, packet[0]);

        return;
    }

    PORTF.OUTTGL = PIN1_bm;
    if(receivePower) PORTF.OUTSET = PIN0_bm;
    else PORTF.OUTCLR = PIN0_bm;
    printf("Receive Power: %d\n", receivePower);

    // If it's a message for me.
    if(packet[0] == myId) receiveCallback(packet + 1, length - 1);
    
    // If it's a message for someone else.
    else {
        friend_t *destFriend = findFriend(packet[0]);
        if(destFriend == NULL) return;

        // Relay the message.
        openPrivateWritingPipe(destFriend->via);
        send(packet, length);
    }
}

ISR(PORTF_INT0_vect) {
    uint8_t receivePipe = 0xff;

    // Did I receive something actually valuable? 
    if(nrfAvailable(&receivePipe)) {
        uint8_t receivePower = nrfReadRegister(9);
        uint8_t length = nrfGetDynamicPayloadSize();
        uint8_t packet[32];

        // Put received data into a buffer.
        nrfRead(packet, length);

        interpretPacket(packet, length, receivePipe, receivePower);
    }    
}

ISR(TCD0_OVF_vect) {
    // Cut the
    pingOfLife();
    // you've been feeding my veins.
    friendTimeTick();
}