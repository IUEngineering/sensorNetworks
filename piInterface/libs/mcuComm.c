#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#include "interface.h"
#include "mcuComm.h"
#include "serial.h"

#define INPUT_BUFFER_START_SIZE 64

#define FRIENDLIST 0x01
#define PAYLOAD 0x02

#define BYTES_PER_FRIEND 5
#define FRIENDLIST_HEADER_SIZE 2
#define PAYLOAD_SIZE 31
#define MAX_FRIENDS 254

#define MAX_INPUT_BUFFER_SIZE   MAX_FRIENDS * BYTES_PER_FRIEND + FRIENDLIST_HEADER_SIZE

uint16_t inputBufferSize = INPUT_BUFFER_START_SIZE;
uint8_t *inputBuffer;
uint16_t inputBufferIndex = 0;

void parseFriendsList(uint8_t *buffer);
void parsePayload(uint8_t *buffer);
char* getPrintable(char c);

void initInputHandler(void) {
    inputBuffer = (uint8_t *)malloc(inputBufferSize);

    printf("\e[H\e[2J");
}

void handleNewByte(uint8_t newByte) {

    // A few notes on this function:
    // If this function is not run faster than the bytes of the XMega are coming in, it breaks.
    // This can be fixed by running the function in a while loop until the byte buffer is empty,
    // but I don't think that will be necessary, as the PI is quite fast.
    //
    // The function also breaks if even 1 byte of information is added/missing from the incoming data.
    // This could be fixed by detecting faulty data and trying to find a valid data packet,
    // but honestly I couln't be bothered.

    // Put the new byte into the buffer.
    inputBuffer[inputBufferIndex] = newByte;
    inputBufferIndex++;

    // Check if it's a complete friendslist.
    // Check if the first byte is the friendlist ID.          Check if the buffer has been filled to the length of the sent friend list.
    if(inputBuffer[0] == FRIENDLIST  &&  inputBufferIndex >= (FRIENDLIST_HEADER_SIZE + inputBuffer[1] * BYTES_PER_FRIEND)) {
        parseFriendsList(inputBuffer);
        fflush(stdout);
        inputBufferIndex = 0;
        return;
    }

    // Check if it's a complete message payload.
    if(inputBuffer[0] == PAYLOAD  &&  inputBufferIndex >= PAYLOAD_SIZE + 1) {
        parsePayload(inputBuffer);
        fflush(stdout);
        inputBufferIndex = 0;
        return;
    }

    // Resize the buffer if it's too small.
    if(inputBufferIndex >= inputBufferSize) {
        // if(inputBufferSize + INPUT_BUFFER_START_SIZE > MAX_INPUT_BUFFER_SIZE) {
        //     serialPutChar('e');
        //     // sleep(0.1);
        //     serialPutChar('c');
        // }
        inputBuffer = realloc(inputBuffer, inputBufferSize + INPUT_BUFFER_START_SIZE);
        inputBufferSize += INPUT_BUFFER_START_SIZE;
    }
}

void parseFriendsList(uint8_t *buffer) {
    printf("\e[0;0HFriendlist [%d %d]:\n", buffer[0], buffer[1]);
    for(uint16_t i = 0; i < buffer[1]; i++) {
        printf("[");
        for(uint8_t j = 0; j < 5; j++) printf("%02x ", buffer[2 + i * 5 + j]);
        printf("]\n");
    }
}

void parsePayload(uint8_t *buffer) {
    WINDOW *win = getDataWindow();

    
    wprintw(win, "\e[10;0HPayload:\n");
    wprintw(win, "\e[34m");
    for(uint8_t i = 0; i < PAYLOAD_SIZE; i++) {
        wprintw(win, "%02x ", buffer[i]);
    }
    wprintw(win, "\e[0m\n");

    for(uint8_t i = 0; i < PAYLOAD_SIZE; i++) {
        wprintw(win, "%s ", getPrintable(buffer[i]));
    }
    wprintw(win, "\n");

    wrefresh(win);

}


char* getPrintable(char c) {

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