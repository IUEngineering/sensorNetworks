#include <string.h>

#include "iso.h"
#include "nrf24L01.h"
#include "nrf24spiXM2.h"
#include "serialF0.h"

// Shift team ID 4 bits to the left so that it can be the first(most segnificant) 4 bits of the ID.
#define TEAM_ID 0x02 << 4

static uint8_t myId = 0;

uint8_t initId(void) {
    PORTD.PIN0CTRL = PORT_OPC_PULLUP_gc;
    PORTD.PIN1CTRL = PORT_OPC_PULLUP_gc;
    PORTD.PIN2CTRL = PORT_OPC_PULLUP_gc;
    PORTD.PIN3CTRL = PORT_OPC_PULLUP_gc;

    PORTD.DIRCLR = 0b1111;
    myId = ~PORTD.IN & 0b1111;
    myId |= TEAM_ID;

    return myId;
}


void chatSend(char *command) {
    nrfStopListening();
    // The datasheet says it takes 130 us to switch out of listening mode.
    _delay_us(130);
    uint8_t response = nrfWrite((uint8_t *) command, strlen(command));
    printf("\nID:%2x Verzonden: %s\nAck ontvangen: %s\n", myId, command, response > 0 ? "JA":"NEE");
    nrfStartListening();
}