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
#include "baseStation.h"



int main(void) {
    PORTF.DIRSET = PIN0_bm | PIN1_bm;
    PORTF.OUTCLR = PIN0_bm;

    init_clock();
    init_stream(F_CPU);
    
    PMIC.CTRL |= PMIC_LOLVLEN_bm;
    sei();

    // This bit is used to select the mode of the network node
    PORTE.DIRCLR = PIN0_bm;
    PORTE.PIN0CTRL = PORT_OPC_PULLUP_gc;


    if ((PORTE.IN & PIN0_bm)) {    
        clear_screen();
        nrfChatInit();
        nrfChatLoop();

    }
    else {
        baseStationInit();
        baseStationLoop();
    }
}

