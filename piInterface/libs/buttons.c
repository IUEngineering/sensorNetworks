#include "buttons.h"
#include <rpitouch.h>

#define MAX_BUTTONS 15

button_t buttons[MAX_BUTTONS];
uint8_t buttonsIndex = 0;

button_t *makeButton(void (*callback)(button_t *button), uint32_t startCol, uint32_t startRow, uint8_t width, uint8_t height) {
    buttons[buttonsIndex].wasPressed = 0;
    
    buttons[buttonsIndex].window = newwin(height, width, startRow, startCol);
    buttons[buttonsIndex].clickCallback = callback;
    box(buttons[buttonsIndex].window, 0, 0);
    wrefresh(buttons[buttonsIndex].window);
    
    //TODO: Check if array is out of mem size

    buttonsIndex++;
    return &buttons[buttonsIndex];
}

uint8_t isButtonTouched(button_t button, RPiTouch_Touch_t touchPoint) {
    return touchPoint.nCol >= button.window->_begx
        && touchPoint.nCol <= button.window->_begx + button.window->_maxx
        && touchPoint.nRow >= button.window->_begy
        && touchPoint.nRow <= button.window->_begy + button.window->_maxy;
}

void checkTouchedButtons(RPiTouch_Touch_t touchPoint) {
    for(uint8_t i = 0; i < buttonsIndex; i++) {
        if(isButtonTouched(buttons[i], touchPoint) && touchPoint.bButton) {
            if(!buttons[i].wasPressed) {
                buttons[i].clickCallback(&buttons[i]);
                buttons[i].wasPressed = 1;
                wattron(buttons[i].window, COLOR_PAIR(9));
                box(buttons[i].window, 0, 0);
                wattroff(buttons[i].window, COLOR_PAIR(9));
                wrefresh(buttons[i].window);
            }
        }
        else {
            if(buttons[i].wasPressed) {
                box(buttons[i].window, 0, 0);
                wrefresh(buttons[i].window);
            }
            buttons[i].wasPressed = 0;

        }
    }
}

