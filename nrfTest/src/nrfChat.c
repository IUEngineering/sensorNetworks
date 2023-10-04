#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "friendList.h"
#include "nrfChat.h"
#include "nrf24L01.h"
#include "nrf24spiXM2.h"
#include "serialF0.h"
#include "encrypt.h"

#define COMMANDS 7
#define INPUT_BUFFER_LENGTH 38

static void rpip(char *command);
static void help(char *command);
static void chan(char *command);
static void send(char *command);
static void list(char *command);
static void dest(char *command);
static void myid(char *command);

static void runCommand(char *command);
static void interpretInput(char *buffer, uint8_t repeat);

static void messageReceive(uint8_t *payload, uint8_t length);

static uint8_t receivedMessage[32];
static uint8_t receivedMessageLength = 0;
static uint8_t destinationId = 0xff;

// Make a buffer for the command.
// Not going to worry about buffer overflow, just don't input too much and you'll be fine.
static char inputBuffer[INPUT_BUFFER_LENGTH];

// Make a char pointer to insert the next typed character into the buffer.
static char *bufferPtr = inputBuffer;

// Flag for printing the received message.
static uint8_t receivedFlag = 0;


void initChat(void) {
    // Send welcome message
    printf("Welcome to the nrfTester\nMade by Jochem Leijenhorst.\n\nType /help for a list of commands.\n");
    isoInit(messageReceive);
    for(uint8_t i = 0; i < 64; i++) uartF0_putc('-');
    printf("\n\n");
}


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

        printf("\n");
        // Just repeat last message if nothing is entered.
        if(inputBuffer != bufferPtr) *bufferPtr = '\0';

        interpretInput(inputBuffer, inputBuffer == bufferPtr);

        // Reset the bufferPtr to the start of the buffer again.
        bufferPtr = inputBuffer;
    }

    // Check if it's a printable character and if it's not overflowing any buffers.
    else if(newChar >= ' ' && newChar <= '~' && (bufferPtr - inputBuffer < 31 || inputBuffer[0] == '/') && bufferPtr - inputBuffer < INPUT_BUFFER_LENGTH) {
        *bufferPtr = newChar;
        bufferPtr++;

        // Provide an echo of what the user is typing.
        // Otherwise the user's input would be invisible to the user.
        uartF0_putc(newChar);
    }
}

void interpretInput(char *buffer, uint8_t repeat) {
    static uint8_t repeatCount = 0;

    if(inputBuffer[0] == '/') runCommand(inputBuffer + 1);
    else if(repeat) {
        char sendBuf[INPUT_BUFFER_LENGTH + 4];
        sprintf(sendBuf, "%s %d", buffer, repeatCount++);
        printf("Sent %s\n", sendBuf);
        send(sendBuf);
    }
    else {
        send(inputBuffer);
        repeatCount = 0;
    }
}

uint8_t getUserInputLength(void) {
    return bufferPtr - inputBuffer;
}

char *getCurrentInputBuffer(void) {
    // Make sure there is a terminating \0 character so printf stops at the right place.
    *bufferPtr = '\0';
    return inputBuffer;
}

void printReceivedMessage(void) {
    if(receivedFlag == 0) return;

    // Make sure to overwrite the current buffer.
    if(bufferPtr != inputBuffer) {
        printf("\rReceived: \e[0;34m");

        // Add enough spaces to hide te current buffer. (10 is the length of "Received: ")
        for(uint8_t i = 0; i < (bufferPtr - inputBuffer) - 10; i++) 
            uartF0_putc(' ');

        printf("\n");
    }
    else 
        printf("Received: \e[0;34m\n");

    // Print received message as hex values.
    for(uint8_t i = 0; i < receivedMessageLength && receivedMessage[i] != '\0'; i++)
        printf("%02x ", receivedMessage[i]);

    printf("\e[0m\n");

    // Print received message as characters.
    for(uint8_t i = 0; i < receivedMessageLength && receivedMessage[i] != '\0'; i++) {
        if(isprint(receivedMessage[i])) printf("%c  ", receivedMessage[i]);
        else printf("   ");
    }
    
    printf("\n\n");

    // Print the input buffer back onto the terminal.
    if(bufferPtr != inputBuffer) {
        *bufferPtr = '\0';
        printf("%s", inputBuffer);
    }

    receivedFlag = 0;
}


void messageReceive(uint8_t *payload, uint8_t length) {
    receivedFlag = 1;
    memcpy(receivedMessage, payload, length);
    receivedMessageLength = length;
}

void runCommand(char *command) {
    // Make an array of functions.
    const void (*comFunc[COMMANDS])(char*) = {
        rpip, send, help, chan, list, dest, myid
    };

    // Make a corresponding array of 4 letter function names.
    const char commands[COMMANDS][4] = {
        "rpip", "send", "help", "chan", "list", "dest", "myid"
    };

    
    for(uint8_t i = 0; i < COMMANDS; i++) {
        if(strncmp(commands[i], command, 4) == 0) {
            // Run the command, with the text after it as argument.
            comFunc[i](command + 5);
            return;
        }
    }
    printf("I don't know that command :(\n");

}

//TODO: Refactor perhaps
void rpip(char *command) {
    //TODO: Look at this code and at what Dolman already checks
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
        printf("\nNo valid pipename provided\n\n");
        return;
    }

    // Check if there is a second argument (the reading pipe index).
    spaceChar = strchr(command, ' ');
    if(spaceChar != NULL && *(spaceChar + 1) != '\0') pipeIndex = atoi(spaceChar + 1);
    
    // Open the pipe
    nrfStopListening();
    nrfOpenReadingPipe(pipeIndex, (uint8_t *) pipeName);
    nrfStartListening();
    printf("\nOpened reading pipe %d, %s.\n\n", pipeIndex, pipeName);
}

void send(char *command) {
    isoSendPacket(destinationId, (uint8_t*) command, strlen(command));    
}

void help(char *command) {
    printf("\n\nThere are %d commands:\n\n", COMMANDS);
    printf("*    /help\n\tPrint this list.\n\n");
    printf("*    /send <message>\n\tSend a message. This can also be done by just typing a message and pressing enter.\n\n");
    printf("*    /rpip <pipename> [index]\n\tChange one of the reading pipes. The reading pipe index can be provided, the default is 0.\n\n");
    printf("*    /chan <channel>\n\tVerander de channel frequentie.\n\n");
    printf("*    /list\n\tPrint a list of friends :)\n\n");
    printf("*    /dest <id>\n\tChange the id of the destination node.\n\n");
    printf("*    /myid\n\tGet your ID.\n\n");
    printf("\nThe program continually prints what it is receiving on all open reading pipes.\n\n");

    uint8_t len = strlen(command);
    uint8_t *encryptedData = xorEncrypt((uint8_t*)command, len , 0xAB);

    for (uint8_t i = 0; i < len; i++) {
        printf("%x ", encryptedData[i]);
    }
    printf("\n");
}

// Function to change the frequency channel.
void chan(char *command) {
    uint8_t channel = atoi(command);
    nrfStopListening();
    nrfSetChannel(channel);
    nrfStartListening();

    printf("\nSwitched to channel %d\n\n", channel);
}

void list(char *command) {
    printFriends();
}

void dest(char *command) {
    uint8_t newId = strtol(command, NULL, 16);
    if(newId == 0) printf("Invalid ID entered.\n\n");
    else {
        destinationId = newId;
        printf("New destination ID is 0x%02x\n\n", newId);
    }
}

void myid(char *command) {
    printf("Your ID is 0x%02x\n", isoGetId());
}