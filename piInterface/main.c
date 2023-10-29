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

#include <ncurses.h>
#include <rpitouch.h>

#include "libs/interface.h"




int main(int nArgc, char* aArgv[]) {
    refresh();


    int nRet;


    // Start to search for the correct event-stream
    nRet = RPiTouch_InitTouch();
    if (nRet < 0) {
        printf("RaspberryPi 7\" Touch display is not found!\nError %d\n\n", nRet);
        return -1;
    }

    // Init ncurses
    initscr();
    clear();
    noecho();
    cbreak();

    initInterface();
    refresh();

    while(1) {
        runInterface();
    }

    // Close the device
    nRet = RPiTouch_CloseTouch();
    if (nRet < 0) {
        printw("Close error %d!\n", nRet);
    }

    // Close ncurses
    endwin();

    return 0;
}