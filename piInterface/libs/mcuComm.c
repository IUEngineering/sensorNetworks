#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include "mcuComm.h"
#include "serial.h"

#define INPUT_BUFFER_START_SIZE 64

#define ID_BYTE 0x00
#define ID_SIZE 2

#define FRIENDSLIST_BYTE 0x01

#define MY_PAYLOAD_BYTE 0x02
#define PAYLOAD_SIZE    31

#define RELAYED_BYTE    0x03
#define BROADCAST_BYTE  0x04
#define PACKET_SIZE     32

#define BYTES_PER_FRIEND 5
#define FRIENDLIST_HEADER_SIZE 2
#define MAX_FRIENDS 254

#define MAX_INPUT_BUFFER_SIZE   MAX_FRIENDS * BYTES_PER_FRIEND + FRIENDLIST_HEADER_SIZE


void parseFriendsList(uint8_t *data);
void parsePayload(uint8_t *payload);
void parseBroadcast(uint8_t *packet);
void parseRelay(uint8_t *packet);

char* getPrintable(char c);


int8_t initInputHandler(void) {

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

    uint8_t inByte = 0;
    uint32_t retryCount = 0;
    do {
        sleep(0.1);
        fflush(stdout);
        serialPutChar('c');
    }
    while(serialGetChar(&inByte));

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
    static uint8_t myId = 0;

    // Resize the buffer if it's too small.
    if(inputBufferIndex >= inputBufferSize) {
        // I LOVE that realloc acts as malloc if the input pointer is a nullpointer.
        inputBuffer = realloc(inputBuffer, inputBufferSize + INPUT_BUFFER_START_SIZE);
        inputBufferSize += INPUT_BUFFER_START_SIZE;
    }

    // Put the new byte into the buffer.
    inputBuffer[inputBufferIndex] = newByte;
    inputBufferIndex++;


    switch(inputBuffer[0]) {

        case ID_BYTE:
            if(inputBufferIndex < ID_SIZE) return;
            myId = inputBuffer[1];

            break;

        case FRIENDSLIST_BYTE:
            // Check if the length byte hasn't been sent. We check this because inputBuffer [1] is still unset.
            // │                       Check if the length of the buffer is less than the predicted length (header of the buffer + (friend amount * friend size)).
            // ↓                       ↓
            if(inputBufferIndex < 2 || inputBufferIndex < FRIENDLIST_HEADER_SIZE + inputBuffer[1] * BYTES_PER_FRIEND)
                return; //! Notice that this isn't break, it's return.
                // I only want the code below this switch to run if we just parsed something.

            parseFriendsList(inputBuffer);
            break;

        case MY_PAYLOAD_BYTE:
            // + 1 because of the header.
            if(inputBufferIndex < PAYLOAD_SIZE + 1) return;

            parsePayload(inputBuffer + 1);
            break;

        case BROADCAST_BYTE:
            if(inputBufferIndex < PACKET_SIZE + 1) return;

            parseBroadcast(inputBuffer + 1);
            break;

        case RELAYED_BYTE:
            if(inputBufferIndex < PACKET_SIZE + 1) return;
            
            parseRelay(inputBuffer + 1);
            break;

        default: return; 
       
    }
    //* This code only gets run if we just parsed a full message (return vs break in the switch/case).
    fflush(stdout);
    inputBufferIndex = 0;
}

void parseFriendsList(uint8_t *data) {
  
}

void parsePayload(uint8_t *packet) {
   
}

void parseBroadcast(uint8_t *packet) {
    
}

void parseRelay(uint8_t *packet) {
   
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
