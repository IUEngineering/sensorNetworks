/*
 * nrftester.c
 *
 * Created: 18/03/2021 21:13:47
 * Author : Jochem Leijenhorst
 
 Dit programma heeft variabele pipes. Deze voer je in op tera term/putty
 Er zijn 4 commandos:
 
 *    help
    print deze lijst
 
 *    send <waarde>
    verstuurt wat je invoert op waarde naar de geselecteerde pipe
    
 *    wpip <pipenaam>
    verander de writing pipe
    
 *    rpip <pipenaamm> <index>
    verander de reading pipes
    
    Het programma print continu uit wat hij ontvangt.
 */ 

#define F_CPU    32000000


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
    
    // Send welcome message
    printf("Welkom bij de nrftester\nGemaakt door Jochem Leijenhorst.\n\nTyp help voor een lijst met commando's.\n");
    for(uint8_t i = 0; i < 64; i++) uartF0_putc('-');
    printf("\n\n");
    nrfInit(54);


    while (1) {
        
        // When something was received:
        if(receivedPacket[0] != '\0') {
            uint8_t inputLength = getUserInputLength();

            // Handle the user typing while something has been received.
            if(inputLength > 0) {
                printf("\rReceived: %s", receivedPacket);

                // Calculate how many characters of the user written command are left after printing the received packet.
                int16_t trailingCharacters = inputLength - strlen(receivedPacket) - 10;

                // Print that many spaces
                for(int16_t i = 0; i < trailingCharacters; i++)
                    printf(" ");

                // Print the user inputted buffer (make sure there is a terminating \0 character so printf stops at the right place).
                printf("\n%s", getCurrentInputBuffer());
            }
            else printf("Received: %s\n", receivedPacket);

            // Prevent the packet from being printed multiple times.
            receivedPacket[0] = 0;
        }

        // Get the character from the user.
        char newInputChar = uartF0_getc();

        if(newInputChar != '\0') {
           interpretNewChar(newInputChar);
        }
    }
}

