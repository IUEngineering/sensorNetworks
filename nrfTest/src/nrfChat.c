#include <string.h>
#include <stdlib.h>

#include "nrfChat.h"
#include "nrf24L01.h"
#include "nrf24spiXM2.h"
#include "serialF0.h"

#define COMMANDS 5

void wpip(char *command);
void rpip(char *command);
void help(char *command);
void chan(char *command);

void runCommand(char *command);


// Define the received packet globally so that the received text can be printed
// without interrupting user's the current text input.
char receivedPacket[NRF_MAX_PAYLOAD_SIZE+1] = "\0";

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

// Make a buffer for the command.
// Not going to worry about buffer overflow, just don't input too much and you'll be fine.
static char inputBuffer[256];

// Make a char pointer to insert the next typed character into the buffer.
static char *bufferPtr = inputBuffer;

void interpretNewChar(char newChar) {
    // Backspace support :)
    if(newChar == '\b') {
        if(bufferPtr != inputBuffer) {
            bufferPtr--;
            // Go back one character, replace the next character with a space, and then go back one character again.
            printf("\b \b");
        }
    }

    // Things like minicom and teraterm send return characters as \r. Very annoying.
    else if(newChar == '\r') {
        *bufferPtr = '\0';
        printf("\n");

        if(inputBuffer[0] == '/') runCommand(inputBuffer + 1);
        else chatSend(inputBuffer); 

        // Reset the bufferPtr to the start of the buffer again.
        bufferPtr = inputBuffer;
    }

    // Check if it's a printable character.
    else if(newChar >= ' ' && newChar <= '~') {
        *bufferPtr = newChar;
        bufferPtr++;

        // Provide an echo of what the user is typing.
        // Otherwise the user's input would be invisible to the user.
        uartF0_putc(newChar);
    }
}

uint8_t getUserInputLength() {
    return bufferPtr - inputBuffer;
}

char *getCurrentInputBuffer() {
    *bufferPtr = '\0';
    return inputBuffer;
}


void runCommand(char *command) {
    // Make an array of functions.
    const void (*comFunc[COMMANDS])(char*) = {
        wpip, rpip, chatSend, help, chan
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
    if(spaceChar != NULL && *(spaceChar + 1) != '\0') pipeIndex = atoi(spaceChar + 1);
    
    // Open the pipe
    nrfStopListening();
    nrfOpenReadingPipe(pipeIndex, (uint8_t *) pipeName);
    nrfStartListening();
    printf("\n\nReading pipe %d, %s geopend.\n", pipeIndex, pipeName);
    if(pipeIndex > 1) printf("Onthoud goed dat voor pipes 2 tot 5 alleen het laatste karakter wordt gebruikt. In dit geval is dat %c\n", pipeName[4]);
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
