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
#define PRIVATE_PIPE "TYCH"

// Counter every 250 ms formula:
// TC_CCA = ((t * F_CPU) / (2* N)) - 1 
#define TC_CCA  15624

#define BROADCAST_PIPE (uint8_t *)"420B\0"
#define PRIVATE_PIPE {'4', '2', '0', 'P', '\0'} 



// Define list of neighbors (neighbors are friends :)
static uint8_t myId = 0;
static void pingOfLife(void);
static void (*receiveCallback)(uint8_t *data, uint8_t length);
static void send(uint8_t *data, uint8_t len);


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
    PORTD.PIN0CTRL = PORT_OPC_PULLUP_gc;
    PORTD.PIN1CTRL = PORT_OPC_PULLUP_gc;
    PORTD.PIN2CTRL = PORT_OPC_PULLUP_gc;
    PORTD.PIN3CTRL = PORT_OPC_PULLUP_gc;

    PORTD.DIRCLR = 0b1111;
    myId = ~PORTD.IN & 0b1111;
    myId |= TEAM_ID;
    uint8_t privatePipe[5] = PRIVATE_PIPE;
    privatePipe[4] = myId;
    
    nrfPowerUp();

    _delay_ms(5);
    nrfOpenWritingPipe((uint8_t *) "HVA01");
    _delay_ms(5);
    nrfOpenReadingPipe(0, (uint8_t *) BROADCAST_PIPE);
    _delay_ms(5);
    nrfOpenReadingPipe(1, privatePipe);
    nrfStartListening();


    initFriendList();


    // Initialize the timer/counter for sending POL's and removing friends.
    TCD0.CTRLB    = TC0_CCAEN_bm | TC_WGMODE_FRQ_gc;
    TCD0.CTRLA    = TC_CLKSEL_DIV256_gc;
    TCD0.CCA      = TC_CCA;
    TCD0.INTCTRLA |= TC_OVFINTLVL_LO_gc;

    receiveCallback = callback;

    printf("My ID is 0x%02x, my pipe is %c%c%c%c\e[0;31m%c\e[0m\n", myId, privatePipe[0], privatePipe[1], privatePipe[2], privatePipe[3], privatePipe[4]);
}

void isoSend(uint8_t dest, uint8_t *data, uint8_t len) {
    // Prevent segfault
    if(len > 31) len = 31;

    uint8_t sendData[32];
    sendData[0] = dest;
    memcpy(sendData + 1, data, len);

    static uint8_t writingPipe[5] = PRIVATE_PIPE;
    writingPipe[4] = dest;

    TCD0.CTRLA    = TC_CLKSEL_OFF_gc;
    nrfOpenWritingPipe(writingPipe);
    send(sendData, len + 1);
    TCD0.CTRLA    = TC_CLKSEL_DIV256_gc;
}

void send(uint8_t *data, uint8_t len) {
    nrfStopListening();
    // The datasheet says it takes 130 us to switch out of listening mode.
    _delay_us(130);
    nrfWrite(data, len);
    nrfStartListening();
}

static void interpretPacket(uint8_t *packet, uint8_t length, uint8_t receivePipe) {
    // If it's the broadcast pipe, it's probably a ping of life from another node.
    if(receivePipe == 0) {
        // Add a new direct neighbor friend.
        friend_t newFriend;
        newFriend.id = packet[0];
        newFriend.hops = 0;
        newFriend.remainingTime = FORGET_FRIEND_TIME;
        newFriend.via = 0;

        addFriend(newFriend);
        return;
    }

    PORTF.OUTTGL = PIN1_bm;

    if(packet[0] == myId) receiveCallback(packet + 1, length - 1);
}

void pingOfLife(void) {
    nrfOpenWritingPipe((uint8_t *) BROADCAST_PIPE);
    send(&myId, 1);
}


ISR(PORTF_INT0_vect) {
    uint8_t receivePipe = 0xff;

    // Did I receive something actually valuable? 
    if(nrfAvailable(&receivePipe)) {
        uint8_t length = nrfGetDynamicPayloadSize();
        uint8_t packet[32];

        // Put received data into a buffer.
        nrfRead(packet, length);

        interpretPacket(packet, length, receivePipe);
    }    
}

ISR(TCD0_OVF_vect) {
    // Cut the
    pingOfLife();
    // you've been feeding my veins.
    friendTimeTick();
}