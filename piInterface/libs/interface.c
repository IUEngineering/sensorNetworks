#include <rpitouch.h>
#include <unistd.h>
#include <signal.h>


#include "interface.h"
#include "mcuComm.h"
#include "serial.h"
#include "dummyData.h"

// 100x30 columns max

#define RPITOUCH_SCRIPT_RELOAD_PROGRAM "/home/piuser/sensorNetworks/piInterface/reloadProgram.sh"
#define SHUTDOWN_SCRIPT "~/hva_libraries/rpitouch/shellscripts/rpitouch_shutdown.sh"

#define TEST_PAIR 9
#define ACTIVE_PAIR 2

#define SCREEN_WIDTH 100
#define SCREEN_HEIGHT 30

#define BROADCAST_WIDTH 64
#define BROADCAST_COL 1

#define BROADCAST_HEIGHT 15

#define MAIN_SCREEN_ELEMENTS 32
#define MENU_SCREEN_ELEMENTS 32 
#define DEBUG_SCREEN_ELEMENTS 32 


static uint8_t isButtonTouched(screenElement_t button, RPiTouch_Touch_t touchPoint);
static void checkTouchedButtons(screen_t screen, RPiTouch_Touch_t touchPoint);

static screen_t debugScreen = {0};
static screen_t metaScreen = {0};

static void resetButtonPressed(uint32_t row, uint32_t col);
static void shutdownButtonPressed(uint32_t row, uint32_t col) __attribute__((unused));
void switchScreenButtonPressed(uint32_t row, uint32_t col);
void drawSwitchButton(WINDOW *window);
void drawShutdownButton(WINDOW* win);

static void drawButton(WINDOW *win);

static void initDebugScreen(void);
static void initMetaScreen(void);
static void initTouchCoords(WINDOW *win);
static void printVerticalLine(WINDOW *window);
static void printHorizontalLine(WINDOW *window);
static void printLine(WINDOW *window); 

static screenElement_t addScreenElement(
    screen_t *screen, uint32_t startRow, uint32_t startCol, uint8_t height, uint8_t width, void (*clickCallback)(uint32_t, uint32_t), void (*initCallback)(WINDOW *win)
);
static void drawScreen(screen_t screen);
static void drawTouchCoords(void);
void endInterface(int idk);

static screenElement_t coordElement; 
static uint8_t debugMode = 1;
static screenElement_t debugDataElement;
static screenElement_t metaDataElement;

void initInterface(void) {

    signal(SIGINT, endInterface);

    refresh();
    start_color();
    curs_set(0);



    screenElement_t payloadElement = addScreenElement(&debugScreen, 16, 1, 15, 64, NULL, NULL);
    screenElement_t friendsElement = addScreenElement(&debugScreen, 5, 67, 19, 30, friendListClick, printFriendListWindow);
    coordElement = addScreenElement(&debugScreen, 0, 84, 4, 8, NULL, initTouchCoords);
    addScreenElement(&debugScreen, 0, 67, 4, 8, shutdownButtonPressed, drawShutdownButton);

    addScreenElement(&debugScreen, 0, BROADCAST_WIDTH + BROADCAST_COL, 0, 1, NULL, printLine);
    addScreenElement(&debugScreen, BROADCAST_HEIGHT, 0, 1, BROADCAST_WIDTH + BROADCAST_COL, NULL, printHorizontalLine);
    addScreenElement(&debugScreen, 4, 65, 1, 35, NULL, printLine);

    fprintf(stderr, "ik haat katten!!! %d %d\n", getbegy(friendsElement.window), getbegx(friendsElement.window));


    init_pair(BANNER_PAIR, COLOR_WHITE, COLOR_MAGENTA);
    init_pair(TEST_PAIR, COLOR_GREEN, 0);
    init_pair(LINE_PAIR, 0, COLOR_WHITE);
    init_pair(RED_PAIR, COLOR_WHITE, COLOR_RED);
  
    //addScreenElement(&debugScreen, 0, 92, 4, 8, resetButtonPressed, drawButton);
    
    addScreenElement(&debugScreen, 0, 92, 4, 8, switchScreenButtonPressed, drawSwitchButton);    
    fprintf(stderr, "adding new thingy %d %p\n", debugScreen.elementCount, debugScreen.elements);



    screenElement_t broadcastElement = addScreenElement(&debugScreen, 0, 1, 15, 64, NULL, NULL);
    debugDataElement = addScreenElement(&debugScreen, 24, 66, 6, 34, NULL, initDummyData);

    if(initInputHandler(friendsElement.window, broadcastElement.window, payloadElement.window, &debugMode)) {
        fprintf(stderr, "Could not find xMage (ﾉ´ｰ`)ﾉ\n");


        endwin();
        exit(-1);
    }

    // Meta Screen
    addScreenElement(&metaScreen, 0, 92, 4, 8, switchScreenButtonPressed, drawSwitchButton);
    metaDataElement = addScreenElement(&metaScreen, 9, 32, 6, 34, NULL, initDummyData);
    addScreenElement(&metaScreen, 16, 32, 40, 50, NULL, initMetaWindow);
    // addScreenElement(&metaScreen, 16, 1, 20, 20, NULL, drawMetaConclusions);
    // addScreenElement(&metaScreen, 0, 0, 20, 20, NULL, )
    refresh();
    initDebugScreen();

}

