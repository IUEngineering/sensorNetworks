#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include <stdint.h>
#include <ncurses.h>
#include <rpitouch.h>

#define BANNER_PAIR    1
#define ACTIVE_PAIR    2
#define LINE_PAIR      3


typedef struct screenElement_t screenElement_t;

void initInterface(void);
void runInterface(void);

struct screenElement_t {
    WINDOW *window;
    uint8_t clickable;
    void (*clickCallback)(uint32_t col, uint32_t row);
    void (*initCallback)(WINDOW *win);

};

typedef struct {
    uint8_t elementCount;
    screenElement_t *elements;
}screen_t;

#endif // _INTERFACE_H_
