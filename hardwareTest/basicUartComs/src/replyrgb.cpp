// replyrgb
//
// Author : Edwin Boer
// Version: 20200321

#include "replyrgb.hpp"

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

  // Stay busy
  nCount = 65535;
  while (true) {

    // Determine the used data
    if (bSendOnly) {
      // Use the counter
      nData = nCount;
    }
    else {
      // Wait for receiving data
      RgbLed_Gon();
      while ((nData = uartF0_getc()) == UART_NO_DATA) {
      };
      //_delay_ms(250);
      RgbLed_Goff();
    };

    // Send the data
    RgbLed_Bon();
    uartF0_putc(nData);
    //_delay_ms(250);
    RgbLed_Boff();

    // Next
    //_delay_ms(250);
    nCount--;
  };

  return 0;
};

