#define F_CPU 2000000UL

#include <avr/io.h>
#include <util/delay.h>

int main(void) {

    PORTC.DIRSET = PIN0_bm;

    while (1) {
        PORTC.OUTTGL = PIN0_bm;
        _delay_ms(500);
    }
    
    return 0;
}
