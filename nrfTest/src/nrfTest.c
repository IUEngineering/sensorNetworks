/*
 * nrftester.c
 *
 * Created: 18/03/2021 21:13:47
 * Author : Jochem Leijenhorst
 
 Dit programma heeft variabele pipes. Deze voer je in op tera term/putty
 Er zijn 4 commandos:
 
 *    help
    print deze lijst
 
 *    send <waarde>
    verstuurt wat je invoert op waarde naar de geselecteerde pipe
    
 *    wpip <pipenaam>
    verander de writing pipe
    
 *    rpip <pipenaamm> <index>
    verander de reading pipes
    
    Het programma print continu uit wat hij ontvangt.
 */ 

#define F_CPU    32000000
#define COMMANDS 5

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include "serialF0.h"
#include "nrf24L01.h"
#include "nrf24spiXM2.h"
#include "clock.h"

void runCommand(char *command);
void wpip(char *command);
void rpip(char *command);
void send(char *command);
void help(char *command);
void chan(char *command);

void nrfInit(uint16_t channel) {
    nrfspiInit();
    nrfBegin();
    
    nrfSetRetries(NRF_SETUP_ARD_1000US_gc, NRF_SETUP_ARC_8RETRANSMIT_gc);
    nrfSetPALevel(NRF_RF_SETUP_PWR_6DBM_gc);
    nrfSetDataRate(NRF_RF_SETUP_RF_DR_250K_gc);
    nrfSetCRCLength(NRF_CONFIG_CRC_16_gc);
    nrfSetChannel(channel);
    nrfSetAutoAck(1);
    nrfEnableDynamicPayloads();
    
    nrfClearInterruptBits();
    nrfFlushRx();
    nrfFlushTx();
    
    //initialiseert de interrupt die runt wanneer er een signaal ontvangen wordt
    PORTF.INT0MASK |= PIN6_bm;
    PORTF.PIN6CTRL = PORT_ISC_FALLING_gc;
    PORTF.INTCTRL |= (PORTF.INTCTRL & ~PORT_INT0LVL_gm) | PORT_INT0LVL_LO_gc;
    
    nrfOpenReadingPipe(0, (uint8_t *)"HVA01");
    nrfPowerUp();
    nrfStartListening();
}


int main(void) {
    PORTF.DIRSET = PIN0_bm;

    init_clock();
    init_stream(F_CPU);
    
    PMIC.CTRL |= PMIC_LOLVLEN_bm;
    sei();
    clear_screen();
    
    printf("Welkom bij de nrftester\nGemaakt door Jochem Leijenhorst.\n\nTyp help voor een lijst met commando's.\n");
    uint16_t channel = 54;
    scanf("%d", &channel);
    nrfInit(channel);
    printf("Gestart met channel %d. Geen idee of dat legaal is, maar dat is jouw probleem.\n\n", channel);
    
    char inputBuffer[256];
    char *inChar = inputBuffer;

    while (1) {
        char newInputChar = uartF0_getc();
        if(newInputChar != '\0') {
            if(newInputChar == '\r') newInputChar = '\n';
            if(newInputChar == '\b') {
                if(inChar != inputBuffer) {
                    inChar--;
                    printf("\b \b");
                }
                continue;
            }

            *inChar = newInputChar;
            uartF0_putc(newInputChar);
            if(newInputChar == '\n') {
                *inChar = '\0';
                uartF0_putc('\r');
                runCommand(inputBuffer);
                inChar = inputBuffer;
            }
            else inChar++;
        }
    }
}


void runCommand(char *command) {
    void (*comFunc[COMMANDS])(char*) = {
        wpip, rpip, send, help, chan
    };
    char commands[COMMANDS][4] = {
        "wpip", "rpip", "send", "help", "chan"
    };

    for(uint8_t i = 0; i < COMMANDS; i++) {
        if(strncmp(commands[i], command, 4) == 0) {
            comFunc[i](command + 5);
            return;
        }
    }
    printf("Die ken ik niet :(\n");

}

void wpip(char *command) {
    nrfOpenWritingPipe((uint8_t *)command);
    printf("\n\nWriting pipe %s geopend.\n", command);
}

void rpip(char *command) {
    char pipeName[6];
    uint8_t pipeIndex = 0;
    char *spaceChar = strchr(command, ' ');
    if(spaceChar == NULL) spaceChar = strchr(command, '\0');

    if(spaceChar != NULL || spaceChar - command > 5) {
        strncpy(pipeName, command, spaceChar - command);
        pipeName[spaceChar - command] = '\0';
    }
    else {
        printf("No pipename provided\n");
        return;
    }

    spaceChar = strchr(command, ' ');
    if(spaceChar != NULL && spaceChar + 1 != NULL) pipeIndex = atoi(spaceChar + 1);
     
    nrfStopListening();
    nrfOpenReadingPipe(pipeIndex, (uint8_t *) pipeName);
    nrfStartListening();
    printf("\n\nReading pipe %d, %s geopend.\n", pipeIndex, pipeName);
    if(pipeIndex > 1) printf("Onthoud goed dat voor pipes 2 tot 5 alleen het laatste karakter wordt gebruikt. In dit geval is dat %c\n", pipeName[3]);
}


void send(char *command) {
    nrfStopListening();
    _delay_us(130);
    uint8_t response = nrfWrite((uint8_t *) command, strlen(command));
    printf("\n\nVerzonden: %s\nAck ontvangen: %s\n", command, response > 0 ? "JA":"NEE");
    _delay_ms(5);
    nrfStartListening();
}

void help(char *command) {
    printf("\n\nEr zijn 4 commandos:\n*	help\n\tprint deze lijst\n\n*	send <waarde>\n\tverstuurt wat je invoert op waarde naar de geselecteerde pipe\n\n*	wpip <pipenaam>\n\tverander de writing pipe\n");
	printf("\n*	rpip <index> <pipenaam>\n\tverander de reading pipes. Index is welke van de 6 pipes je wilt aanpassen (0 t/m 5).\n\nHet programma print continu uit wat het ontvangt.\n");
}

void chan (char *command) {
    uint8_t channel = atoi(command);
    nrfStopListening();
    nrfSetChannel(channel);
    nrfStartListening();

    printf("Geswitched naar channel %d\n", channel);
}

ISR(PORTF_INT0_vect) {
    char receivedPacket[NRF_MAX_PAYLOAD_SIZE+1];

    PORTF.OUTTGL = PIN0_bm;
    uint8_t packetLength;
    if(nrfAvailable(NULL)) {                        // is er iets nuttigs binnengekomen??
        packetLength = nrfGetDynamicPayloadSize();    // kijk hoe groot het is
        nrfRead(receivedPacket, packetLength);    // lees wat er is gestuurd
        receivedPacket[packetLength] = '\0';        // zet laatste karakter van de array naar null
        
        printf("Ontvangen: %s\n", receivedPacket);
    }    
}
