/*!
 *  \file    serialF0.c
 *  \author  Wim Dolman (<a href="mailto:w.e.dolman@hva.nl">w.e.dolman@hva.nl</a>)
 *  \date    16-04-2018
 *  \version 1.8
 *
 *  \brief   Serial interface voor HvA-Xmegaboard
 *
 *  \details This serial interface doesn't use the drivers of Atmel
 *           The interface uses two \e non circulair buffers for sending and 
 *           receiving the data.
 *           It is based om md_serial.c from J.D.Bakker.
 *
 *           It is a serial interface for the HvA-Xmegaboard (Version 2) with a
 *           Xmega256a3u and Xmega32a4u for programming and the serial interface.
 *           You can use the standard printf, putchar, puts, scanf, getchar, ...
 *           functions.
 *
 *           The baud rate is 115200
 */

#include "serialF0.h"

#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <avr/io.h>
#include <avr/interrupt.h>

static uint8_t CanRead_F0(void);
static uint8_t ReadByte_F0(void);
static uint8_t CanWrite_F0(void);
static void    WriteByte_F0(uint8_t data);

/*! \brief  Send a byte to UARTF0
 *
 *  \param  data      byte to send
 *
 *  \return void
 */
inline void uartF0_putc(uint8_t data) 
{ 
  WriteByte_F0(data);
}

/*! \brief  Read a byte from UARTF0
 *
 *  \return Received byte from buffer or
 *          UART_NO_DATA if buffer is empty
 */
inline uint16_t uartF0_getc(void) 
{
  uint8_t data;

  if ( ! CanRead_F0() ) {
    return UART_NO_DATA;
  }

  data = ReadByte_F0();

  return (data & 0x00FF);
}

/*! \brief  Send a string to UARTF0
 *
 *  \param  s      a pointer to the pointer
 *
 *  \return void
 */
void uartF0_puts(char *s)
{
  char c;

  while ( (c = *s++) ) {
     WriteByte_F0(c);
  }
}

/*  \brief  Send a character to UARTF0 
 *          a static function necessary for standard stream
 *
 *  \param  c        character to send
 *  \param  stream   file stream
 *
 *  \return 0 if succeeded, else 1
 */
static int uartF0_fputc(char c, FILE *stream)
{
  uint8_t timeout = 0xFF;
  while ( ! CanWrite_F0() ) {
    if (timeout == 0) break;
    timeout--;
  }
  if (timeout == 0)  return 1;
  
  if (c == '\n') WriteByte_F0('\r');
  WriteByte_F0(c);

  return 0;
}

/*  \brief  Read character fromUARTF0 
 *          a static function necessary for standard stream
 *
 *  \param  stream   file stream
 *
 *  \return Received character
 */
static int uartF0_fgetc(FILE * stream)
{
  int c;

  c  = ReadByte_F0(); 

  return c;
} 

FILE uartF0_stdinout = FDEV_SETUP_STREAM(uartF0_fputc, uartF0_fgetc, _FDEV_SETUP_RW);   //!< FILE structure for standard streams

#define  BAUD_115K2          115200UL    //!< Baud rate 115200
#define  BAUD_57K6           57600UL     //!< Baud rate 57600
#define  BAUD_38K4           38000UL     //!< Baud rate 38400

#define  UART_DOUBLE_CLK      1          //!< Double clock speed true
#define  UART_NO_DOUBLE_CLK   0          //!< Double clock speed false

/*! \brief   Get a line from the serial input 
 *
 *  \param   buf      pointer to a buffer to store the received line
 *  \param   len      length of the buffer to store the received line
 *
 *  \details The line has to be finished with a End-Of-Line.
 *           This can be \<CR\>, \<CR\>\<LF\> or \<LF\>
 *
 *  \return  Received character
 */
char *getline(char* buf,  uint16_t len)
{
  char     *p = buf;
  uint8_t   c;
  uint16_t  timer = 0;
  uint16_t  i = 0;

  while ( (c = getchar()) != '\n') {
    if (c == '\r') {
      for(timer=2000; timer>0; timer--) {  // wait a short time for the next char
        if ( CanRead_F0() ) break;
      }
      if ( timer == 0 ) break;             // is CR "EOF"
      if ( (c = getchar()) == '\n' ) {     // is CRLF
        break;
      } else {                             // is CR
        ungetc(c, stdin);                  // put last character back
        break;
      }
    }
    if (i++ < len) { 
      *p++ = c;
    }
  }
  *p = '\0';

  return buf;
}

