#include <string.h>
#include <stdlib.h>

#include "friendList.h"
#include "nrfChat.h"
#include "terminal.h"

#include "nrf24L01.h"
#include "serialF0.h"

#define COMMANDS 6
#define INPUT_BUFFER_LENGTH 38

#define NO_COLOR        "\e[0m"
#define COMMAND_COLOR   "\e[32m"
#define ID_COLOR        "\e[35m"
#define ERROR_COLOR     "\e[31m"

// 'Command line' functions.
static void help(char *arguments);
static void chan(char *arguments);
static void send(char *arguments);
static void list(char *arguments);
static void dest(char *arguments);
static void myid(char *arguments);

// Callback functions.
static void interpretInput(char *input);
static void messageReceive(uint8_t *payload);

static uint8_t destinationId = 0xff;

void nrfChatInit(void) {
    // Send welcome message
    DEBUG_PRINT("I am a sensor node\n");
    isoInit(messageReceive);
    terminalSetCallback(interpretInput);

    printf("Welcome to the nrfTester\nMade by Team 2.\n\nType " COMMAND_COLOR "/help" NO_COLOR " for a list of commands.\n");
    printf("Started with ID " ID_COLOR "0x%02x" NO_COLOR " on channel " ID_COLOR "%d" NO_COLOR ".\n", isoGetId(), DEFAULT_CHANNEL);
    
    // Print a nice line (it looks cool).
    for(uint8_t i = 0; i < 64; i++) uartF0_putc('-');
    printf("\n\n");
}

void nrfChatLoop(void) {
    while (1) {
        // Get the character from the user.
        // Use a char instead of a uint16_t because we're only expecting user input (ASCII characters), not individual bytes.
        char newInputChar = uartF0_getc();

        if(newInputChar != '\0') {
           terminalInterpretChar(newInputChar);
        }

        isoUpdate();
    }
}

// Callback from iso.c.
// Sends the received buffer over to terminal.c
void messageReceive(uint8_t *payload) {
    terminalPrintStrex(payload, strlen((char *)payload), "Received:");
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

    // Make a corresponding array of 4 letter function names.
    static const char commands[COMMANDS][4] = {
        "send", "help", "chan", "list", "dest", "myid"
    };

    // Make an array of functions.
    static const void (*functions[COMMANDS])(char*) = {
        send, help, chan, list, dest, myid
    };

    
    for(uint8_t i = 0; i < COMMANDS; i++) {
        if(strncmp(commands[i], input, 4) == 0) {
            // If the 5th character of the input is already the end, make the 6th character a terminating \0 as well.
            // This prevents problems with commands that need arguments being run without arguments.
            if(input[4] == '\0') input[5] = '\0';

            // Run the command, with the text after it as argument.
            functions[i](input + 5);
            return;
        }
    }
    printf("I don't know that command :(\n");
}

void send(char *arguments) {
    if(isoSendPacket(destinationId, (uint8_t*)arguments, strlen(arguments))) 
        printf("Failed to send message\n");    
}

void help(char *arguments) {
    printf("There are %d commands:\n\n", COMMANDS);
    printf("*    " COMMAND_COLOR "/help" NO_COLOR "\n\tPrint this list.\n\n");
    printf("*    " COMMAND_COLOR "/send" NO_COLOR " <message>\n\tSend a message. This can also be done by just typing a message and pressing enter.\n\n");
    printf("*    " COMMAND_COLOR "/chan" NO_COLOR " <channel>\n\tChange the channel frequency.\n\n");
    printf("*    " COMMAND_COLOR "/list" NO_COLOR "\n\tPrint a list of friends :)\n\n");
    printf("*    " COMMAND_COLOR "/dest" NO_COLOR " <id>\n\tChange the id of the destination node.\n\n");
    printf("*    " COMMAND_COLOR "/myid" NO_COLOR "\n\tPrints your ID.");
    printf("The program always prints what it is receiving on all open reading pipes.\n\n");
}

// Function to change the frequency channel.
void chan(char *arguments) {
    uint8_t channel = atoi(arguments);

    nrfStopListening();
    nrfSetChannel(channel);
    nrfStartListening();

    printf("\nSwitched to channel " ID_COLOR "%d" NO_COLOR "\n\n", channel);
}

void list(char *arguments) {
    printFriends();
}

void dest(char *arguments) {
    if(arguments[0] == '\0') {
        printf("Usage:" COMMAND_COLOR " /dest" NO_COLOR " <ID>\n\n");
        return;
    }

    uint8_t newId = strtol(arguments, NULL, 16);
    if(newId == 0) printf(ERROR_COLOR "Invalid ID entered.\n\n" NO_COLOR);
    else {
        destinationId = newId;
        printf("New destination ID is " ID_COLOR "0x%02x" NO_COLOR "\n\n", newId);
    }
}

void myid(char *arguments) {
    printf("Your ID is " ID_COLOR "0x%02x" NO_COLOR "\n\n", isoGetId());
}