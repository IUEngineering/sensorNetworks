#ifndef _INPUTHANDLER_H_
#define _INPUTHANDLER_H_

#include <stdint.h>
#include <ncurses.h>

int8_t initInputHandler(WINDOW *friendsWin, WINDOW *diagnosticsWindow);
void handleNewByte(uint8_t newByte);
void transmitSomething(uint8_t destId);

#endif // _INPUTHANDLER_H_