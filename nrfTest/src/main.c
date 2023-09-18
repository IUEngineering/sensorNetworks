/*
 * nrftester.c
 *
 * Created: 18/03/2021 21:13:47
 * Author : Jochem Leijenhorst
 TODO: Verander dit naar van NL naar EN! 
 Dit programma heeft variabele pipes. Deze voer je in op tera term/putty
 Er zijn 4 commandos:
 TODO: Change the commands that they are correct, since that we've implemented the / command method. 
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
    isoInitNrf();
    
    PMIC.CTRL |= PMIC_LOLVLEN_bm;
    sei();
    clear_screen();
    
    // Send welcome message
    printf("Welkom bij de nrftester\nGemaakt door Jochem Leijenhorst.\n\nTyp /help voor een lijst met commando's.\n");
    isoInitId();

    for(uint8_t i = 0; i < 64; i++) uartF0_putc('-');
    printf("\n\n");


    while (1) {
        
        // When something was received:
        uint8_t receivePipe = 0;
        char *receivedPacket = isoGetReceivedChat(&receivePipe);
        
        //TODO: Make a function containing everything in this if statement. Function name printPacket? 
        if(receivedPacket[0] != 0) {

            // Handle the user typing while something has been received.
            uint8_t inputLength = getUserInputLength();

            if(inputLength > 0) {
                printf("\rReceived from pipe %d: %s", receivePipe, receivedPacket);

                // Calculate how many characters of the user written command are left after printing the received packet.
                int16_t trailingCharacters = inputLength - strlen(receivedPacket) - 10;

                // Print that many spaces
                for(int16_t i = 0; i < trailingCharacters; i++)
                    printf(" ");

                // Print the user inputted buffer.
                printf("\n%s", getCurrentInputBuffer());
            }
            else printf("Received from pipe %d: %s\n", receivePipe, receivedPacket);

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