void endInterface(int idk) {
    endwin();
    serialPutChar('e');

    uint8_t inByte;
    while(serialGetChar(&inByte) == 0);

    exitUartStream();
    fprintf(stderr, "\nProgram ended by SIGINT\n");
    printf("cya :)\n");
    exit(EXIT_FAILURE);
}


void runInterface(void) {
    static uint8_t wasScreenTouched = 0;
    static uint16_t runAmount = 0;
    
    
    if(RPiTouch_UpdateTouch() && _oRPiTouch_Touched.bButton == 1 && wasScreenTouched == 0) {
        if(debugMode) checkTouchedButtons(debugScreen, _oRPiTouch_Touched);
        else checkTouchedButtons(metaScreen, _oRPiTouch_Touched);
    }

    if (debugMode) {
        drawTouchCoords();
    }

    if(++runAmount == 1000) {
        if(debugMode) drawDummyData(debugDataElement.window);
        else drawDummyData(metaDataElement.window);
        runAmount = 0;
    }

    
    wasScreenTouched = _oRPiTouch_Touched.bButton; 

    uint8_t inByte;
    if(serialGetChar(&inByte) == 0) handleNewByte(inByte);
}

void resetButtonPressed(uint32_t row, uint32_t column) {
    system(RPITOUCH_SCRIPT_RELOAD_PROGRAM);
}

void shutdownButtonPressed(uint32_t row, uint32_t col) {
    system(SHUTDOWN_SCRIPT);
}

void switchScreenButtonPressed(uint32_t row, uint32_t col) {
    debugMode = !debugMode;
    clear();
    refresh();
    if(debugMode){
        drawScreen(debugScreen);
    }
    else drawScreen(metaScreen);
    
}
void drawSwitchButton(WINDOW *window) {
    box(window, 0, 0);
    mvwprintw(window, 1, 1, "Switch");
}



void drawTouchCoords(void) {
    mvwprintw(coordElement.window, 1, 1, "y:%3d", _oRPiTouch_Touched.nRow);
    mvwprintw(coordElement.window, 2 , 1, "x:%3d", _oRPiTouch_Touched.nCol);

    wrefresh(coordElement.window);
}

void initTouchCoords(WINDOW *win) {
    box(win, 0, 0);
    mvwprintw(win, 0, 1, "Coords");
    wrefresh(win);
}

void initDebugScreen(void) {
    refresh();
    drawScreen(debugScreen);
}

void initMetaScreen(void) {
    refresh();
    drawScreen(metaScreen);
}

