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

#include "libs/serial.h"

void printThingHehe(void);

int main(int nArgc, char* aArgv[]) {
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


    // The most obscene form of short-circuiting you will ever see
    initUartStream("ttyACM0", B115200) || initUartStream("ttyACM1", B115200);

    fprintf(stderr, "connected to ttyACM%d\n", ttyIndex - 24);
    
    
    uint8_t inByte = 0xff;
    uint32_t retries = 0;
    while(serialGetChar(&inByte)) {
        serialPutChar('c');
        usleep(100000);
        retries++;
        fprintf(stderr, "retry %d\n", retries);
    }

    uint8_t prevByte = 0;
    while(1) {
        uint8_t inByte;
        if(serialGetChar(&inByte)) fprintf(stderr, "%02x ", inByte);
        if(inByte == 0x12 && prevByte == 0x12) {
            printf("Bruh moment detected\n");
            printThingHehe();
        }
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






























































void printThingHehe(void) {
    printf("   ⠀⠀⠀⠀⠀⠀⠀⣠⣤⣤⣤⣤⣤⣶⣦⣤⣄⡀⠀⠀⠀⠀⠀⠀⠀⠀ \n");
    printf("⠀⠀⠀⠀⠀⠀⠀⢀⣴⣿⡿⠛⠉⠙⠛⠛⠛⠛⠻⢿⣿⣷⣤⡀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⣼⣿⠋⠀⠀⠀⠀⠀⠀⠀⢀⣀⣀⠈⢻⣿⣿⡄⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⣸⣿⡏⠀⠀⠀⣠⣶⣾⣿⣿⣿⠿⠿⠿⢿⣿⣿⣿⣄⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⣿⣿⠁⠀⠀⢰⣿⣿⣯⠁⠀⠀⠀⠀⠀⠀⠀⠈⠙⢿⣷⡄⠀\n");
    printf("⠀⣀⣤⣴⣶⣶⣿⡟⠀⠀⠀⢸⣿⣿⣿⣆⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⣷⠀\n");
    printf("⢰⣿⡟⠋⠉⣹⣿⡇⠀⠀⠀⠘⣿⣿⣿⣿⣷⣦⣤⣤⣤⣶⣶⣶⣶⣿⣿⣿⠀\n");
    printf("⢸⣿⡇⠀⠀⣿⣿⡇⠀⠀⠀⠀⠹⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠃⠀\n");
    printf("⣸⣿⡇⠀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠉⠻⠿⣿⣿⣿⣿⡿⠿⠿⠛⢻⣿⡇⠀⠀\n");
    printf("⣿⣿⠁⠀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣧⠀⠀\n");
    printf("⣿⣿⠀⠀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⠀⠀\n");
    printf("⣿⣿⠀⠀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⣿⠀⠀\n");
    printf("⢿⣿⡆⠀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣿⡇⠀⠀\n");
    printf("⠸⣿⣧⡀⠀⣿⣿⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⣿⠃⠀⠀\n");
    printf("⠀⠛⢿⣿⣿⣿⣿⣇⠀⠀⠀⠀⠀⣰⣿⣿⣷⣶⣶⣶⣶⠶⠀⢠⣿⣿⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⣿⣿⠀⠀⠀⠀⠀⣿⣿⡇⠀⣽⣿⡏⠁⠀⠀⢸⣿⡇⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⣿⣿⠀⠀⠀⠀⠀⣿⣿⡇⠀⢹⣿⡆⠀⠀⠀⣸⣿⠇⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⢿⣿⣦⣄⣀⣠⣴⣿⣿⠁⠀⠈⠻⣿⣿⣿⣿⡿⠏⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠈⠛⠻⠿⠿⠿⠿⠋⠁⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");
}