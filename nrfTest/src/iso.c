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

// Counter every 250 ms formula:
// TC_CCA = ((t * F_CPU) / (2* N)) - 1 
#define TC_CCA  15624


// Define list of neighbors (neighbors are friends :)
static uint8_t myId = 0;
static void pingOfLife(void);
static void (*receiveCallback)(uint8_t *payload, uint8_t length);
static void send(uint8_t *data, uint8_t len);
static void openPrivateWritingPipe(uint8_t destId);
static void timerOverflow(void);
static void interpretPacket(uint8_t *packet, uint8_t length, uint8_t receivePipe, uint8_t receivePower);


//TODO: optimize the init
void isoInit(void (*callback)(uint8_t *data, uint8_t length)) {
    nrfspiInit();
    nrfBegin();
    
    nrfSetRetries(NRF_SETUP_ARD_1000US_gc, NRF_SETUP_ARC_NORETRANSMIT_gc);
    nrfSetPALevel(NRF_RF_SETUP_PWR_18DBM_gc);
    nrfSetDataRate(NRF_RF_SETUP_RF_DR_2M_gc);
    nrfSetCRCLength(NRF_CONFIG_CRC_16_gc);
    nrfSetChannel(DEFAULT_CHANNEL);
    nrfSetAutoAck(0);
    nrfEnableDynamicPayloads();
    
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
    _delay_loop_1(1);

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
    TCD0.CTRLA    = TC_CLKSEL_DIV256_gc;
    TCD0.CCA      = TC_CCA;

    receiveCallback = callback;

    printf("My ID is \e[34m0x%02x\e[0m, my pipe is \e[34m%.4s\e[0;31m%c\e[0m, I am set to channel \e[34m%d\e[0m\n", \
        myId, (char*)privatePipe, privatePipe[4], DEFAULT_CHANNEL);

    DEBUG_PRINT("\e[31mDebugging is enabled.\e[0m");
}

