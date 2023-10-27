#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include "mcuComm.h"
#include "friendWindow.h"
#include "rs232.h"

#define INPUT_BUFFER_START_SIZE 16

// For receiving from the XMega:
#define ID_BYTE 0x00
#define ID_SIZE 2

#define FRIENDSLIST_BYTE 0x01

#define MY_PAYLOAD_BYTE 0x02
#define PAYLOAD_SIZE    31

#define RELAYED_BYTE    0x03
#define BROADCAST_BYTE  0x04
#define PACKET_SIZE     32

#define HEX_VAL_EVEN_PAIR 16
#define HEX_VAL_ODD_PAIR  17

#define INITIAL_INPUT_BUFFER_SIZE BYTES_PER_FRIEND * 8 + FRIENDLIST_HEADER_SIZE

// For sending to the XMega:
#define TRANSMIT_SOMETHING_BYTE 0x01


uint8_t myId = 0;
static int ttyIndex;

static char* getPrintable(char c) __attribute__((unused));
static void printPacketToWindow(uint8_t *packet, WINDOW *window);
static void handleNewByte(uint8_t newByte);


WINDOW *diagWindow;

// Assumes:
//- ncurses has been initialised.
//- `start_color()` has been run.
//- `friendsWin` is an initialized window of 28 wide and at least 4 high.
//- `diagnosticsWindow` is an initialized window of 64 wide and at least 2 high.
int8_t initInputHandler(WINDOW *friendWindow, WINDOW *diagnosticWindow) {

    // Find the XMega by scanning the /dev/ directory for ttyACM(any number)
    if(!RS232_OpenComport(24, 115200, "8N1", 0)) ttyIndex = 24;
    else if (!RS232_OpenComport(25, 115200, "8N1", 0)) ttyIndex = 25;
    else return -1;

    fprintf(stderr, "connected to ttyACM%d\n", ttyIndex - 24);

    init_pair(HEX_VAL_EVEN_PAIR, COLOR_BLUE, COLOR_BLACK);  
    init_pair(HEX_VAL_ODD_PAIR, COLOR_GREEN, COLOR_BLACK); 

    diagWindow = diagnosticWindow;
    scrollok(diagWindow, 1);
    if(getmaxx(diagWindow) != 64) wprintw(diagWindow, "WARNING: diagnostics window should be exactly 68 cols wide!");

    initFriendWindow(friendWindow);

    uint8_t inByte = 0xff;
    uint32_t retries = 0;
    while(RS232_PollComport(ttyIndex, &inByte, 1) <= 0) {
        RS232_SendByte(ttyIndex, 'c');
        usleep(100000);
        retries++;
        fprintf(stderr, "retry %d\n", retries);
    }

    handleNewByte(inByte);
    return 0;
}

void endInputHandler(void) {
    RS232_SendByte(ttyIndex, 'e');
    RS232_CloseComport(ttyIndex);
}

void handleXMegaInput(void) {

    uint8_t inByte = 0;
    if(RS232_PollComport(ttyIndex, &inByte, 1) <= 0) return;

    uint16_t inputBufferSize = INITIAL_INPUT_BUFFER_SIZE;
    uint8_t *inputBuffer = (uint8_t *) malloc(inputBufferSize);
    uint16_t inputBufferIndex = 0;

    // Fill a buffer up with all the juicy bytes coming into the system.
    do {
        inputBuffer[inputBufferIndex] = inByte;
        inputBufferIndex++;

        if(inputBufferIndex == inputBufferSize) {
            inputBufferSize += INITIAL_INPUT_BUFFER_SIZE;
            inputBuffer = (uint8_t *) realloc(inputBuffer, inputBufferSize);
        }
    } 
    while(RS232_PollComport(ttyIndex, &inByte, 1) > 0);

    const uint16_t byteAmount = inputBufferIndex;
    inputBufferIndex = 0;

    // Only then will we interpret all these bytes.
    while(inputBufferIndex < byteAmount) {
        handleNewByte(inputBuffer[inputBufferIndex]);
        inputBufferIndex++;
    }

    free(inputBuffer);
}

