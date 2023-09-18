#include <string.h>
#include <avr/interrupt.h>

#include "iso.h"
#include "nrf24L01.h"
#include "nrf24spiXM2.h"
#include "serialF0.h"

// Shift team ID 4 bits to the left so that it can be the first(most segnificant) 4 bits of the ID.
#define TEAM_ID 0x02 << 4

static uint8_t myId = 0;

//TODO: This should be in the nrfChat lib
// receivedChat only gets updated when the program receives a chat, not some other message.
char receivedChat[NRF_MAX_PAYLOAD_SIZE + 1] = "\0";
uint8_t receivePipe = 69;

//TODO: optimize the init
void isoInitNrf(void) {
    nrfspiInit();
    nrfBegin();
    
    nrfSetRetries(NRF_SETUP_ARD_1000US_gc, NRF_SETUP_ARC_8RETRANSMIT_gc);
    nrfSetPALevel(NRF_RF_SETUP_PWR_6DBM_gc);
    nrfSetDataRate(NRF_RF_SETUP_RF_DR_250K_gc);
    nrfSetCRCLength(NRF_CONFIG_CRC_16_gc);
    nrfSetChannel(STANDARD_CHANNEL);
    nrfSetAutoAck(1);
    nrfEnableDynamicPayloads();
    
    nrfClearInterruptBits();
    nrfFlushRx();
    nrfFlushTx();
    
    // Initialize the receiving interrupt
    PORTF.INT0MASK |= PIN6_bm;
    PORTF.PIN6CTRL = PORT_ISC_FALLING_gc;
    PORTF.INTCTRL |= (PORTF.INTCTRL & ~PORT_INT0LVL_gm) | PORT_INT0LVL_LO_gc;
    nrfPowerUp();
    
    _delay_ms(1);
    nrfOpenWritingPipe((uint8_t *) "HVA01");
    _delay_ms(1);
    nrfOpenReadingPipe(0, (uint8_t *) "HVA01");
    nrfStartListening();
}

uint8_t isoInitId(void) {
    PORTD.PIN0CTRL = PORT_OPC_PULLUP_gc;
    PORTD.PIN1CTRL = PORT_OPC_PULLUP_gc;
    PORTD.PIN2CTRL = PORT_OPC_PULLUP_gc;
    PORTD.PIN3CTRL = PORT_OPC_PULLUP_gc;

    PORTD.DIRCLR = 0b1111;
    myId = ~PORTD.IN & 0b1111;
    myId |= TEAM_ID;

    printf("My ID is 0x%02x\n", myId);

    return myId;
}

char *isoGetReceivedChat(uint8_t *pipe) {
    *pipe = receivePipe;
    return receivedChat;
}


void isoSendChat(char *command) {
    nrfStopListening();
    // The datasheet says it takes 130 us to switch out of listening mode.
    _delay_us(130);
    uint8_t response = nrfWrite((uint8_t *) command, strlen(command));
    nrfStartListening();
    
    printf("Verzonden%s: %s\n\n", response > 0 ? " (ACK)" : "", command);
}

void isoSend(uint8_t dest, uint8_t *data, uint8_t len) {
    uint8_t sentData[32];
    sentData[0] = myId;
    sentData[1] = dest;
    memcpy(sentData + 2, data, len);

    nrfStopListening();
    // The datasheet says it takes 130 us to switch out of listening mode.
    _delay_us(130);
    nrfWrite((uint8_t *) sentData, len + 2);
    nrfStartListening();

    //TODO: Add printf for debugging crap.
}


void interpretMessage(char *message) {
    strcpy(receivedChat, message);
}


ISR(PORTF_INT0_vect) {
    PORTF.OUTTGL = PIN0_bm;
    uint8_t packetLength;
    // Did I receive something actually valuable? 
    if(nrfAvailable(NULL)) {
        static char receivedPacket[NRF_MAX_PAYLOAD_SIZE + 1];
        packetLength = nrfGetDynamicPayloadSize();

        // Put received data into a buffer.
        receivePipe = (nrfGetStatus() & NRF_STATUS_RX_P_NO_gm) >> 1;
        nrfRead(receivedPacket, packetLength);
        receivedPacket[packetLength] = '\0';

        interpretMessage(receivedPacket);
    }    
}
