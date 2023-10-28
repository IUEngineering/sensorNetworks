#include <stdlib.h>
#include "mcuComm.h"
#include "friendWindow.h"


#define ID_PAIR 18
#define TABLE_HEADER_PAIR 19
#define ACCEPT_PAIR 20
#define REFUSE_PAIR 21

#define ACCEPT_COLUMN 12
#define REFUSE_COLUMN 18

// The column of the friend number, so that it can be overwritten.
#define FRIEND_AMOUNT_COLUMN 12

#define MY_ID_COLUMN 5

WINDOW *friendWindow;
WINDOW *popup;

uint8_t friendAmount = 0;
uint8_t friendListLength = 0;
uint8_t *friendIds;

// Keep track of the clicked friend for the popup.
uint8_t popupSendId = 0;


// Has to be an initialized ncurses window with 36 columns.
void initFriendWindow(WINDOW *window) {
    friendWindow = window;
    friendListLength = getmaxy(window) - 2;
    friendIds = (uint8_t *)malloc(friendListLength * sizeof(uint8_t));

    init_pair(ID_PAIR, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(TABLE_HEADER_PAIR, COLOR_GREEN, COLOR_BLACK);
    init_pair(ACCEPT_PAIR, COLOR_BLACK, COLOR_GREEN);
    init_pair(REFUSE_PAIR, COLOR_BLACK, COLOR_RED);

    printFriendListWindow(friendWindow);

    fprintf(stderr, "The win is now this hahahah %d %d\n", getbegy(friendWindow), getbegx(friendWindow));

}


void parseFriendsList(uint8_t *data) {

    friendAmount = data[0];
    fprintf(stderr, "printing %d friends %d %d\n", friendAmount, getbegy(friendWindow), getbegx(friendWindow));

    // Print the amount of friends in the "My %3d friends:" header.
    if(!popupSendId) mvwprintw(friendWindow, 0, FRIEND_AMOUNT_COLUMN, "%3d", friendAmount);
    wmove(friendWindow, 2, 0);

    uint8_t friend = 0;
    for(; friend < friendAmount && friend < friendListLength; friend++) {
        // + 1 because of the first byte, which is the amount of friends.
        const uint16_t index = friend * BYTES_PER_FRIEND + 1;

        // Store the ID for clicking.
        friendIds[friend] = data[index];

        wattrset(friendWindow, COLOR_PAIR(ID_PAIR));
        wprintw(friendWindow, "%02x   ", data[index]);      // ID
        wattrset(friendWindow, 0);

        wprintw(friendWindow, "%-2d   ", data[index + 3]);    // Trust
        wprintw(friendWindow, "%-2d   ", data[index + 4]);    // Active

        wattrset(friendWindow, COLOR_PAIR(ID_PAIR));
        wprintw(friendWindow, "%02x   ", data[index + 2]);  // Via
        wattrset(friendWindow, 0);

        wprintw(friendWindow, "%-3d", data[index + 1]);    // Hops
        wprintw(friendWindow, "\n");
    }

    // Clear the rest of the window.
    wclrtobot(friendWindow);
    wrefresh(friendWindow);
}

// Assumes the row and col are relative to friendWindow.
void friendListClick(uint32_t row, uint32_t col) {

    // Check if click is out of bounds.
    if(col < 0 || col > getmaxx(friendWindow) || row > FRIENDLIST_HEADER_SIZE + friendAmount) return;
    fprintf(stderr, "friend list click!!");
    return;

    if(row >= FRIENDLIST_HEADER_SIZE) {
        popupSendId = friendIds[row - FRIENDLIST_HEADER_SIZE];
        printFriendListWindow(friendWindow);
    }
    else if(popupSendId) {
        if(col >= REFUSE_COLUMN) {
            popupSendId = 0;
            printFriendListWindow(friendWindow);
        }
        else if(col >= ACCEPT_COLUMN) {
            transmitSomething(popupSendId);
            popupSendId = 0;
            printFriendListWindow(friendWindow);
        }
    }

}

void rePrintId(void) {
    wattrset(friendWindow, COLOR_PAIR(ID_PAIR));
    mvwprintw(friendWindow, 0, MY_ID_COLUMN, "%02x", myId);
    wattrset(friendWindow, 0);
}

void printFriendListWindow(WINDOW* win) {

    if(popupSendId == 0) {
        mvwprintw(win, 0, 0, "I am   . My %3d friends are:\n", friendAmount);

        rePrintId();

        wattrset(win, COLOR_PAIR(TABLE_HEADER_PAIR));
        mvwprintw(win, 1, 0, "ID   Trst Actv Via  Hops");
        wattrset(win, 0);
    }
    else {
        // Print the question.
        mvwprintw(win, 0, 0, "Send to ");
        wattrset(win, COLOR_PAIR(ID_PAIR));
        wprintw(win, "%02x", popupSendId);
        wattrset(win, 0);
        wprintw(win, "?|\n           |");

        // Print accept button.
        wattrset(win, COLOR_PAIR(ACCEPT_PAIR) | A_ITALIC);
        mvwprintw(win, 0, ACCEPT_COLUMN, " Yes  ");
        mvwprintw(win, 1, ACCEPT_COLUMN, "      ");
        // Print the refuse button.
        wattrset(win, COLOR_PAIR(REFUSE_PAIR) | A_ITALIC);
        mvwprintw(win, 0, REFUSE_COLUMN, " No   ");
        mvwprintw(win, 1, REFUSE_COLUMN, "      ");
        
        wattrset(win, 0);
    }
    

    wrefresh(friendWindow);
}