#include "buttons.h"
#include <rpitouch.h>

button_t makeButton(uint8_t (*callback)(void), uint32_t startCol, uint32_t startRow, uint8_t width, uint8_t height) {
    button_t button;

    button.window = newwin(height, width, startRow, startCol);
    box(button.window, 0, 0);
    
    wrefresh(button.window);

    return button;
}

uint8_t isButtonTouched(button_t button, RPiTouch_Touch_t touchPoint) {
    return touchPoint.nCol > button.window->_begx
        && touchPoint.nCol < button.window->_begx + button.window->_maxx
        && touchPoint.nRow > button.window->_begy
        && touchPoint.nRow < button.window->_begy + button.window->_maxy;
}

