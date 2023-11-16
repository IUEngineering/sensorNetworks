// serial lib rewritten for C, inspired by Edwin Boer's serial C++ lib
// Author : IUE 2023


#include "serial.h"
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

int streamIndex = 0; 

uint8_t initUartStream(char *name, uint32_t baudrate) {

    struct termios ttyConfig;

    streamIndex = open(name, O_RDWR | O_NONBLOCK);
    if (streamIndex <= 0){
        perror(name);
        return 0;
    }

    ttyConfig.c_iflag = 0; // Input
    ttyConfig.c_oflag = 0; // Output
    ttyConfig.c_lflag = 0; // Line
    ttyConfig.c_cflag = CS8 | CREAD | CLOCAL; // Character
    ttyConfig.c_cc[VMIN] = 1; // Length
    ttyConfig.c_cc[VTIME] = 0; // Timeout
    cfsetispeed(&ttyConfig, baudrate); // Input speed
    cfsetospeed(&ttyConfig, baudrate); // Output speed

    // Clear buffers 
    tcflush(streamIndex, TCIFLUSH);
    

    // Set new settings NOW
    tcsetattr(streamIndex, TCSANOW, &ttyConfig);

    return 1;
}

uint8_t serialGetChar(uint8_t *byte) {
    ssize_t size;

    if (streamIndex <= 0) {
        return SERIAL_WARNING;
    }

    // Read one byte only
    size = read(streamIndex, byte, 1);
    if (size == 0){
        return SERIAL_WARNING + 1;
    }
    if (size == -1) {
         // Fatal error 6: Device not configured or disconnected
        if (errno == 6) {
            return SERIAL_WARNING + 2;
        };

        // Warning Mac 35: Resource temporarily unavailable
        if (errno == 35) {
            return 1;
        };

        // Warning Linux 11: Resource temporarily unavailable
        if (errno == 11) {
            return 2;
        };

        // Catch all
            printf("get()-error %d: %s ", errno, strerror(errno));
        return 255;
    }

    return 0;
}


uint8_t serialPutChar(uint8_t byte) {
    ssize_t size;

    // Check if the connection is valid
    if (streamIndex <= 0){
        return -1;
    }

    // Write one byte only
    size = write(streamIndex, &byte, 1);
    if (size != 0) {
        return -1;
    }

    return 0;
}


uint8_t sendBuffer(uint8_t *buffer, uint8_t length) {
    for(uint8_t i = 0; i < length; i++) {
        if(serialPutChar(buffer[i])) return -1;
    }

    return 0;
}
