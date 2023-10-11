#ifndef _INTERFACE_H_
#define _INTERFACE_H_

#include <stdint.h>
#include <ncurses.h>
#include <rpitouch.h>
#include "buttons.h"

#define BANNER_PAIR    1
#define ACTIVE_PAIR    2
#define WATER_PAIR     2
#define MOUNTAIN_PAIR  3
#define PLAYER_PAIR    4


void initInterface(void);
void runInterface(void);
void resetButtonPressed(button_t *button);
void shutdownThePi(button_t *button);

#endif // _INTERFACE_H_
