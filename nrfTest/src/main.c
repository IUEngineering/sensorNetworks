//* nrftester.c
//*
//* Created: 18/03/2021 21:13:47
//* Author : Jochem Leijenhorst

#define F_CPU 32000000UL

#define DEBUG 0

#define ENTER 0x0D

#include <avr/io.h>
#include <avr/interrupt.h>

#include "serialF0.h"
#include "clock.h"
#include "nrfChat.h"
#include "baseStation.h"
#include "dummyData.h"



int main(void) {
    PORTF.DIRSET = PIN0_bm | PIN1_bm;

    init_clock();
    init_stream(F_CPU);
    
    PMIC.CTRL |= PMIC_LOLVLEN_bm;
    sei();

    // This bit is used to select the mode of the network node
    PORTE.PIN0CTRL = PORT_OPC_PULLUP_gc;
    PORTE.DIRCLR = PIN0_bm | PIN1_bm;

    while(1) {
        if ((PORTE.IN & PIN0_bm) && !(PORTE.IN & PIN1_bm)) {
            if (DEBUG)
                printf("I am a debug node\n");
                
            clear_screen();
            nrfChatInit();
            nrfChatLoop();

        }
        else if (!(PORTE.IN & PIN0_bm) && !(PORTE.IN & PIN1_bm)) {
            if (DEBUG)
                printf("I am a base-station\n");
            
            baseStationInit();
            baseStationLoop();
        } 
        else if (!(PORTE.IN & PIN0_bm) && (PORTE.IN & PIN1_bm)) {
            if (DEBUG)
                printf("I am a sensor node\n");
            
            dummyDataInit();
            dummyDataLoop();
        } 
        else {
            printf("I am not correctly configured please check the documentation how to slect the mode of this node.\n");
            printf("In case this problem can not be resolved please contact the system designers\n");
            printf("\n");
            printf("Press enter to try again to start a program.");

            while (uartF0_getc() != ENTER);
        }
    }

}

