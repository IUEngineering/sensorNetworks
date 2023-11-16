// replyrgb
//
// Author : Edwin Boer
// Version: 20200321

#ifndef _REPLYRGB_HPP_
#define _REPLYRGB_HPP_

  #define F_CPU 2000000UL

  // RGB-LED defines
  #define RgbLed_Init() { PORTF.DIRSET = PIN1_bm; PORTF.DIRSET = PIN0_bm; PORTC.DIRSET = PIN0_bm; }
  #define RgbLed_Ron()  { PORTF.OUTSET = PIN1_bm; }
  #define RgbLed_Roff() { PORTF.OUTCLR = PIN1_bm; }
  #define RgbLed_Gon()  { PORTF.OUTSET = PIN0_bm; }
  #define RgbLed_Goff() { PORTF.OUTCLR = PIN0_bm; }
  #define RgbLed_Bon()  { PORTC.OUTSET = PIN0_bm; }
  #define RgbLed_Boff() { PORTC.OUTCLR = PIN0_bm; }

  // System includes
  #include <avr/io.h>
  #include <avr/interrupt.h>
  #include <util/delay.h>
  #include <stdbool.h>

  // Use serial on port F0
  #include "serialF0.h"

#endif // _REPLYRGB_HPP_