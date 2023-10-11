#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "friendList.h"
#include "nrfChat.h"
#include "terminal.h"

#include "nrf24L01.h"
#include "nrf24spiXM2.h"
#include "serialF0.h"

#define COMMANDS 6
#define INPUT_BUFFER_LENGTH 38

// 'Command line' functions.
static void help(char *command);
static void chan(char *command);
static void send(char *command);
static void list(char *command);
static void dest(char *command);
static void myid(char *command);

static void interpretInput(char *buffer);

// Callback functions.
static void interpretInput(char *input);
static void messageReceive(uint8_t *payload);

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
           terminalInterpretChar(newInputChar);
        }

        isoUpdate();

        if(receivedMessageLength) {
            terminalPrintStrex(receivedMessage, receivedMessageLength, "Received:");
            receivedMessageLength = 0;
        }
    }
}

// Callback from iso.c.
// Sends the received buffer over to terminal.c
void messageReceive(uint8_t *payload) {
    memcpy(receivedMessage, payload, PAYLOAD_SIZE);
    receivedMessageLength = strnlen((char *)payload, PAYLOAD_SIZE);
}

// Callback from terminal.c.
// Interprets the user input. 
void interpretInput(char *input) {

    // If no '/' is given, just send the inputted string.
    if(input[0] != '/') {
        send(input);
        return;
    }

    // If a '/' is given, we want to interpret from the character after it.
    input++;

    // Make an array of functions.
    const void (*comFunc[COMMANDS])(char*) = {
        send, help, chan, list, dest, myid
    };

    // Make a corresponding array of 4 letter function names.
    const char commands[COMMANDS][4] = {
        "send", "help", "chan", "list", "dest", "myid"
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

void send(char *command) {
    if(isoSendPacket(destinationId, (uint8_t*) command, strlen(command))) 
        printf("Failed to send message\n");    
}

void help(char *command) {
    printf("\n\nThere are %d commands:\n\n", COMMANDS);
    printf("*    \e[32m/help\e[0m\n\tPrint this list.\n\n");
    printf("*    \e[32m/send\e[0m <message>\n\tSend a message. This can also be done by just typing a message and pressing enter.\n\n");
    printf("*    \e[32m/chan\e[0m <channel>\n\tChange the channel frequency.\n\n");
    printf("*    \e[32m/list\e[0m\n\tPrint a list of friends :)\n\n");
    printf("*    \e[32m/dest\e[0m <id>\n\tChange the id of the destination node.\n\n");
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