/* \brief   Calculates the baud rate value BSEL
 *          A static function used by init_stream
 *          See also code 19.13 from 'De taal C en de Xmega' 2nd edition
 *
 *  \param  f_cpu       system clock (F_CPU)
 *  \param  baud        desired baud rate
 *  \param  scale       scale factor (BSCALE)
 *  \param  clk2x       clock speed double (1 for double, 0 for no double)
 *
 *  It calculates the baud selection value BSEL from the system clock,
 *  the baud rate, the scale factor and a boolean for clock doubling.
 *
 *  The formula to calculate BSEL is:
 *  \f{eqnarray*}{
 *      \mbox{BSCALE}>=0\quad &:& \quad
 *      \mbox{BSEL} =  \frac{f_{\mbox{cpu}}}{N\ 2^{\mbox{BSCALE}}\ f_{\mbox{baud}}} - 1 \\[3pt]
 *      \mbox{BSCALE}<0\quad  &:& \quad
 *      \mbox{BSEL} =  \frac{1}{2^{\mbox{BSCALE}}}\
 *                     \left( \frac{f_{\mbox{cpu}}}{N\ f_{\mbox{baud}}} - 1 \right)
 *  \f}
 *  N is a factor which is 16 with no clock doubling and 8 with clock doubling
 *
 *  \return the calculated BSEL
 */
static uint16_t calc_bsel(uint32_t f_cpu, uint32_t baud, int8_t scale, uint8_t clk2x)
{
  uint8_t factor = 16;

  factor = factor >> (clk2x & 0x01);
  if ( scale < 0 ) {
    return round(  (((double)(f_cpu)/(factor*(double)(baud))) - 1) * (1<<-(scale))  );
  } else {
    return round(  ((double)(f_cpu)/(factor*(double)(baud))/(1<<(scale))) - 1);
  }
} // calc_bsel

/* \brief   Determines the scale factor BSCALE
 *          A static function used by init_stream
 *          See also code 19.12 from 'De taal C en de Xmega' 2nd edition
 *
 *  \param  f_cpu       system clock (F_CPU)
 *  \param  baud        desired baud rate
 *  \param  clk2x       clock speed double (1 for double, 0 for no double)
 *
 *  It determines the scale factor BSCALE from the system clock, the baud rate,
 *  and a boolean for clock doubling.
 *
 *  \return the scale factor BSCALE
 */
static int8_t calc_bscale(uint32_t f_cpu, uint32_t baud, uint8_t clk2x)
{
  int8_t   bscale;
  uint16_t bsel;

  for (bscale = -7; bscale<8; bscale++) {
    if ( (bsel = calc_bsel(f_cpu, baud, bscale, clk2x)) < 4096 ) return bscale;
  }

  return bscale;
}	// calc_bscale

/*! \brief   Initializes the serial stream for the HvA-Xmegaboard
 *          
 *  \param   f_cpu    clock frequency
 *
 *  \details The only paramter is the clockfrequency. The default baud rate is 115200.
 *           At the moment this match best with the HvA-Xmegaboard.
 *           If you want to use another baud rate you can change it in this function.
 *
 *  \return  void
 */
void init_stream(uint32_t f_cpu)
{
  uint16_t bsel;
  int8_t bscale;

  bscale = calc_bscale(f_cpu, BAUD_115K2, UART_NO_DOUBLE_CLK);
  bsel   = calc_bsel(f_cpu, BAUD_115K2, bscale, UART_NO_DOUBLE_CLK);

	PORTF.PIN2CTRL = PORT_OPC_PULLUP_gc;  // pullup on rx
	PORTF.OUTSET = PIN3_bm;               // tx high
	PORTF.DIRSET = PIN3_bm;
	PORTF.DIRCLR = PIN2_bm;

	USARTF0.BAUDCTRLA = (bsel & USART_BSEL_gm);
	USARTF0.BAUDCTRLB = ((bscale << USART_BSCALE_gp) & USART_BSCALE_gm) |
                      ((bsel >> 8) & ~USART_BSCALE_gm);
	
 	USARTF0.CTRLB = USART_RXEN_bm | USART_TXEN_bm;

	USARTF0.CTRLA = USART_RXCINTLVL_MED_gc | 
                  USART_TXCINTLVL_OFF_gc | USART_DREINTLVL_OFF_gc;
	
	PMIC.CTRL |= PMIC_MEDLVLEN_bm | PMIC_LOLVLEN_bm;
  stdout = stdin = &uartF0_stdinout;
	
} // init_stream


//@cond
// from here borrowed from J.D.Bakker md_serial.c


static volatile uint8_t tx_f0_wridx, tx_f0_rdidx, tx_f0_buf[TXBUF_DEPTH_F0];
static volatile uint8_t rx_f0_wridx, rx_f0_rdidx, rx_f0_buf[RXBUF_DEPTH_F0];

