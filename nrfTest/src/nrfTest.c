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
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
#include "serialF0.h"
#include "nrf24L01.h"
#include "nrf24spiXM2.h"
#include "clock.h"

// Define the received packet globally so that the received text can be printed
// without interrupting user's the current text input.
char receivedPacket[NRF_MAX_PAYLOAD_SIZE+1] = "\0";

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
    
    // Initialize the receiving interrupt
    PORTF.INT0MASK |= PIN6_bm;
    PORTF.PIN6CTRL = PORT_ISC_FALLING_gc;
    PORTF.INTCTRL |= (PORTF.INTCTRL & ~PORT_INT0LVL_gm) | PORT_INT0LVL_LO_gc;
    
    nrfOpenReadingPipe(0, (uint8_t *)"HVA01");
    nrfPowerUp();
    nrfStartListening();
}


int main(void) {
    PORTF.DIRSET = PIN0_bm | PIN1_bm;

    init_clock();
    init_stream(F_CPU);
    
    PMIC.CTRL |= PMIC_LOLVLEN_bm;
    sei();
    clear_screen();
    
    // Send welcome message
    printf("Welkom bij de nrftester\nGemaakt door Jochem Leijenhorst.\n\nTyp help voor een lijst met commando's.\n");
    for(uint8_t i = 0; i < 64; i++) uartF0_putc('-');
    printf("\n\n");
    nrfInit(54);

    
    // Make a buffer for the command.
    // Not going to worry about buffer overflow, just don't input too much and you'll be fine.
    char inputBuffer[256];

    // Make a char pointer to insert the next typed character into the buffer.
    char *bufferPtr = inputBuffer;

    while (1) {

        // When something was received:
        if(receivedPacket[0] != '\0') {

            // Handle the user typing while something has been received.
            if(bufferPtr != inputBuffer) {
                printf("\rReceived: %s", receivedPacket);

                // Calculate how many characters of the user written command are left after printing the received packet.
                int16_t trailingCharacters = (bufferPtr - inputBuffer) - strlen(receivedPacket) - 10;

                // Print that many spaces
                for(int16_t i = 0; i < trailingCharacters; i++)
                    printf(" ");

                // Print the user inputted buffer (make sure there is a terminating \0 character so printf stops at the right place).
                *bufferPtr = '\0';
                printf("\n%s", inputBuffer);
            }
            else printf("Received: %s\n", receivedPacket);

            // Prevent the packet from being printed multiple times.
            receivedPacket[0] = 0;
        }

        // Get the character from the user.
        char newInputChar = uartF0_getc();

        if(newInputChar != '\0') {
            // Backspace support :)
            if(newInputChar == '\b') {
                if(bufferPtr != inputBuffer) {
                    bufferPtr--;
                    // Go back one character, replace the next character with a space, and then go back one character again.
                    printf("\b \b");
                }
            }

            // Things like minicom and teraterm send return characters as \r. Very annoying.
            else if(newInputChar == '\r') {
                *bufferPtr = '\0';
                printf("\n");

                if(inputBuffer[0] == '/') runCommand(inputBuffer + 1);
                else send(inputBuffer); 

                // Reset the bufferPtr to the start of the buffer again.
                bufferPtr = inputBuffer;
            }

            // Check if it's a printable character.
            else if(newInputChar >= ' ' && newInputChar <= '~') {
                *bufferPtr = newInputChar;
                bufferPtr++;

                // Provide an echo of what the user is typing.
                // Otherwise the user's input would be invisible to the user.
                uartF0_putc(newInputChar);
            }
        }
    }
}


void runCommand(char *command) {
    // Make an array of functions.
    const void (*comFunc[COMMANDS])(char*) = {
        wpip, rpip, send, help, chan
    };

    // Make a corresponding array of 4 letter function names.
    const char commands[COMMANDS][4] = {
        "wpip", "rpip", "send", "help", "chan"
    };

    // Start comparing the sent command with the command names.
    for(uint8_t i = 0; i < COMMANDS; i++) {
        if(strncmp(commands[i], command, 4) == 0) {
            // Run the command, with the text after it as argument.
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
    // Pipenames are max 5 characters + 1 null character.
    char pipeName[6];
    uint8_t pipeIndex = 0;
    char *spaceChar = strchr(command, ' ');
    if(spaceChar == NULL) spaceChar = strchr(command, '\0');

    // Check if there is even a pipename given, and if it's not longer than 5 characters.
    if(spaceChar != NULL) {
        uint8_t nameLength = spaceChar - command;
        // Make sure to not overflow the array.
        strncpy(pipeName, command, nameLength <= 5 ? nameLength : 5);
        pipeName[nameLength] = '\0';
    }
    else {
        printf("No valid pipename provided\n");
        return;
    }

    // Check if there is a second argument (the reading pipe index).
    spaceChar = strchr(command, ' ');
    if(spaceChar != NULL && *(spaceChar + 1) != NULL) pipeIndex = atoi(spaceChar + 1);
    
    // Open the pipe
    nrfStopListening();
    nrfOpenReadingPipe(pipeIndex, (uint8_t *) pipeName);
    nrfStartListening();
    printf("\n\nReading pipe %d, %s geopend.\n", pipeIndex, pipeName);
    if(pipeIndex > 1) printf("Onthoud goed dat voor pipes 2 tot 5 alleen het laatste karakter wordt gebruikt. In dit geval is dat %c\n", pipeName[4]);
}


void send(char *command) {
    nrfStopListening();
    // The datasheet says it takes 130 us to switch out of listening mode.
    _delay_us(130);
    uint8_t response = nrfWrite((uint8_t *) command, strlen(command));
    printf("\nVerzonden: %s\nAck ontvangen: %s\n", command, response > 0 ? "JA":"NEE");
    nrfStartListening();
}

void help(char *command) {
    printf("\n\nEr zijn 4 commandos:\n*	help\n\tprint deze lijst\n\n*	send <waarde>\n\tverstuurt wat je invoert op waarde naar de geselecteerde pipe\n\n*	wpip <pipenaam>\n\tverander de writing pipe\n");
	printf("\n*	rpip <index> <pipenaam>\n\tverander de reading pipes. Index is welke van de 6 pipes je wilt aanpassen (0 t/m 5).\n\nHet programma print continu uit wat het ontvangt.\n\n");
}

// Function to change the frequency channel.
void chan(char *command) {
    uint8_t channel = atoi(command);
    nrfStopListening();
    nrfSetChannel(channel);
    nrfStartListening();

    printf("Geswitched naar channel %d\n", channel);
}



ISR(PORTF_INT0_vect) {
    PORTF.OUTTGL = PIN0_bm;
    uint8_t packetLength;
    // Did I receive something actually valuable? 
    if(nrfAvailable(NULL)) {
        packetLength = nrfGetDynamicPayloadSize();

        // Put received data into a buffer.
        nrfRead(receivedPacket, packetLength);
        receivedPacket[packetLength] = '\0';
    }    
}
