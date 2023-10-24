#include <stdlib.h>
#include "mcuComm.h"

#define ID_PAIR 18
#define TABLE_HEADER_PAIR 19
#define ACCEPT_PAIR 20
#define REFUSE_PAIR 21

#define ACCEPT_COLUMN 20
#define REFUSE_COLUMN 28

WINDOW *friendWindow;
WINDOW *popup;

uint8_t friendAmount = 0;
uint8_t friendListLength = 0;
uint8_t *friendIds;

// Keep track of the clicked friend for the popup.
uint8_t popupSendId = 0;

static void printNewHeader(void);

// Has to be an initialized ncurses window with 36 columns.
void initFriendWindow(WINDOW *window) {
    friendWindow = window;
    friendListLength = getmaxy(window) - 2;
    friendIds = (uint8_t *)malloc(friendListLength * sizeof(uint8_t));

    init_pair(ID_PAIR, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(TABLE_HEADER_PAIR, COLOR_GREEN, COLOR_BLACK);
    init_pair(ACCEPT_PAIR, COLOR_BLACK, COLOR_GREEN);
    init_pair(REFUSE_PAIR, COLOR_BLACK, COLOR_RED);

    printNewHeader();
}


void parseFriendsList(uint8_t *data) {

    friendAmount = data[0];
    // Print the amount of friends in the "My %3d friends:" header.
    if(!popupSendId) mvwprintw(friendWindow, 0, 3, "%3d", friendAmount);
    wmove(friendWindow, 2, 0);

    uint8_t friend = 0;
    for(; friend < friendAmount && friend < friendListLength; friend++) {
        // + 1 because of the first byte, which is the amount of friends.
        const uint16_t index = friend * BYTES_PER_FRIEND + 1;

        // Store the ID for clicking.
        friendIds[friend] = data[index];

        wattrset(friendWindow, COLOR_PAIR(ID_PAIR));
        wprintw(friendWindow, "%02x\t", data[index]);      // ID
        wattrset(friendWindow, 0);

        wprintw(friendWindow, "%d\t", data[index + 3]);    // Trust
        wprintw(friendWindow, "%d\t", data[index + 4]);    // Active

        wattrset(friendWindow, COLOR_PAIR(ID_PAIR));
        wprintw(friendWindow, "%02x\t", data[index + 2]);  // Via
        wattrset(friendWindow, 0);

        wprintw(friendWindow, "%d\n", data[index + 1]);    // Hops
    }

    // Clear the rest of the window.
    wclrtobot(friendWindow);
    wrefresh(friendWindow);
}

// Assumes the row and col are relative to friendWindow.
void friendListClick(uint16_t row, uint16_t col) {
    // Check if click is out of bounds.
    if(col < 0 || col > getmaxx(friendWindow) || row > FRIENDLIST_HEADER_SIZE + friendAmount) return;

    if(row >= FRIENDLIST_HEADER_SIZE) {
        popupSendId = friendIds[row - FRIENDLIST_HEADER_SIZE];
        printNewHeader();
    }
    else if(popupSendId) {
        if(col >= REFUSE_COLUMN) {
            popupSendId = 0;
            printNewHeader();
        }
        else if(col >= ACCEPT_COLUMN) {
            transmitSomething(popupSendId);
            popupSendId = 0;
            printNewHeader();
        }
    }

}

void printNewHeader(void) {
    if(popupSendId == 0) {
        mvwprintw(friendWindow, 0, 0, "My %3d friends:\n", friendAmount);
        wattrset(friendWindow, COLOR_PAIR(TABLE_HEADER_PAIR));
        wprintw(friendWindow, "ID\tTrust\tActive\tVia\tHops\n");
        wattrset(friendWindow, 0);
    }
    else {
        // Print the question.
        mvwprintw(friendWindow, 0, 0, "Do you want to send|\na message to ");
        wattrset(friendWindow, COLOR_PAIR(ID_PAIR));
        wprintw(friendWindow, "%02x", popupSendId);
        wattrset(friendWindow, 0);
        wprintw(friendWindow, "?   |");

        // Print accept button.
        wattrset(friendWindow, COLOR_PAIR(ACCEPT_PAIR) | A_ITALIC);
        mvwprintw(friendWindow, 0, ACCEPT_COLUMN, "  Yes   ");
        mvwprintw(friendWindow, 1, ACCEPT_COLUMN, "        ");
        // Print the refuse button.
        wattrset(friendWindow, COLOR_PAIR(REFUSE_PAIR) | A_ITALIC);
        mvwprintw(friendWindow, 0, REFUSE_COLUMN, "  No    ");
        mvwprintw(friendWindow, 1, REFUSE_COLUMN, "        ");
        
        wattrset(friendWindow, 0);
    }
    

    wrefresh(friendWindow);
}