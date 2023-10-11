#ifndef _BUTTONS_H_
#define _BUTTONS_H_

#include <stdint.h>
#include <ncurses.h>
#include <rpitouch.h>

typedef struct button_t button_t;

struct button_t{
    WINDOW *window;
    uint8_t wasPressed;
    void (*clickCallback)(button_t *button); 
};

uint8_t isButtonTouched(button_t button, RPiTouch_Touch_t touchPoint);
button_t *makeButton(void (*callback)(button_t *button), uint32_t startCol, uint32_t startRow, uint8_t width, uint8_t height);
void checkTouchedButtons(RPiTouch_Touch_t touchPoint);


#endif // _BUTTONS_H_
