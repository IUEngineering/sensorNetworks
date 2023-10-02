/*
 * nrftester.c
 *
 * Created: 18/03/2021 21:13:47
 * Author : Jochem Leijenhorst
 TODO: Verander dit naar van NL naar EN! 
 Dit programma heeft variabele pipes. Deze voer je in op tera term/putty
 Er zijn 4 commandos:
 TODO: Change the commands that they are correct, since that we've implemented the / command method. 
 *  help
    Print deze lijst
 
 *  send <waarde>
    Verstuurt wat je invoert op waarde naar de geselecteerde pipe
    
 *  rpip <pipenaamm> <index>
    Verander de reading pipes

 *  chan <channel>
    Verander de channel frequentie
    
    Het programma print continu uit wat hij ontvangt.
 */ 

#define F_CPU 32000000UL

#define DEBUG 1


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

    // This bit is used to select the mode of the network node
    PORTE.PIN0CTRL = 0x03 << 3;
    PORTE.DIRCLR = PIN0_bm;

    if ((PORTE.IN & 0x01)) {
        if (DEBUG)
            printf("I am a sensor node\n");
        
        initChat();
        nrfChatLoop();

    } else {
        if (DEBUG)
            printf("I am a base-station");
        
    }
}

