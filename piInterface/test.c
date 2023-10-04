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

#define BESTTEXT_PAIR     1
#define EMPTY_PAIR     1
#define WATER_PAIR     2
#define MOUNTAIN_PAIR  3
#define PLAYER_PAIR    4

int UpdateTouchWindow(WINDOW *pWin) {

    // Draw window border
    box(pWin, 0, 0);
    
    init_pair(BESTTEXT_PAIR, COLOR_MAGENTA, COLOR_BLACK);
    attron(COLOR_PAIR(BESTTEXT_PAIR));
    mvwprintw(pWin, 0, 1, "Hi Nielsie ");
    attroff(COLOR_PAIR(BESTTEXT_PAIR));

    // Show mouse status
    mvwprintw(pWin, 1, 4, "%s", (_oRPiTouch_Touched.bButton ? "Touch :D" : "-----        "));
    mvwprintw(pWin, 2, 4, "(%4d, %4d) -> (%4d, %4d)", _oRPiTouch_Touched.nX, _oRPiTouch_Touched.nY, _oRPiTouch_Touched.nCol, _oRPiTouch_Touched.nRow);

    // Show touches
    for (int i = 0; i < RPITOUCH_DEVICE_SLOT_COUNT; i++) {
        mvwprintw(pWin, 3 + i, 1, "T%01d", i);
        if (_aRPiTouch_Slot[i].bUsed) {
            mvwprintw(pWin, 3 + i, 4, "(%4d, %4d) -> (%4d, %4d)", _aRPiTouch_Slot[i].nX, _aRPiTouch_Slot[i].nY, _aRPiTouch_Slot[i].nCol, _aRPiTouch_Slot[i].nRow);
        }
        else {
            mvwprintw(pWin, 3 + i, 4, "-                                 ");
        }
    }

    // Update window
    wrefresh(pWin);
    return 0;
}


int main(int nArgc, char* aArgv[]) {
    refresh();



    int nRet;
    WINDOW *pMenuWindow;

    mvwprintw(pMenuWindow, 1, 4, "%s", ("-----"));

    wborder(pMenuWindow, '|', '|', '-', '-', '+', '+', '+', '+');


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

    

    attron(A_REVERSE);
    start_color();
    init_pair(BESTTEXT_PAIR, COLOR_MAGENTA, COLOR_BLACK);
    attron(COLOR_PAIR(BESTTEXT_PAIR));
    mvprintw(1, 1, "[ESC] to quit, LT->RT->RB to restart, LT->LB-> RB to shutdown, LB->LM->RM->RB to reboot");
    attroff(COLOR_PAIR(BESTTEXT_PAIR));

    refresh();
    attroff(A_REVERSE);

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

        // Check for key press
        nKey = wgetch(pMenuWindow);
        if (nKey == 27 || nKey == 'q') {
            // Exit the program
            bExit = true;
        }
        if (nKey == KEY_RESIZE) {
            // Terminal size is changed
            UpdateTouchWindow(pMenuWindow);
        }
        //refresh();
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