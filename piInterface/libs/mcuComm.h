#ifndef _INPUTHANDLER_H_
#define _INPUTHANDLER_H_

#include <stdint.h>
#include <ncurses.h>


#define BYTES_PER_FRIEND 5

// This holds for both the rows in the window and the bytes in the transmission lmao.
#define FRIENDLIST_HEADER_SIZE 2
#define MAX_FRIENDS 254

extern uint8_t myId;

int8_t initInputHandler(WINDOW *friendsWin, WINDOW *diagnosticsWindow);

void handleNewByte(uint8_t newByte);
void transmitSomething(uint8_t destId);
void friendListClick(uint32_t row, uint32_t col);

void printNewHeader(WINDOW *win);


#endif // _INPUTHANDLER_H_