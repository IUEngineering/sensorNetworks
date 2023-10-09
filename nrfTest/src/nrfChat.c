#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "friendList.h"
#include "nrfChat.h"
#include "terminal.h"

#include "nrf24L01.h"
#include "nrf24spiXM2.h"
#include "serialF0.h"

#define COMMANDS 7
#define INPUT_BUFFER_LENGTH 38

// 'Command line' functions.
static void rpip(char *command);
static void help(char *command);
static void chan(char *command);
static void send(char *command);
static void list(char *command);
static void dest(char *command);
static void myid(char *command);

static void interpretInput(char *buffer);

// Callback functions.
static void interpretInput(char *input);
static void messageReceive(uint8_t *payload, uint8_t length);

static uint8_t receivedMessage[32];
static uint8_t receivedMessageLength = 0;
static uint8_t destinationId = 0xff;

void nrfChatInit(void) {
    // Send welcome message
    printf("Welcome to the nrfTester\nMade by Jochem Leijenhorst.\n\nType /help for a list of commands.\n");
    isoInit(messageReceive);
    terminalSetCallback(interpretInput);
    
    // Print a nice line (it looks cool).
    for(uint8_t i = 0; i < 64; i++) uartF0_putc('-');
    printf("\n\n");

    PORTA.PIN0CTRL = PORT_INVEN_bm | PORT_OPC_PULLUP_gc;
}

void nrfChatLoop(void) {
    while (1) {
        // Get the character from the user.
        char newInputChar = uartF0_getc();

        if(newInputChar != '\0') {
           interpretNewChar(newInputChar);
        }

        printReceivedMessage();
        isoUpdate();
    }
}

// This function should be run when a new character has been inputted by the user.
void interpretChar(char newChar) {
    terminalInterpretChar(newChar);
}

// This function has to be run run repeatedly by the main.
void printReceivedMessages(void) {
    isoUpdate();

    if(receivedMessageLength) {
        terminalPrintStrex(receivedMessage, receivedMessageLength, "Received:");
        receivedMessageLength = 0;
    }
}

// Callback from iso.c.
// Sends the received buffer over to terminal.c
void messageReceive(uint8_t *payload, uint8_t length) {
    memcpy(receivedMessage, payload, length);
    receivedMessageLength = length;
}

void interpretInput(char *input) {

    // If no '/' is given, just send the inputted string.
    if(input[0] != '/') {
        send(input + 6);
        return;
    }

    // If a '/' is given, we want to interpret from the character after it.
    input++;

    // Make an array of functions.
    const void (*comFunc[COMMANDS])(char*) = {
        rpip, send, help, chan, list, dest, myid
    };

    // Make a corresponding array of 4 letter function names.
    const char commands[COMMANDS][4] = {
        "rpip", "send", "help", "chan", "list", "dest", "myid"
    };

    
    for(uint8_t i = 0; i < COMMANDS; i++) {
        if(strncmp(commands[i], input, 4) == 0) {
            // Run the command, with the text after it as argument.
            comFunc[i](input + 5);
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
    if(isoSendPacket(destinationId, (uint8_t*) command, strlen(command))) 
        printf("Failed to send message\n");    
}

void help(char *command) {
    printf("\n\nThere are %d commands:\n\n", COMMANDS);
    printf("*    \e[32m/help\e[0m\n\tPrint this list.\n\n");
    printf("*    \e[32m/send\e[0m <message>\n\tSend a message. This can also be done by just typing a message and pressing enter.\n\n");
    printf("*    \e[32m/rpip\e[0m <pipename> [index]\n\tChange one of the reading pipes. The reading pipe index can be provided, the default is 0.\n\n");
    printf("*    \e[32m/chan\e[0m <channel>\n\tChange the channel frequency.\n\n");
    printf("*    \e[32m/list\e[0m\n\tPrint a list of friends :)\n\n");
    printf("*    \e[32m/dest\e[0m <id>\n\tChange the id of the destination node.\n\n\n");
    printf("*    \e[32m/myid\e[0m\n\tPrints your ID.");
    printf("The program always prints what it is receiving on all open reading pipes.\n\n");
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
        printf("New destination ID is \e[35m0x%02x\e[0m\n\n", newId);
    }
}

void myid(char *command) {
    printf("Your ID is \e[35m0x%02x\e[0m\n\n", isoGetId());
}