void isoUpdate(void) {
    // Read the timer counter overflow interrupt flag.
    if(TCD0.INTFLAGS & TC0_OVFIF_bm) timerOverflow();
    // Reset said flag.
    TCD0.INTFLAGS |= TC0_OVFIF_bm;

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

void timerOverflow(void) {
    // Cut the
    pingOfLife();
    // you've been feeding my veins.

    friendTimeTick();
}

void isoSendPacket(uint8_t dest, uint8_t *payload, uint8_t len) {

    // Prevent segfault
    if(len > 31) len = 31;

    // Build pay
    uint8_t sendData[32];
    sendData[0] = dest;
    memcpy(sendData + 1, payload, len);

    // Find the destination
    friend_t *sendFriend = findFriend(dest);
    if(sendFriend == NULL) {
        printf("I don't know friend %02x\n\n", dest);
        return;
    }

    // Check if the friend is active. If it isn't, send via another friend.
    if(sendFriend->active == 0) sendFriend = findFriend(sendFriend->via);

    // Does the via friend exist?
    if(sendFriend == NULL) {
        printf("Friend isn't trusted. Found no active vias.\n\n");
        return;
    }

    if(sendFriend->id == dest) printf("Sending directly to %02x\n\n", dest);
    else printf("Sending to %02x via %02x.\n\n", dest, sendFriend->id);

    TCD0.CTRLA = TC_CLKSEL_OFF_gc;
    openPrivateWritingPipe(sendFriend->id);
    send(sendData, len + 1);
    TCD0.CTRLA = TC_CLKSEL_DIV256_gc;
}

void send(uint8_t *data, uint8_t len) {
    nrfStopListening();
    // The datasheet says it takes 130 us to switch out of listening mode.
    _delay_us(130);
    nrfWrite(data, len);
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

    // For the first byte of the ping.
    uint8_t foundFirstDirectFriend = 0;

    // Keep track of indeces.
    uint8_t friendsIndex = 0;
    uint8_t pingIndex = 3;


    // Build ping message.
    while(friends[friendsIndex].id != 0 && pingIndex < 31 && friendsIndex < 32) {

        // Make sure we're not sending inactive direct friends.
        if(!friends[friendsIndex].active  &&  !friends[friendsIndex].hops) {
            friendsIndex++;
            continue;
        }

        // Put the first direct friend on index one, as the iso states (I hope).
        if(!foundFirstDirectFriend  &&  friends[friendsIndex].active) {
            foundFirstDirectFriend = 1;
            ping[1] = friends[friendsIndex].id;
        }

        // Build the rest of the ping.
        else if(friends[friendsIndex].hops < 16 || friends[friendsIndex].active) {
            // Ping build:
            //* Index:      0   1   2   3   4   5   6   7   8   9   10
            //* Index % 3:  0   1   2   0   1   2   0   1   2   0   1
            //* Type:       mID Fst Hps id1 id2 Hps id1 id2 Hps id1 id2
            //
            //  mID = My ID.
            //  Fst = First direct friend.
            //  Hps = Hops of the coming 2 friends.     | Index % 3 = 2
            //  id1 = First friend of last hops count.  | Index % 3 = 0
            //  id2 = Second friend of last hops count. | Index % 3 = 1

            // Actually copy the friend's id into the ping.
            ping[pingIndex] = friends[friendsIndex].id;

            // Make sure to set the hops to 0 if the friend is active.
            const uint8_t hopsAmount = friends[friendsIndex].active ? 0 : friends[friendsIndex].hops;

            // Find if this is an id1 or id2 situation
            if(pingIndex % 3 == 0) {
                ping[pingIndex - 1] |= hopsAmount << 4;
                pingIndex++;
            }
            else {
                ping[pingIndex - 2] |= hopsAmount;
                pingIndex += 2;
            }   
        }
        
        friendsIndex++;
    }

    // If we don't have any friends, make sure to only send our own ID.
    if(friends[0].id == 0) pingIndex = 1;

    // If it ended with a hops byte, don't include it (it's obviously just going to be 0x00).
    // This also works if we only have 1 friend (the pingIndex never changes from 3).
    else if(pingIndex % 3 == 0) pingIndex--;


    nrfOpenWritingPipe((uint8_t *) BROADCAST_PIPE);
    send(ping, pingIndex);
}


void openPrivateWritingPipe(uint8_t destId) {
    static uint8_t writingPipe[5] = PRIVATE_PIPE;
    writingPipe[4] = destId;
    nrfOpenWritingPipe(writingPipe);
}

void interpretPacket(uint8_t *packet, uint8_t length, uint8_t receivePipe, uint8_t receivePower) {


    // If it's a Ping of Life:
    if(receivePipe == BROADCAST_PIPE_INDEX) {

        #ifdef DEBUG
        terminalPrintf("Received PoL from 0x%02x\n", packet[0]);
        char printPOLBuf[MAX_PACKET_SIZE * 12];
        char *printPOLPtr = printPOLBuf;

        for(uint8_t i = 0; i < length; i++)
            printPOLPtr += sprintf(printPOLPtr, "\e[%sm%02x\e[0m ", (i % 3) == 2 ? "34" : "0", packet[i]);

        sprintf(printPOLPtr, "\n");
        terminalPrint(printPOLBuf);

        #endif

        // Add the sender as a new direct neighbor friend.
        DEBUG_PRINT("Direct:");
        friend_t *directFriend = updateFriend(packet[0], 0, 0);

        // Do we trust this friend enough?
        if(!directFriend->active) return;

        removeViaReferences(packet[0]);

        // Add the first direct friend.
        if(packet[1] != myId  &&  length > 1) {
            DEBUG_PRINT("packet[1]:");
            updateFriend(packet[1], 1, packet[0]);
        }

        // Add the rest of sender's friends.
        for(uint8_t i = 3; i < length;) {
            // See the comments in pingOfLife() for an explanation of this.
            // If it's the first of the 2 ID's:
            if(i % 3 == 0) {
                if(packet[i] != myId) {
                    DEBUG_PRINT("First ID:");
                    updateFriend(packet[i], (packet[i - 1] >> 4) + 1, packet[0]);
                }
                i++;
            }
            // Else if it's the second:
            else {
                if(packet[i] != myId) {
                    DEBUG_PRINT("Second ID:");
                    updateFriend(packet[i], (packet[i - 2] & 0x0f) + 1, packet[0]);
                }
                i += 2;
            }
        }

        // receiveCallback(packet, length);
    
        return;
    }


    PORTF.OUTTGL = PIN1_bm;
    if(receivePower) PORTF.OUTSET = PIN0_bm;
    else PORTF.OUTCLR = PIN0_bm;

    // If it's a message for me.
    if(packet[0] == myId) receiveCallback(packet + 1, length - 1);

    // If it's a message for someone else.
    else {
        // Relay packet
        isoSendPacket(packet[0], packet + 1, length - 1);
    }
}


uint8_t isoGetId(void) {
    return myId;
}
