// replyrgb
//
// Author : Edwin Boer
// Version: 20200321

#define F_CPU 2000000UL
#include "replyrgb.hpp"
#include <util/delay.h>

int main(void) {

    uint16_t nData;
    uint16_t nCount;
    bool bSendOnly = false; // Debug option to activate sending a counter only

    // Init the RGB LED and check it for 1 sec with color white
    RgbLed_Init();
    RgbLed_Ron();
    RgbLed_Gon();
    RgbLed_Bon();
    _delay_ms(1000);
    RgbLed_Roff();
    RgbLed_Goff();
    RgbLed_Boff();

    // Init the PORT F0 for UART (options: BAUD_38K4 or BAUD_57K6 or BAUD_115K2)
    init_stream(F_CPU, BAUD_115K2);
    sei();
    clear_screen();

    // Stay busy
    nCount = 65535;

    while(uartF0_getc() != 'c');
    RgbLed_Ron();
    while (1) {
        for(uint8_t i = 'A'; i < 'Z'; i++) {
            uartF0_putc(i);
        }
        _delay_us(3);
    
    };

    return 0;
};

