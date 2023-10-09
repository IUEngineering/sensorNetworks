#include <stdint.h>
#include <ncurses.h>
#include <rpitouch.h>


#ifndef _BUTTONS_H_
#define _BUTTONS_H_

typedef struct {
    WINDOW *window;

    uint8_t (*clickCallback)(void); 
} button_t;

uint8_t isButtonTouched(button_t button, RPiTouch_Touch_t touchPoint);
button_t makeButton(uint8_t (*callback)(void), uint32_t startCol, uint32_t startRow, uint8_t width, uint8_t height);


#endif // _BUTTONS_H_
