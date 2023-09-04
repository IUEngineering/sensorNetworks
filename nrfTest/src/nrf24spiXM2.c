/*!
 *  \file    nrf24spiXM2.c
 *  \author  Wim Dolman (<a href="mailto:w.e.dolman@hva.nl">w.e.dolman@hva.nl</a>)
 *  \date    18-02-2016
 *  \version 1.0
 *
 *  \brief   Spi driver for Nordic NRF24L01p on HvA Xmegaboard-version2
 *
 *  \details This file contains the routines for interfacing a Nordic NRF24L01p
 *           on HvA Xmegaboard-version2 from july 2015.
 *
 *           This driver uses UARTC0 as SPI-interface. The baudrate is the maximum
 *           rate. This is 8 MHz with Fcpu 32MHz and doesn't exceed the maximum
 *           speed 10 MHz from the NRF24L01p.
 *
 *           The interface exists of six signals:
 *           -   Nordic NRF24L01p           | Xmegaboard-version2
 *           -   IRQ   - interrupt request  | PF6
 *           -   CE    - chip enable        | PF7
 *           -   CSN   - SPI slave select   | PF5
 *           -   SCK   - SPI clock          | PC1
 *           -   MOSI  - SPI MOSI           | PC3
 *           -   MISO  - SPI MOSI           | PC2
 *
 */
#include "nrf24spiXM2.h"

/*! \brief   Initialization of SPI
 *
 *  \details This routines has no parameters. It Initializes UARTC0 as SPI
 *           and the signals IRQ and CE
 *
 *  \return void
 */

void nrfspiInit(void)
{
  PORTC.DIRSET = PIN3_bm;  // MOSI
  PORTC.DIRCLR = PIN2_bm;  // MISO
  PORTC.DIRSET = PIN1_bm;  // SCK
  PORTF.DIRSET = PIN5_bm;  // CSN
  PORTF.DIRCLR = PIN6_bm;  // IRQ
  PORTF.DIRSET = PIN7_bm;  // CE

  USARTC0.CTRLB = USART_TXEN_bm | USART_RXEN_bm;
  USARTC0.CTRLC = USART_CMODE_MSPI_gc;

  USARTC0.BAUDCTRLB = 0;
  USARTC0.BAUDCTRLA = 1;   // F_CPU/(2*(BSEL+1))  is 8MHz on 32MHz CPU
}

/*! \brief SPI transfer
 *
 *  \param   iData    data byte send to the slave
 *
 *  \details This function send a byte IData to register to the slave,
 *           while data from the slave is received into the DATA register.
 *
 *           In this case, for the nrf24L01p, the value of status-register
 *           is shifted out
 *
 *  \return  Data received from slave (status of the nrf24L01p)
 */
uint8_t nrfspiTransfer(uint8_t iData)
{
  USARTC0.DATA = iData;
  while( !(USARTC0.STATUS & USART_TXCIF_bm) );
  USARTC0.STATUS |= USART_TXCIF_bm;

  return USARTC0.DATA;
}


