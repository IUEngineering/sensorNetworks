#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include "mcuComm.h"
#include "friendWindow.h"
#include "serial.h"

#define INPUT_BUFFER_START_SIZE 16

// For receiving from the XMega:
#define ID_BYTE 0x00
#define ID_SIZE 3

#define FRIENDSLIST_BYTE 0x01

#define MY_PAYLOAD_BYTE 0x02
#define PAYLOAD_SIZE    31

#define RELAYED_BYTE    0x03
#define BROADCAST_BYTE  0x04
#define PACKET_SIZE     32

#define FRIENDLIST_HEADER_SIZE 3

#define HEX_VAL_EVEN_PAIR 16
#define HEX_VAL_ODD_PAIR  17

// For sending to the XMega:
#define TRANSMIT_SOMETHING_BYTE 0x01

// The amount of parity bytes each message has.
#define PARITY 1

#define FIX_AMMOUNT 2


uint8_t myId = 0;

uint8_t checkParity(uint8_t parityByte, uint8_t parity);

static char* getPrintable(char c) __attribute__((unused));
static void printPacketToWindow(uint8_t *packet, WINDOW *window, uint8_t printASCII, uint8_t length);


WINDOW *broadWindow;
WINDOW *payloadWindow;
uint8_t *debugMode;

// Assumes:
//- ncurses has been initialised.
//- `start_color()` has been run.
//- `friendsWin` is an initialized window of 28 wide and at least 4 high.
//- `broadcastWindow` is an initialized window of 64 wide and at least 2 high.
//- `packetWindow` is an initialized window of 64 wide and at least 2 high.
//  I'd put it below `broadcastWindow`.
//- `debugMode` is a pointer to the variable that stores if the program is currently in debug mode.
int8_t initInputHandler(WINDOW *friendWin, WINDOW *broadcastWin, WINDOW *payloadWin, uint8_t *dMode) {

    // Find the XMega by scanning the /dev/ directory for ttyACM(any number)
    DIR *devDir = opendir("/dev");
    struct dirent *entry;
    if(devDir == NULL) return -1;

    while((entry = readdir(devDir)) != NULL) {
        if(strncmp(entry->d_name, "ttyACM", 6)) continue;
        
        // Concatenate the name of the file to /dev/ to make /dev/ttyACMx
        char streamPath[14] = "/dev/";
        strcat(streamPath, entry->d_name);

        // Test if it can be opened.
        if(initUartStream(streamPath, B115200) != 0) break;
    }
    // If we couldn't find any openable streams, stop the program.
    if(entry == NULL) return -1;


    // Init the colors :)
    init_pair(HEX_VAL_EVEN_PAIR, COLOR_WHITE, COLOR_BLACK);  
    init_pair(HEX_VAL_ODD_PAIR, COLOR_RED, COLOR_BLACK); 


    // Set the windows.
    debugMode = dMode;
    payloadWindow = payloadWin;
    broadWindow = broadcastWin;
    scrollok(broadWindow, 1);
    scrollok(payloadWindow, 1);
    if(getmaxx(broadWindow) != 64) wprintw(broadWindow, "WARNING: diagnostics window should be exactly 64 cols wide!");
    if(getmaxx(payloadWindow) != 64) wprintw(payloadWindow, "WARNING: payload window should be exactly 64 cols wide!");

    initFriendWindow(friendWin);

    uint8_t inByte = 0xff;
    uint32_t retries = 0;
    while(inByte == 0xff) {
        serialPutChar('c');
        serialGetChar(&inByte);
        sleep(0.5);
        retries++;
        fprintf(stderr, "retry %d\n", retries);
    }

    handleNewByte(inByte);
    return 0;
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

    // XOR the parity.
    static uint8_t parity = 0;
    parity += newByte + 1;

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
            if(inputBufferIndex < ID_SIZE + PARITY) return;

            // Do not interpret if the parity is not zero.
            // Also if the XMega is setting the ID for the second time there's probably something wrong.
            if(checkParity(newByte, parity) || myId) break;

            myId = inputBuffer[2];
            fprintf(stderr, "Id set to %x.\n", inputBuffer[2]);
            rePrintId();

            break;

        case FRIENDSLIST_BYTE:
            // Check if the length byte hasn't been seprintpackettont. We check this because inputBuffer [1] is still unset.
            // │                       Check if the length of the buffer is less than the predicted length (header of the buffer + (friend amount * friend size)).
            // ↓                       ↓
            if(inputBufferIndex < 3 || inputBufferIndex < FRIENDLIST_HEADER_SIZE + inputBuffer[2] * BYTES_PER_FRIEND + PARITY)
                return; //! Notice that this isn't break, it's return.
                // We only want the code below this switch to run if we just parsed something.

            // Do not interpret if the parity is not zero or if we're not in debug mode.
            if(checkParity(newByte, parity) || !*debugMode) break;

            parseFriendsList(inputBuffer + 2);
            break;

        case MY_PAYLOAD_BYTE:
            // + 1 because of the header.
            if(inputBufferIndex < PAYLOAD_SIZE + 2 + PARITY) return;

            // Do not interpret if the parity is not zero or if we're not in debug mode.
            if(checkParity(newByte, parity) || !*debugMode) break;

            // TODO: DummyData here if it's not debug mode.
            printPacketToWindow(inputBuffer + 2, payloadWindow, 1, PAYLOAD_SIZE);
            break;

        case BROADCAST_BYTE:
            if(inputBufferIndex < PACKET_SIZE + 2 + PARITY) return;

            // Do not interpret if the parity is not zero or if we're not in debug mode.
            if(checkParity(newByte, parity) || !*debugMode) break;

            printPacketToWindow(inputBuffer + 2, broadWindow, 0, PACKET_SIZE);
            break;

        case RELAYED_BYTE:
            if(inputBufferIndex < PACKET_SIZE + 2 + PARITY) return;

            // Do not interpret if the parity is not zero or if we're not in debug mode.
            if(checkParity(newByte, parity)) break;
            
            printPacketToWindow(inputBuffer + 2, payloadWindow, 1, PACKET_SIZE);
            break;

        default:
            fprintf(stderr, "big problem :(\n");
       
    }
    //* This code only gets run if we just parsed a full message (return vs break in the switch/case).
    if(checkParity(newByte, parity)) {
        fprintf(stderr, "Bad parity :(\n");
    }
    parity = 0;
    inputBufferIndex = 0;
}

