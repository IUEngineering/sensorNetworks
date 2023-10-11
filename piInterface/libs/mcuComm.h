#ifndef _MCUCOMM_H_
#define _MCUCOMM_H_

#include <stdint.h>
#include <ncurses.h>
#include "buttons.h"

void sendC(button_t *button);
void initMcuComm(void);

#endif // _MCUCOMM_H_
