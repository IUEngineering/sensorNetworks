//* nrftester.c
//*
//* Created: 18/03/2021 21:13:47
//* Author : Jochem Leijenhorst

#define F_CPU 32000000UL


#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>

#include "serialF0.h"
#include "clock.h"
#include "nrfChat.h"



int main(void) {
    PORTF.DIRSET = PIN0_bm | PIN1_bm;

    init_clock();
    init_stream(F_CPU);
    
    PMIC.CTRL |= PMIC_LOLVLEN_bm;
    sei();
    clear_screen();
    
    initChat();

    while (1) {
        // Get the character from the user.
        char newInputChar = uartF0_getc();

        if(newInputChar != '\0') {
           interpretChar(newInputChar);
        }

        printReceivedMessages();
    }
}

