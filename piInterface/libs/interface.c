#include "interface.h"
#include "mcuComm.h"

#define RPITOUCH_SCRIPT_RELOAD_PROGRAM "/home/piuser/sensorNetworks/piInterface/reloadProgram.sh"
#define SHUTDOWN_SCRIPT "~/hva_libraries/rpitouch/shellscripts/rpitouch_shutdown.sh"

#define TEST_PAIR 9
#define ACTIVE_PAIR 2

static WINDOW *posWindow;

static void drawTouchCords(void);

void kwek(button_t *fuck) {
    ;
}

void initInterface(void) {
    refresh();
    start_color();

    init_pair(BANNER_PAIR, COLOR_WHITE, COLOR_MAGENTA);
    init_pair(TEST_PAIR, COLOR_GREEN, 0);

    clear();    

    posWindow = newwin(3, 15, 4, 85);
    box(posWindow, 0, 0);
    mvwprintw(posWindow, 0, 1, "cords");


    button_t *button_reset = makeButton(resetButtonPressed, 92, 0, 8, 4);
    makeButton(sendC, 30, 2, 8, 4);
}


void runInterface(void) {
    drawTouchCords();
}

void resetButtonPressed(button_t *button) {
    system(RPITOUCH_SCRIPT_RELOAD_PROGRAM);
}

void drawTouchCords(void) {
    mvwprintw(posWindow, 1, 1, "(%4d,%4d)", _aRPiTouch_Slot[0].nCol, _aRPiTouch_Slot[0].nRow);
    wrefresh(posWindow);
}

void shutdownThePi(button_t *button) {
    system(SHUTDOWN_SCRIPT);
}
