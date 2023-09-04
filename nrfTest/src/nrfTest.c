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

#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdlib.h>
#include "serialF0.h"
#include "nrf24L01.h"
#include "nrf24spiXM2.h"
#include "clock.h"

char received_packet[NRF_MAX_PAYLOAD_SIZE+1];
void runCommand(char *command);
void wpip(char *command);
void rpip(char *command);
void send(char *command);
void help(char *command);

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
    PORTC.DIRSET = PIN0_bm;

    init_clock();
    init_stream(F_CPU);
    
    PMIC.CTRL |= PMIC_LOLVLEN_bm;
    sei();
    clear_screen();
    
    printf("Welkom bij de nrftester\nGemaakt door Jochem Leijenhorst.\n\nTyp help voor een lijst met commando's.");
    printf("\nTyp een channel en druk op enter om te beginnen.\n\nBELANGRIJK: Ga in tera term naar setup -> terminal en klik op \"local echo\".\nZo kan je zien wat je typt. Je kan dit opslaan door naar setup -> save settings te gaan en daar op \"save\" te drukken.\n\n");
    uint16_t channel = 125;
    scanf("%d", &channel);
    nrfInit(channel);
    printf("Gestart met channel %d. Geen idee of dat legaal is, maar dat is jouw probleem.\n\n", channel);
    
    char inputBuffer[256];
    char *inChar = inputBuffer;

    while (1) {
        char newInputChar = uartF0_getc();
        if(newInputChar != '\0') {
            if(newInputChar == '\r') newInputChar = '\n';
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
    void (*comFunc[4])(char*) = {
        wpip, rpip, send, help
    };
    char commands[4][4] = {
        "wpip", "rpip", "send", "help"
    };

    if(command[4] != ' ' || strlen(command) < 6) {
        printf("fuck off commando's zijn 4 karakters lang\n");
        return;
    }

    for(uint8_t i = 0; i < 4; i++) {
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
    char *token = strtok(command, " ");
    char pipeName[6];
    uint8_t pipeIndex = 0; 

    if(token != NULL) strncpy(pipeName, token, 6);
    else {
        printf("No pipename provided\n");
        return;
    }

    token = strtok(token, NULL);
    if(token != NULL) pipeIndex = atoi(token);
    
    nrfStopListening();
    nrfOpenReadingPipe(pipeIndex, (uint8_t *) "FRANS");
    nrfStartListening();
    printf("\n\nReading pipe %d, %s geopend.\n", pipeIndex, pipeName);
    if(pipeIndex > 1) printf("Onthoud goed dat voor pipes 2 tot 5 alleen het laatste karakter wordt gebruikt. In dit geval is dat %c\n", pipeName[3]);
}

void send(char *command) {
    nrfStopListening();
    uint8_t response = nrfWrite((uint8_t *) command, strlen(command));
    printf("\n\nVerzonden: %s\nAck ontvangen: %s\n", command, response > 0 ? "JA":"NEE");
    _delay_ms(5);
    nrfStartListening();
}

void help(char *command) {

}

ISR(PORTF_INT0_vect) {
    PORTC.OUTTGL = PIN0_bm;
    uint8_t    packet_length;
    if(nrfAvailable(NULL)) {                        // is er iets nuttigs binnengekomen??
        packet_length = nrfGetDynamicPayloadSize();    // kijk hoe groot het is
        nrfRead(received_packet, packet_length);    // lees wat er is gestuurd
        received_packet[packet_length] = '\0';        // zet laatste karakter van de array naar null
        
        printf("Ontvangen: %s\n", received_packet);
    }    
}
