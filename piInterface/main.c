//
// Created by illyau on 9/27/23.
//

// basisstation.c
//
// gcc -Wall -o basisstation basisstation.c ~/hva_libraries/rpitouch/*.c -I/home/piuser/hva_libraries/rpitouch -lncurses
// ./basisstation
//
// Author : Edwin Boer
// Version: 20200824

#include <unistd.h>
#include <stdint.h>

#include "libs/rs232.h"



int main(int nArgc, char* aArgv[]) {
    refresh();


    int nRet;

    // Start to search for the correct event-stream
    // nRet = RPiTouch_InitTouch();
    // if (nRet < 0) {
    //     printf("RaspberryPi 7\" Touch display is not found!\nError %d\n\n", nRet);
    //     return -1;
    // }

    // Init ncurses
    // initscr();
    // clear();
    // noecho();
    // cbreak();

    // initInterface();
    // refresh();


    int ttyIndex;

    if(!RS232_OpenComport(24, 115200, "8N1", 0)) ttyIndex = 24;
    else if (!RS232_OpenComport(25, 115200, "8N1", 0)) ttyIndex = 25;
    else return -1;

    fprintf(stderr, "connected to ttyACM%d\n", ttyIndex - 24);
    
    
    uint8_t inByte = 0xff;
    uint32_t retries = 0;
    while(RS232_PollComport(ttyIndex, &inByte, 1) <= 0) {
        RS232_SendByte(ttyIndex, 'c');
        usleep(100000);
        retries++;
        fprintf(stderr, "retry %d\n", retries);
    }



    while(1) {
        uint8_t inByte;
        if(RS232_PollComport(ttyIndex, &inByte, 1)) fprintf(stderr, "%02x ", inByte);
    }

    // while(1) {
    //     runInterface();
    // }

    // // Close the device
    // nRet = RPiTouch_CloseTouch();
    // if (nRet < 0) {
    //     printw("Close error %d!\n", nRet);
    // }

    // // Close ncurses
    // endwin();

    return 0;
}