// Print the packet as a string of hex chars to the given window.
// Will put the title, if not NULL, at the start of the line.
void printPacketToWindow(uint8_t *packet, WINDOW *window, uint8_t printASCII, uint8_t length) {
    if(length == PAYLOAD_SIZE){
        wattron(window, A_BOLD);
    }
    for(uint8_t i = 0; i < length; i++) {
        wattron(window, COLOR_PAIR(HEX_VAL_EVEN_PAIR + (i & 1)));
        wprintw(window, "%02x", packet[i]);
    }

    // Newlines are automatically added by the ncurses scroll functionality.
    if(length * 2 < getmaxx(window)) wprintw(window, "\n");
    // Remove the color :(
    wattrset(window, 0);
    if(printASCII){
        for(uint8_t i = 0; i < length; i++) {
            wprintw(window, "%c ", isprint(packet[i]) ? packet[i] : ' ');
        }
        if(length * 2 < getmaxx(window)) wprintw(window, "\n");
    }
    
    wrefresh(window);
}

void transmitSomething(uint8_t destId) {
    serialPutChar(TRANSMIT_SOMETHING_BYTE);
    serialPutChar(destId);
    wprintw(broadWindow, "Sent something to %02x\n", destId);
}

uint8_t checkParity(uint8_t parityByte, uint8_t parity) {
//    fprintf(stderr, "Check parrity | Par: %x | ParB: %x | Calc: %x |\n", parity, parityByte, (uint8_t) (parity - parityByte - (uint8_t) 1));
    return (uint8_t) (parity - parityByte - (uint8_t) 1) != parityByte;
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