void handleNewByte(uint8_t newByte) {
    // A few notes on this function:
    // If this function is not run faster than the bytes of the XMega are coming in, it breaks.
    // This can be fixed by running the function in a while loop until the byte buffer is empty,
    // but I don't think that will be necessary, as the PI is quite fast.
    //
    // The function also breaks if even 1 byte of information is added/missing from the incoming data.
    // This could be fixed by detecting faulty data and trying to find a valid data packet,
    // but honestly I can't be bothered.

    static uint8_t *inputBuffer = NULL; // <- Replace with nullptr in C23 (hype).
    static uint16_t inputBufferSize = 0;
    static uint16_t inputBufferIndex = 0;

    // Resize the buffer if it's too small.
    if(inputBufferIndex >= inputBufferSize) {
        // I LOVE that realloc acts as malloc if the input pointer is a nullpointer.
        inputBuffer = realloc(inputBuffer, inputBufferSize + INPUT_BUFFER_START_SIZE);
        inputBufferSize += INPUT_BUFFER_START_SIZE;
    }

    // Put the new byte into the buffer.
    inputBuffer[inputBufferIndex] = newByte;
    if(inputBufferIndex == 0) fprintf(stderr, "\n");
    fprintf(stderr, "[%3d %02x]", inputBufferIndex, newByte);
    inputBufferIndex++;

    switch(inputBuffer[0]) {

        case ID_BYTE:
            if(inputBufferIndex < ID_SIZE) return;
            myId = inputBuffer[1];
            fprintf(stderr, "Id set to %x.\n", inputBuffer[1]);
            rePrintId();

            break;

        case FRIENDSLIST_BYTE:
            // Check if the length byte hasn't been seprintpackettont. We check this because inputBuffer [1] is still unset.
            // │                       Check if the length of the buffer is less than the predicted length (header of the buffer + (friend amount * friend size)).
            // ↓                       ↓
            if(inputBufferIndex < 2 || inputBufferIndex < FRIENDLIST_HEADER_SIZE + inputBuffer[1] * BYTES_PER_FRIEND)
                return; //! Notice that this isn't break, it's return.
                // We only want the code below this switch to run if we just parsed something.

            parseFriendsList(inputBuffer + 1);
            break;

        case MY_PAYLOAD_BYTE:
            // + 1 because of the header.
            if(inputBufferIndex < PAYLOAD_SIZE + 1) return;

            printPacketToWindow(inputBuffer + 1, diagWindow);
            break;

        case BROADCAST_BYTE:
            if(inputBufferIndex < PACKET_SIZE + 1) return;

            printPacketToWindow(inputBuffer + 1, diagWindow);
            break;

        case RELAYED_BYTE:
            if(inputBufferIndex < PACKET_SIZE + 1) return;
            
            printPacketToWindow(inputBuffer + 1, diagWindow);
            break;

        default:
            fprintf(stderr, "big problem :(\n");
       
    }
    //* This code only gets run if we just parsed a full message (return vs break in the switch/case).
    inputBufferIndex = 0;
}

// Print the packet as a string of hex chars to the given window.
// Will put the title, if not NULL, at the start of the line.
void printPacketToWindow(uint8_t *packet, WINDOW *window) {

    for(uint8_t i = 0; i < PACKET_SIZE; i += 2) {
        wattrset(window, COLOR_PAIR(HEX_VAL_EVEN_PAIR));
        wprintw(window, "%02x", packet[i]);
        wattrset(window, COLOR_PAIR(HEX_VAL_ODD_PAIR));
        wprintw(window, "%02x", packet[i + 1]);
    }

    // Newlines are automatically added by the ncurses scroll functionality.

    // Remove the color :(
    wattrset(window, 0);

    for(uint8_t i = 0; i < PACKET_SIZE; i++) {
        wprintw(window, "%c ", isprint(packet[i]) ? packet[i] : ' ');
    }
    
    wrefresh(window);
}

void transmitSomething(uint8_t destId) {
    RS232_SendByte(ttyIndex, TRANSMIT_SOMETHING_BYTE);
    RS232_SendByte(ttyIndex, destId);
    wprintw(diagWindow, "Sent something to %02x\n", destId);
}


char *getPrintable(char c) {

    switch(c) {
        case '\n':
            return "\e[32m\\n\e[0m";
        case '\r':
            return "\e[32m\\r\e[0m";
        case '\b':
            return "\e[32m\\b\e[0m";
        case '\e':
            return "\e[32m\\e\e[0m";
        case '\t':
            return "\e[32m\\t\e[0m";
        default:
            if(isprint(c)) {
                // Make buffer of 3 chars (2 chars + \0)
                static char ret[3] = "  ";
                ret[0] = c; ret[1] = ' ';
                return ret;
            }
            else return "  ";
    }
}