/*  \brief  Static function that tests if there is data in the RX buffer
 *
 *  \return non-zero if there is data else zero
 */
static uint8_t CanRead_F0(void) {
	uint8_t wridx = rx_f0_wridx, rdidx = rx_f0_rdidx;
	
	if(wridx >= rdidx)
		return wridx - rdidx;
	else
		return wridx - rdidx + RXBUF_DEPTH_F0;
	
} // CanRead_F0 


/*  \brief  Static function that reads a byte
 *          This function waits until there is a byte in the buffer.
 *
 *  \return The received byte 
 */
static uint8_t ReadByte_F0(void) {
	uint8_t res, curSlot, nextSlot;
	
	curSlot = rx_f0_rdidx;
	// Busy-wait for a byte to be available. Should not be necessary if the caller calls CanRead_xxx() first 
	while(!CanRead_F0()) ;
	
	res = rx_f0_buf[curSlot];

	nextSlot = curSlot + 1;
	if(nextSlot >= RXBUF_DEPTH_F0)
		nextSlot = 0;
	rx_f0_rdidx = nextSlot;
	
	return res;
} // ReadByte_F0 


/*  \brief  Static function that tests if there is space in the TX buffer
 *
 *  \return non-zero if there is space else zero
 */
static uint8_t CanWrite_F0(void) {
	uint8_t wridx1 = tx_f0_wridx + 1, rdidx = tx_f0_rdidx;
	
	if(wridx1 >= TXBUF_DEPTH_F0)
		wridx1 -= TXBUF_DEPTH_F0;
	if(rdidx >= wridx1)
		return rdidx - wridx1;
	else
		return rdidx - wridx1 + TXBUF_DEPTH_F0;
	
} // CanWrite_F0 


/*  \brief  Static function writes a byte to the TX buffer
 *          This function waits until there is space in the buffer.
 *
 *  \param  data    byte to be written
 *
 *  \return void
 */
static void WriteByte_F0(uint8_t data) {
	uint8_t curSlot, nextSlot, savePMIC;
	
	// Busy-wait for a byte to be available. Should not be necessary if the caller calls CanWrite_xxx() first 
	while(!CanWrite_F0())
		USARTF0.CTRLA = USART_RXCINTLVL_MED_gc | USART_TXCINTLVL_OFF_gc | USART_DREINTLVL_LO_gc;
	
	curSlot = tx_f0_wridx;
	tx_f0_buf[curSlot] = data;
	
	nextSlot = curSlot + 1;
	if(nextSlot >= TXBUF_DEPTH_F0)
		nextSlot = 0;

	savePMIC = PMIC.CTRL;
	PMIC.CTRL = savePMIC & ~PMIC_LOLVLEN_bm;
	tx_f0_wridx = nextSlot;
	USARTF0.CTRLA = USART_RXCINTLVL_MED_gc | USART_TXCINTLVL_OFF_gc | USART_DREINTLVL_LO_gc;
	PMIC.CTRL = savePMIC;

} // WriteByte_F0 

/*  \brief  ISR for receiving bytes from UARTF0.
 *          It puts the received byte in the RX buffer
 */
ISR(USARTF0_RXC_vect) {
	
	uint8_t curSlot, nextSlot;
	
	curSlot = rx_f0_wridx;
	rx_f0_buf[curSlot] = USARTF0.DATA;
	
	nextSlot = curSlot + 1;
	if(nextSlot >= RXBUF_DEPTH_F0)
	nextSlot = 0;
	
	if(nextSlot != rx_f0_rdidx)
	rx_f0_wridx = nextSlot;
	
} // ISR(USARTF0_RXC_vect)


/*  \brief  ISR for transmitting bytes to UARTF0.
 *          If there is a byte to send in the TX buffer, it will be send
 */
ISR(USARTF0_DRE_vect) {
	
	uint8_t curSlot, nextSlot, lastSlot;
	
	nextSlot = curSlot = tx_f0_rdidx;
	lastSlot = tx_f0_wridx;
	
	if(curSlot != lastSlot) {
		USARTF0.DATA = tx_f0_buf[curSlot];
		nextSlot = curSlot + 1;
		if(nextSlot >= TXBUF_DEPTH_F0)
		nextSlot = 0;
	}
	if(nextSlot == lastSlot)
	USARTF0.CTRLA = USART_RXCINTLVL_MED_gc | USART_TXCINTLVL_OFF_gc | USART_DREINTLVL_OFF_gc;
	
	tx_f0_rdidx = nextSlot;
	
} // ISR(USARTF0_DRE_vect) 
//@endcond
