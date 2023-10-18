#include "interface.h"

#define RPITOUCH_SCRIPT_RELOAD_PROGRAM "/home/piuser/sensorNetworks/piInterface/reloadProgram.sh"
#define SHUTDOWN_SCRIPT "~/hva_libraries/rpitouch/shellscripts/rpitouch_shutdown.sh"

#define TEST_PAIR 9
#define ACTIVE_PAIR 2

#define MAIN_SCREEN_ELEMENTS 32
#define MENU_SCREEN_ELEMENTS 32 
#define DEBUG_SCREEN_ELEMENTS 32 

static WINDOW *posWindow;
static screen_t debugScreen;

static void initDataWindow(WINDOW *win);
static void initDebugScreen(void);
static void addScreenElement(
    screen_t *screen, uint32_t startCol, uint32_t startRow, uint8_t width, uint8_t height, void (*clickCallback)(uint32_t col, uint32_t row), void (*initCallback)(WINDOW *)
);
static void drawScreen(screen_t screen);

static void drawTouchCords(void);

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

    initDebugScreen();
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

void initDebugScreen(void) {
    debugScreen.elements = NULL;
    addScreenElement(&debugScreen, 5, 5, 50, 10, NULL, initDataWindow);
    mvprintw(0, 1, "D-MODE (DISCO MODE)");
    refresh();
    drawScreen(debugScreen);

}

void drawScreen(screen_t screen) {
    printw("kanker op %d", screen.elementCount);
    for(uint8_t i=0; i < screen.elementCount; i++) {
        mvprintw(1,0 ,"jij eenden hater");
        if(screen.elements[i].initCallback) screen.elements[i].initCallback(screen.elements[i].window);
        wrefresh(screen.elements[i].window);
    }
}


void addScreenElement(screen_t *screen, uint32_t startCol, uint32_t startRow, uint8_t width, uint8_t height, 
    void (*clickCallback)(uint32_t, uint32_t), void (*initCallback)(WINDOW *)) 
{
    screen->elements = (screenElement_t *) realloc(screen->elements, sizeof(screenElement_t) * (screen->elementCount + 1));
    screen->elements[screen->elementCount].window = newwin(height, width, startRow, startCol);

    screen->elements[screen->elementCount].clickCallback = clickCallback;
    screen->elements[screen->elementCount].initCallback = initCallback;
    screen->elementCount++;
}



void initDataWindow(WINDOW *win) {
    // wprintw(win, "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE\n");
}

WINDOW *getDataWindow(void) {

    return debugScreen.elements[0].window;
}