void drawScreen(screen_t screen) {
    for(uint8_t i = 0; i < screen.elementCount; i++) {
        wclear(screen.elements[i].window);
        if(is_scrollok(screen.elements[i].window)) wmove(screen.elements[i].window, 0, 0);
        if(screen.elements[i].initCallback) {
            screen.elements[i].initCallback(screen.elements[i].window);
        }

        wrefresh(screen.elements[i].window);
    }
}


screenElement_t addScreenElement(screen_t *screen, uint32_t startRow, uint32_t startCol, uint8_t height, uint8_t width, 
    void (*clickCallback)(uint32_t, uint32_t), void (*initCallback)(WINDOW *win)) 
{

    screen->elements = (screenElement_t *) realloc(screen->elements, sizeof(screenElement_t) * (screen->elementCount + 1));
    screen->elements[screen->elementCount].window = newwin(height, width, startRow, startCol);

    screen->elements[screen->elementCount].clickCallback = clickCallback;
    screen->elements[screen->elementCount].initCallback = initCallback;
    screen->elementCount++;

    return screen->elements[screen->elementCount - 1];
}


uint8_t isButtonTouched(screenElement_t button, RPiTouch_Touch_t touchPoint) {
    return touchPoint.nCol >= button.window->_begx
        && touchPoint.nCol <= button.window->_begx + button.window->_maxx
        && touchPoint.nRow >= button.window->_begy
        && touchPoint.nRow <= button.window->_begy + button.window->_maxy;
}

void checkTouchedButtons(screen_t screen, RPiTouch_Touch_t touchPoint) {
    for(uint8_t i = 0; i < screen.elementCount; i++) {
        if(screen.elements[i].clickCallback && isButtonTouched(screen.elements[i], touchPoint)) {
            fprintf(stderr, "Window %d is touched :D\n", i);
            screen.elements[i].clickCallback(
                touchPoint.nRow - getbegy(screen.elements[i].window), 
                touchPoint.nCol - getbegx(screen.elements[i].window)
            );
        }
    }
}

void drawButton(WINDOW *win) {
    box(win, 0, 0);
    wrefresh(win);
}

void drawShutdownButton(WINDOW *win) {
    box(win, 0, 0);
    mvwprintw(win, 1, 2, "SHUT");
    mvwprintw(win, 2, 2, "DOWN");
    wrefresh(win);
}

static void printVerticalLine(WINDOW *window) {
    // Print separation lines 
    wmove(window, 0, 0);
    wattrset(window, COLOR_PAIR(LINE_PAIR));

    // Vertical line between broadcasts and friend list.
    for(uint8_t i = 0; i < SCREEN_HEIGHT; i++) {
        mvwaddch(window, i, 0, ' ');
    }

    wattrset(window, 0);
}

static void printHorizontalLine(WINDOW *window) {
    wmove(window, 0, 0);
    // Print separation lines 
    wattrset(window, COLOR_PAIR(LINE_PAIR));

    // Horizontal line between broadcasts and relays.
    for(uint8_t i = 0; i < BROADCAST_COL + BROADCAST_WIDTH + 1; i++) {
        waddch(window, ' ');
    }

    mvwaddch(window, 0, 5, ACS_DARROW);
    waddch(window, ACS_DARROW);
    wprintw(window, " Relays & payloads "); 

    waddch(window, ACS_DARROW);
    waddch(window, ACS_DARROW);

    wmove(window, 0, 40);

    waddch(window, ACS_UARROW);
    waddch(window, ACS_UARROW);
    wprintw(window, " Broadcasts "); 
    waddch(window, ACS_UARROW);
    waddch(window, ACS_UARROW);

    attrset(0);

}

static void printLine(WINDOW *window) {
    wmove(window, 0, 0);
    wattrset(window, COLOR_PAIR(LINE_PAIR));
    // Horizontal line between broadcasts and relays.
    for(uint8_t i = 0; i < getmaxx(window) * getmaxy(window); i++) {
        waddch(window, ' ');
    }
}

