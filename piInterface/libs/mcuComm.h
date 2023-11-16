#ifndef _INPUTHANDLER_H_
#define _INPUTHANDLER_H_

#include <stdint.h>
#include <ncurses.h>



#define BYTES_PER_FRIEND 5

#define MAX_FRIENDS 254
#define FRIENDLIST_WIN_HEADER_SIZE 2

extern uint8_t myId;

int8_t initInputHandler(WINDOW *friendsWindow, WINDOW *broadcastsWindow, WINDOW *payloadWindow, uint8_t *debugMode);

void handleNewByte(uint8_t newByte);
void transmitSomething(uint8_t destId);
void friendListClick(uint32_t row, uint32_t col);

// Initializes the friend list window.
void printFriendListWindow(WINDOW *win);


#endif // _INPUTHANDLER_H_