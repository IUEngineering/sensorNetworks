#include <rpitouch.h>
#include <unistd.h>
#include <signal.h>


#include "interface.h"
#include "mcuComm.h"
#include "serial.h"

// 100x30 columns max

#define RPITOUCH_SCRIPT_RELOAD_PROGRAM "/home/piuser/sensorNetworks/piInterface/reloadProgram.sh"
#define SHUTDOWN_SCRIPT "~/hva_libraries/rpitouch/shellscripts/rpitouch_shutdown.sh"

#define TEST_PAIR 9
#define ACTIVE_PAIR 2

#define SCREEN_WIDTH 100
#define SCREEN_HEIGHT 30

#define BROADCAST_WIDTH 64
#define BROADCAST_COL 1

#define MAIN_SCREEN_ELEMENTS 32
#define MENU_SCREEN_ELEMENTS 32 
#define DEBUG_SCREEN_ELEMENTS 32 




static uint8_t isButtonTouched(screenElement_t button, RPiTouch_Touch_t touchPoint);
static void checkTouchedButtons(screen_t screen, RPiTouch_Touch_t touchPoint);

static screen_t debugScreen = {0};

static void resetButtonPressed(uint32_t row, uint32_t col);
static void shutdownButtonPressed(uint32_t row, uint32_t col) __attribute__((unused));

static void drawButton(WINDOW *win);

static void initDebugScreen(void);
static void initTouchCoords(WINDOW *win);

screenElement_t addScreenElement(
    screen_t *screen, uint32_t startRow, uint32_t startCol, uint8_t height, uint8_t width, void (*clickCallback)(uint32_t, uint32_t), void (*initCallback)(WINDOW *win)
);
static void drawScreen(screen_t screen);
static void drawTouchCoords(void);
void endInterface(int idk);

screenElement_t coordElement; 

void initInterface(void) {

    signal(SIGINT, endInterface);

    refresh();
    start_color();
    curs_set(0);


    screenElement_t friendsElement;
    screenElement_t diagElement;

    friendsElement = addScreenElement(&debugScreen, 16, 1, 10, 30, friendListClick, printNewHeader);
    diagElement = addScreenElement(&debugScreen, 0, 1, 15, 64, NULL, NULL);

    fprintf(stderr, "ik haat katten!!! %d %d\n", getbegy(friendsElement.window), getbegx(friendsElement.window));

    if(initInputHandler(friendsElement.window, diagElement.window)) {
        fprintf(stderr, "Could not find xmege\n");

        endwin();
        exit(-1);
    }

    init_pair(BANNER_PAIR, COLOR_WHITE, COLOR_MAGENTA);
    init_pair(TEST_PAIR, COLOR_GREEN, 0);
    init_pair(LINE_PAIR, 0, COLOR_WHITE);
  
    coordElement = addScreenElement(&debugScreen, 4, 92, 3, 8, NULL, initTouchCoords);
    addScreenElement(&debugScreen, 0, 92, 4, 8, resetButtonPressed, drawButton);
    addScreenElement(&debugScreen, 26, 92, 4, 8, shutdownButtonPressed, drawButton);

    // Print separation lines 
    attrset(COLOR_PAIR(LINE_PAIR));

    // Vertical line between broadcasts and friend list.
    for(uint8_t i = 0; i < SCREEN_HEIGHT; i++) {
        static const char broadcastText[] = "Broadcasts";
        if(i < sizeof(broadcastText)) mvaddch(i, BROADCAST_COL + BROADCAST_WIDTH, broadcastText[i]);
        else mvaddch(i, BROADCAST_COL + BROADCAST_WIDTH, ' ');
    }

    // Horizontal line between broadcasts and relays.
    mvprintw(BROADCAST_HEIGHT, 0, "Relays & payloads:");
    for(uint8_t i = sizeof("Relays & payloads:"); i < BROADCAST_COL + BROADCAST_WIDTH; i++) {
        addch(' ');
    }

    attset(0);

    fprintf(stderr, "adding new thingy %d %p\n", debugScreen.elementCount, debugScreen.elements);

    clear();    

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

    if (RPiTouch_UpdateTouch()) {
        drawTouchCoords();
        if(_oRPiTouch_Touched.bButton == 1 && wasScreenTouched == 0) {
            checkTouchedButtons(debugScreen, _oRPiTouch_Touched);
        }
    }

    drawTouchCoords();
    
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

void drawTouchCoords(void) {
    mvwprintw(coordElement.window, 1, 1, "%2d,%3d", _oRPiTouch_Touched.nRow , _oRPiTouch_Touched.nCol);
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

void drawScreen(screen_t screen) {
    for(uint8_t i = 0; i < screen.elementCount; i++) {

        if(screen.elements[i].initCallback) {
            screen.elements[i].initCallback(screen.elements[i].window);
        }
        else fprintf(stderr, "no callback\n");

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
            screen.elements[i].clickCallback(touchPoint.nRow, touchPoint.nCol);
        }
    }
}

void drawButton(WINDOW *win) {
    box(win, 0, 0);
    wrefresh(win);
}