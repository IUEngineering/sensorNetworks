#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include <stdint.h>
#include <ncurses.h>
#include <rpitouch.h>
#include "buttons.h"

#define BANNER_PAIR    1
#define ACTIVE_PAIR    2
#define WATER_PAIR     2
#define MOUNTAIN_PAIR  3
#define PLAYER_PAIR    4


typedef struct screenElement_t screenElement_t;


void initInterface(void);
void runInterface(void);
void resetButtonPressed(button_t *button);
void shutdownThePi(button_t *button);


struct screenElement_t {
    WINDOW *window;
    uint8_t clickable;
    void (*clickCallback)(uint32_t col, uint32_t row);
    void (*initCallback)(WINDOW *window);

};

typedef struct {
    uint8_t elementCount;
    screenElement_t *elements;
}screen_t;

WINDOW *getDataWindow(void);


#endif // _INTERFACE_H_
