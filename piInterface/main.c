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
#include "libs/buttons.h"
#include "libs/interface.h"


int UpdateTouchWindow(WINDOW *pWin) {

    wattron(pWin, COLOR_PAIR(9));

    // Draw window border
    box(pWin, 0, 0);
    
    // Show mouse status
    mvwprintw(pWin, 1, 4, "%s", (_oRPiTouch_Touched.bButton ? "Touch :D" : "-----        "));
    mvwprintw(pWin, 2, 4, "(%4d, %4d) -> (%4d, %4d)", _oRPiTouch_Touched.nX, _oRPiTouch_Touched.nY, _oRPiTouch_Touched.nCol, _oRPiTouch_Touched.nRow);

    wattroff(pWin, COLOR_PAIR(9));
    //wie is deze man
    // Update window
    wrefresh(pWin);
    return 0;
}


int main(int nArgc, char* aArgv[]) {
    initMcuComm();
    refresh();



    int nRet;
    WINDOW *pMenuWindow;

    // mvwprintw(pMenuWindow, 1, 4, "%s", ("-----"));

    // wborder(pMenuWindow, '|', '|', '-', '-', '+', '+', '+', '+');


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


    attron(COLOR_PAIR(BANNER_PAIR));
    mvprintw(1, 1, "[ESC] to quit, LT->RT->RB to restart, LT->LB-> RB to shutdown, LB->LM->RM->RB to reboot");
    attroff(COLOR_PAIR(BANNER_PAIR));

    refresh();

    pMenuWindow = newwin(1 + 2 + RPITOUCH_DEVICE_SLOT_COUNT + 1, 40, 6, 1);
    keypad(pMenuWindow, true);
    nodelay(pMenuWindow, true);

    // Cheange touch settings
    _oRPiTouch_Settings.bRestartApply = false;
    _oRPiTouch_Settings.nRestartWait = 3;
    _oRPiTouch_Settings.bShutdownApply = false;
    _oRPiTouch_Settings.nShutdownWait = 3;

    // Do UI
    bool bExit = false;
    int nKey;

    while (!bExit) {
        //drawTouchCords(50, 22);
        runInterface();

        

        checkTouchedButtons(_oRPiTouch_Touched);

        // Update touch
        if (RPiTouch_UpdateTouch()) {
            UpdateTouchWindow(pMenuWindow);
        };

        // Check for restart and shutdown
        if (_oRPiTouch_Settings.bRestartDetected) {
            RPiTouch_CloseTouch();
            endwin();

            RPiTouch_ApplyRestart();
            return 1;
        }
        if (_oRPiTouch_Settings.bShutdownDetected) {
            RPiTouch_CloseTouch();
            endwin();

            RPiTouch_ApplyShutdown();
            return 2;
        }

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