/*!
 *  \file    nrf24L01.c
 *  \author  Wim Dolman (<a href="mailto:w.e.dolman@hva.nl">w.e.dolman@hva.nl</a>)
 *  \date    19-02-2016
 *  \version 1.0
 *
 *  \brief   Driver for Nordic NRF24L01p with Xmega
 *
 *  \details This file contains the routines for interfacing a Nordic NRF24L01p
 *           and a Xmega.
 *           The accompanying files nrf24spiXM2.c and nrf24spiXM2.h comtains the
 *           driverroutines for the HvA Xmegaboard version 2.
 *
 *           This driver is based on:
 *             - the <a href="https://github.com/maniacbug/RF24">the C++ driver</a> from J.Coliz (a.k.a maniacbug)
 *             - <a href="https://tmrh20.github.io/RF24/index.html">
 *               Optimized High Speed NRF24l01+ Driver Class Documentation v1.1.4</a>
 *             - the <a href="https://github.com/TKJElectronics/QuadCopter_STM32F4/blob/master/Libraries/nRF24/">
 *                   C driver for the STM32</a> of TKJElectronics
 *             - the C-driver for the HvA Xmegaboard version 2 of Wouter Zylstra (Internal HvA).
 *
 *           Other references:
 *             - the library and documentation by S. Brennen Bal, see
 *               <a href="http://blog.diyembedded.com/">DIYEmbedded</a>
 *             - the <a href="http://www.tinkerer.eu/AVRLib/nRF24L01">AVR-Lib/nRF24L01</a>
 *               of Tinkerer's Playground
 *             - Nordic Semiconductor, nRF24L01p Product Specification 1.0.
 *              <a href="http://www.nordicsemi.com/eng/nordic/download_resource/8765/2/16812260">
 *               nRF24L01p_Product_Specification_v1_0.pdf</a>
 *
 *
 */
#include "nrf24spiXM2.h"
#include "nrf24L01.h"
#include <string.h>
#include <stdio.h>

/*!
 *  \brief Global variables and constants
 */
uint8_t  p_variant = 1;                             //!< 1 for NRF24L01p 0 for NRF24L01
uint8_t  fixed_payload_size = NRF_MAX_PAYLOAD_SIZE; //!< Size of a fixed payload
uint8_t  dynamic_payloads_enabled = 0;              //!< Whether dynamic payloads are enabled
uint8_t  pipe0_reading_address[5] = {0,0,0,0,0};    //!< Last address set on pipe 0 for reading.
uint8_t  addr_width = 5;                            //!< The address width to use - 3,4 or 5 bytes.

static const uint8_t child_pipe[] =
{
  REG_RX_ADDR_P0, REG_RX_ADDR_P1, REG_RX_ADDR_P2, REG_RX_ADDR_P3, REG_RX_ADDR_P4, REG_RX_ADDR_P5
};

static const uint8_t child_payload_size[] =
{
  REG_RX_PW_P0,  REG_RX_PW_P1,  REG_RX_PW_P2,  REG_RX_PW_P3,  REG_RX_PW_P4,  REG_RX_PW_P5
};

#define ERX_P5      5
#define ERX_P4      4
#define ERX_P3      3
#define ERX_P2      2
#define ERX_P1      1
#define ERX_P0      0

static const uint8_t child_pipe_enable[] =
{
  ERX_P0, ERX_P1, ERX_P2, ERX_P3, ERX_P4, ERX_P5
};


/*! \brief   Begin operation of NRF24L01p
 *
 *  \details This function is used at the inilialzation of NRF24L01p.
 *
 *  \return  void
 */
void nrfBegin(void)
{
  // Must allow the radio time to settle else configuration bits will not necessarily stick.
  // This is actually only required following power up but some settling time also appears to
  // be required after resets too. For full coverage, we'll always assume the worst.
  // Enabling 16b CRC is by far the most obvious case if the wrong timing is used - or skipped.
  // Technically we require 4.5ms + 14us as a worst case. We'll just call it 5ms for good measure.
  // WARNING: Delay is based on P-variant whereby non-P *may* require different timing.
  // NRFDelayMS(5);
  _delay_ms(5);

  // Set 1500uS (minimum for 32B payload in ESB@250KBPS) timeouts, to make testing a little easier
  // WARNING: If this is ever lowered, either 250KBS mode with AA is broken or maximum packet
  // sizes must never be used. See documentation for a more complete explanation.
  //  nrfWriteRegister(REG_SETUP_RETR, NRF_SETUP_ARD_1500US_gc | NRF_SETUP_ARC_15RETRANSMIT_gc );
  nrfSetRetries(NRF_SETUP_ARD_1500US_gc, NRF_SETUP_ARC_15RETRANSMIT_gc);

  // Restore our default PA level
  nrfSetPALevel( NRF_RF_SETUP_PWR_0DBM_gc ) ;

  // Determine if this is a p or non-p RF24 module and then
  // reset our data rate back to default value. This works
  // because a non-P variant won't allow the data rate to
  // be set to 250Kbps.
  if( nrfSetDataRate( NRF_RF_SETUP_RF_DR_250K_gc ) )
  {
    p_variant = 1 ;
  }

  // Then set the data rate to the slowest (and most reliable) speed supported by all
  // hardware.
  nrfSetDataRate( NRF_RF_SETUP_RF_DR_1M_gc );

  // Initialize CRC and request 2-byte (16bit) CRC
  nrfSetCRCLength( NRF_CONFIG_CRC_16_gc ) ;

  // Disable dynamic payloads, to match dynamic_payloads_enabled setting
  nrfWriteRegister(REG_DYNPD, 0);

  // Set up default configuration.  Callers can always change it later.
  // This channel should be universally safe and not bleed over into adjacent
  // spectrum.
  nrfSetChannel(76);

  // Reset current status
  // Notice reset and flush is the last thing we do
  nrfClearInterruptBits();
  nrfFlushRx();
  nrfFlushTx();
}


/*! \brief   Read multiple bytes from a register
 *
 *  \param   reg   Register address, see also tabel 28 of datasheet
 *  \param   buf   Pointer to buffer to put data in
 *  \param   len   Number of bytes to be received
 *
 *  \return  Current value of status register
 */
uint8_t nrfReadRegisterMulti(uint8_t reg, uint8_t* buf, uint8_t len)
{
  uint8_t status;

  nrfCSn(NRF_SELECT);

  status = nrfspiTransfer( NRF_R_REGISTER | ( NRF_REGISTER_gm & reg ) );
  while ( len-- ){
    *buf++ = nrfspiTransfer(NRF_NOP);
  }

  nrfCSn(NRF_DESELECT);

  return status;
}

/*! \brief   Read a byte from a register
 *
 *  \param   reg   Register address, see also tabel 28 of datasheet
 *
 *  \return  Current value of register \p reg
 */
uint8_t nrfReadRegister(uint8_t reg)
{
  uint8_t result;

  nrfCSn(NRF_SELECT);

  nrfspiTransfer( NRF_R_REGISTER | ( NRF_REGISTER_gm & reg ) );
  result = nrfspiTransfer(NRF_NOP);

  nrfCSn(NRF_DESELECT);

  return result;
}

/*! \brief   Write multiple bytes to a register
 *
 *  \param   reg   Register address, see also tabel 28 of datasheet
 *  \param   buf   Pointer to buffer with data to be sent
 *  \param   len   Number of bytes to be sent
 *
 *  \details This function is used at the inilialzation of NRF24L01p.
 *
 *  \return  Current value of status register
 */
uint8_t nrfWriteRegisterMulti(uint8_t reg, const uint8_t* buf, uint8_t len)
{
  uint8_t status;

  nrfCSn(NRF_SELECT);

  status = nrfspiTransfer( NRF_W_REGISTER | ( NRF_REGISTER_gm & reg ) );
  while ( len-- ) {
    nrfspiTransfer( *buf++ );
  }

  nrfCSn(NRF_DESELECT);

  return status;
}

/*! \brief   Write a byte to a register
 *
 *  \param   reg    Register address, see also tabel 28 of datasheet
 *  \param   value  The new value of the register
 *
 *  \return  Current value of status register
 */
uint8_t nrfWriteRegister(uint8_t reg, uint8_t value)
{
  uint8_t status;

  nrfCSn(NRF_SELECT);

  status = nrfspiTransfer( NRF_W_REGISTER | ( NRF_REGISTER_gm & reg ) );
  nrfspiTransfer(value);

  nrfCSn(NRF_DESELECT);

  return status;
}

/*!
 * \brief   Write the transmit payload
 *
 * \details The size of data written has a fixed payload size or a dynamic
 *          payload size. It uses the command W_TX_PAYLOAD (fixed size) or
 *          W_TX_PAYLOAD_NO_ACK (dynamic size).
 *
 * \param   buf       Buffer to get the data from
 * \param   len       Number of bytes to be written
 * \param   writeType W_TX_PAYLOAD: fixed data size
 *                    W_TX_PAYLOAD_NO_ACK: variable data size (multicast)
 *
 * \return  Current value of status register
 */
uint8_t nrfWritePayload(const void* buf, uint8_t len, const uint8_t writeType)
{
  uint8_t status;
  const uint8_t* current = (const uint8_t*) buf;

  if ( len > fixed_payload_size )  len = fixed_payload_size;
  uint8_t blank_len = dynamic_payloads_enabled ? 0 : fixed_payload_size -  len;

  nrfCSn(NRF_SELECT);

  status = nrfspiTransfer( writeType );
  while ( len-- ) {
    nrfspiTransfer( *current++ );
  }
  while ( blank_len-- ) {
    nrfspiTransfer(0);
  }

  nrfCSn(NRF_DESELECT);

  return status;
}


/*!
 * \brief   Write an ack payload for the specified pipe
 *
 * \details The next time a message is received on \p pipe, the data
 *          in \p buf will be sent back in the acknowledgement.
 *
 * \warning Do note, multicast payloads will not trigger ack payloads.
 *
 * \warning According to the data sheet, only three of these can be
 *          pending at any time.
 *
 * \param   pipe      Pipe number
 * \param   buf       Buffer to get the data from
 * \param   len       Number of bytes to be written
 *
 * \return  Current value of status register
 */
void nrfWriteAckPayload(uint8_t pipe, uint8_t* buf, uint8_t len)
{
  uint8_t data_len;

  nrfCSn(NRF_SELECT);

  nrfspiTransfer( NRF_W_ACK_PAYLOAD | ( pipe & NRF_PIPE_gm ) );
  if (len <= fixed_payload_size)
    data_len = len;
  else
    data_len = fixed_payload_size;
  while ( data_len-- )
    nrfspiTransfer(*buf++);

  nrfCSn(NRF_DESELECT);
}


/*!
 * \brief   Write the receive payload
 *
 * \details The size of data written is the fixed payload size, see getPayloadSize()
 *
 * \param   buf  Buffer to write the data to
 * \param   len  Maximum number of bytes to receive
 *
 * \return  Current value of status register
 */
uint8_t nrfReadPayload(void* buf, uint8_t len)
{
  uint8_t status;
  uint8_t* current = (uint8_t*) buf;

  if (len > fixed_payload_size) len = fixed_payload_size;
  uint8_t blank_len = dynamic_payloads_enabled ? 0 : fixed_payload_size - len;

  nrfCSn(NRF_SELECT);

  status = nrfspiTransfer(NRF_R_RX_PAYLOAD);
  while ( len-- ) {
    *current++ = nrfspiTransfer(NRF_NOP);
  }
  while ( blank_len-- ) {
    nrfspiTransfer(NRF_NOP);
  }

  nrfCSn(NRF_DESELECT);

  return status;
}


/*!
 * \brief   Read the payload
 *
 * \details Get the last payload received
 *
 * \param   buf       Buffer where the data should be written to
 * \param   len       Maximum number of bytes to read into the buffer
 *
 * \return  1 if all data is read else 0
 */
uint8_t nrfRead( void* buf, uint8_t len )
{
  // Fetch the payload
  nrfReadPayload( buf, len );

  // was this the last of the data available?
  return nrfReadRegister(REG_FIFO_STATUS) & NRF_FIFO_STATUS_RX_EMPTY_bm;
}


/*!
 * \brief   Empty the receive fifo
 *
 * \return  Current value of status register
 */
uint8_t nrfFlushRx(void)
{
  uint8_t status;

  nrfCSn(NRF_SELECT);
  status = nrfspiTransfer(NRF_FLUSH_RX);
  nrfCSn(NRF_DESELECT);

  return status;
}

/*!
 * \brief   Empty the transmit fifo
 *
 * \return  Current value of status register
 */
uint8_t nrfFlushTx(void)
{
  uint8_t status;

  nrfCSn(NRF_SELECT);
  status = nrfspiTransfer(NRF_FLUSH_TX);
  nrfCSn(NRF_DESELECT);

  return status;
}


/*!
 * \brief   Start listening on the pipes opened for reading.
 *
 * \details Be sure to call openReadingPipe() first.
 *          Do not call write() while in this mode, without first calling stopListening().
 *          Call isAvailable() to check for incoming traffic, and read() to get it.
 */
void nrfStartListening(void)
{
  uint8_t config =  nrfReadRegister(REG_CONFIG);

  if ( ! (config & NRF_CONFIG_PWR_UP_bm) ) {
    nrfWriteRegister(REG_CONFIG, config|NRF_CONFIG_PWR_UP_bm|NRF_CONFIG_PRIM_RX_bm);
    _delay_ms(2); // delay Power Down --> Standby mode with external oscillator (worst case)
  } else {
    nrfWriteRegister(REG_CONFIG, config|NRF_CONFIG_PRIM_RX_bm);
  }
  _delay_us(130); // delay Standby --> TX mode

  nrfWriteRegister(REG_STATUS, NRF_STATUS_RX_DR_bm | NRF_STATUS_TX_DS_bm | NRF_STATUS_MAX_RT_bm );

  if (pipe0_reading_address > 0){
    nrfWriteRegisterMulti(REG_RX_ADDR_P0, (uint8_t *)(&pipe0_reading_address), addr_width);
  }

  nrfFlushRx();
  nrfFlushTx();

  nrfCE(NRF_ENABLE);
  _delay_us(130);
}


/*!
 * \brief   Stop listening for incoming messages.
 *
 * \details Do this before calling write().
 */
void nrfStopListening(void)
{
  nrfCE(NRF_DISABLE);
  nrfFlushRx();
  nrfFlushTx();
}


/*!
 * \brief   Write to the open writing pipe
 *
 * \details Be sure to call openWritingPipe() first to set the destination
 *          of where to write to.
 *
 *          only write with an acknowledge is implemented
 *
 * \param   buf  Pointer to the data to be sent
 * \param   len  Number of bytes to be sent
 *
 * \return  32 (true) if the payload was delivered successfully 0 if not
 */
// TODO  implemented write with no acknowledge (multicast??)
uint8_t nrfWrite( uint8_t* buf, uint8_t len)
{
  uint8_t iReturn;

  //nrfStartWrite(buf, len, NRF_W_TX_PAYLOAD_NO_ACK);
  nrfStartWrite(buf, len, NRF_W_TX_PAYLOAD);

  iReturn = nrfWaitForAck();  // Wait until packet ACK is received or timed out

  return(iReturn);            // Returns 32 on ACK received, 0 on time out
}


/*!
 * \brief   Wait for acknowledge
 *
 * \details It waits for interrupt on TX complete, maximum retransmits reached
 *          or timer expired. The time depends on the number of retries and
 *          delay time. This time calculated with  nrfGetMaxTimeout();
 *
 *          only write with an acknowledge is implemented
 *
 * \return  32 (true) if the payload was delivered successfully 0 if not
 */
// from Wouter + nrfGetMaxTimeout()
// TODO?  iAckTimer zou ook een globale waarde kunnen zijn,
//        die bij init bepaald of bij setRetries gezet wordt
//        is nrfFlushRx nodig ??
uint8_t nrfWaitForAck(void)
{
  uint16_t iAckTimer;  // Time-out
  uint8_t  iIRQ = 0;
  uint8_t  iSucces = 0;

  iAckTimer = nrfGetMaxTimeout() / 100;
  while (!iIRQ && iAckTimer) {   // Interrupt on TX complete, Maximum retransmits reached, or timer expired
    iIRQ = nrfReadRegister(REG_STATUS) & (NRF_STATUS_TX_DS_bm|NRF_STATUS_MAX_RT_bm);
    iAckTimer--;
    _delay_us(100);
  }
  iSucces = nrfReadRegister(REG_STATUS) & NRF_STATUS_TX_DS_bm;

  nrfFlushRx();       // ??
  nrfFlushTx();       // Flush TX FIFO because of MAX_RT
  nrfWriteRegister(REG_STATUS, NRF_STATUS_RX_DR_bm|NRF_STATUS_TX_DS_bm|NRF_STATUS_MAX_RT_bm);

  return(iSucces);    // Returns 32 on ACK received, 0 on time out
}


/*!
 * \brief   Write to open writing pipe
 *
 * \details Same as write() but doesn't wait for acknowledge
 *
 * \param   buf         Pointer to the data to be sent
 * \param   len         Number of bytes to be sent
 * \param   multicast   ?? NRF_W_TX_PAYLOAD or NRF_W_TX_PAYLOAD_NO_ACK.
 */
void nrfStartWrite( const void* buf, uint8_t len, uint8_t multicast)
{
  uint8_t config =  nrfReadRegister(REG_CONFIG);

  if ( ! (config & NRF_CONFIG_PWR_UP_bm) ) {
    nrfWriteRegister(REG_CONFIG, (config | NRF_CONFIG_PWR_UP_bm) & ~NRF_CONFIG_PRIM_RX_bm );
    _delay_ms(2);  // delay Power Down --> Standby mode with external oscillator (worst case)
  } else {
    nrfWriteRegister(REG_CONFIG, config & ~NRF_CONFIG_PRIM_RX_bm );
  }
  _delay_us(130);  // delay Standby --> TX mode

  nrfWritePayload( buf, len, multicast );

  nrfCE(NRF_ENABLE);
  _delay_us(10);
  nrfCE(NRF_DISABLE);
}


/*!
 * \brief   Enter low-power mode
 *
 * \details To return to normal power mode, either nrfWrite() some
 *          data or nrfStartListening(), or nrfPowerUp().
 */
void nrfPowerDown(void)
{
  nrfWriteRegister(REG_CONFIG, nrfReadRegister(REG_CONFIG) & ~NRF_CONFIG_PWR_UP_bm );
}


/*!
 * \brief   Leave low-power mode - making radio more responsive
 *
 * \details To return to low power mode, call nrfPowerDown().
 */
void nrfPowerUp(void)
{
  nrfWriteRegister(REG_CONFIG, nrfReadRegister(REG_CONFIG) | NRF_CONFIG_PWR_UP_bm );
}


/*!
 * \brief   Test whether there are bytes available to be read
 *
 * \details Call nrfAvaliable(NULL) if you're not interested in the pipe number
 *          Call nrfAvaliable(&pipe) puts the pipenumber in pipe
 *
 * \param[out] pipe_num   Pointer to pipenumber, where it writes
 *                        the pipenumber with a payload that is available
 *
 * \return  64 (true) if there is a payload available, 0 (false) if none is
 */
uint8_t nrfAvailable(uint8_t* pipe_num)
{
  uint8_t status = nrfGetStatus();
  uint8_t result = status & NRF_STATUS_RX_DR_bm;

  if (result)
  {
    if ( pipe_num ) {
      *pipe_num = ( status & NRF_STATUS_RX_P_NO_gm ) >> NRF_STATUS_RX_P_NO_gp;
    }

    nrfWriteRegister(REG_STATUS, NRF_STATUS_RX_DR_bm);      // clear status RX_DR bit

    // Handle ack payload receipt
    if ( status & NRF_STATUS_TX_DS_bm )
    {
      nrfWriteRegister(REG_STATUS, NRF_STATUS_TX_DS_bm);     // clear status bit
    }
  }

  return result;
}


/*!
 * \brief   Call this to find out which interrupt occured
 *
 * \details Tells you what caused the interrupt, and clears the state of
 *          interrupts.
 *          The retuend values are not equal to 0 if the interrupt occured.
 *
 * \param[out]  tx_ok    The send was successful (TX_DS)
 * \param[out]  tx_fail  The send failed, too many retries (MAX_RT)
 * \param[out]  rx_ready There is a message waiting to be read (RX_DR)
 */
void nrfWhatHappened(uint8_t *tx_ok, uint8_t *tx_fail, uint8_t *rx_ready)
{
  // Read the status & reset the status in one easy call
  // Or is that such a good idea?
  uint8_t status = nrfWriteRegister(REG_STATUS, NRF_STATUS_RX_DR_bm | NRF_STATUS_TX_DS_bm | NRF_STATUS_MAX_RT_bm );

  // Report to the user what happened
  *tx_ok    = status & NRF_STATUS_TX_DS_bm;
  *tx_fail  = status & NRF_STATUS_MAX_RT_bm;
  *rx_ready = status & NRF_STATUS_RX_DR_bm;
}

/*!
 * \brief   Open a pipe for writing
 *
 * \details Obsolete, replaced by \see nrfOpenWritingPipe
*/
void nrfOpen64WritingPipe(uint64_t value)
{
  nrfWriteRegisterMulti(REG_RX_ADDR_P0, (uint8_t *) (&value), addr_width);
  nrfWriteRegisterMulti(REG_TX_ADDR,    (uint8_t *) (&value), addr_width);

  if ( fixed_payload_size < NRF_MAX_PAYLOAD_SIZE ) {
    nrfWriteRegister(REG_RX_PW_P0, fixed_payload_size);
  } else {
    nrfWriteRegister(REG_RX_PW_P0, NRF_MAX_PAYLOAD_SIZE);
  }
}


/*!
 * \brief   Open a pipe for reading
 *
 * \details Obsolete, replaced by \see nrfOpenWritingPipe
*/
void nrfOpen64ReadingPipe(uint8_t child, uint64_t address)
{
  // If this is pipe 0, cache the address.  This is needed because
  // openWritingPipe() will overwrite the pipe 0 address, so
  // startListening() will have to restore it.
  if (child == 0) {
    memcpy(pipe0_reading_address, &address, addr_width);;
  }

  if (child <= 6)
  {
    // For pipes 2-5, only write the LSB
    if ( child < 2 )
      nrfWriteRegisterMulti(child_pipe[child], (const uint8_t*)(&address), addr_width);
    else
      nrfWriteRegisterMulti(child_pipe[child], (const uint8_t*)(&address), 1);

    nrfWriteRegister(child_payload_size[child], fixed_payload_size);

    // Note it would be more efficient to set all of the bits for all open
    // pipes at once.  However, I thought it would make the calling code
    // more simple to do it this way.
    nrfWriteRegister(REG_EN_RXADDR, nrfReadRegister(REG_EN_RXADDR) | _BV(child_pipe_enable[child]) );
  }
}


/*!
 * \brief   Open a pipe for writing
 *
 * \details The address is a pointer to byte array with 3-5 bytes.
 *          Only one writing pipe can be open at once, but you can change the
 *          address you'll write to.
 *
 * \param   address  Pointer to address of the pipe to open.
 */
void nrfOpenWritingPipe(uint8_t *address)
{
  nrfWriteRegisterMulti(REG_RX_ADDR_P0, address, addr_width);
  nrfWriteRegisterMulti(REG_TX_ADDR,    address, addr_width);

  if ( fixed_payload_size < NRF_MAX_PAYLOAD_SIZE ) {
    nrfWriteRegister(REG_RX_PW_P0, fixed_payload_size);
  } else {
    nrfWriteRegister(REG_RX_PW_P0, NRF_MAX_PAYLOAD_SIZE);
  }
}


/*!
 * \brief   Open a pipe for reading
 *
 * \details Up to 6 pipes can be open for reading at once.
 *          Open all the required reading pipes, and then call startListening().
 *
 * \param   child    Pipe number (0-5) to read from.
 * \param   address  Pointer to address of the pipe to open.
 */
void nrfOpenReadingPipe(uint8_t child, uint8_t *address)
{
  // If this is pipe 0, cache the address.  This is needed because
  // openWritingPipe() will overwrite the pipe 0 address, so
  // startListening() will have to restore it.
  if (child == 0) {
    memcpy(pipe0_reading_address, address, addr_width);
  }

  if (child <= 6)
  {
    // For pipes 2-5, only write the LSB
    if ( child < 2 )
      nrfWriteRegisterMulti(child_pipe[child], address, addr_width);
    else
      nrfWriteRegisterMulti(child_pipe[child], address, 1);

    nrfWriteRegister(child_payload_size[child], fixed_payload_size);

    // Note it would be more efficient to set all of the bits for all open
    // pipes at once.  However, I thought it would make the calling code
    // more simple to do it this way.
    nrfWriteRegister(REG_EN_RXADDR, nrfReadRegister(REG_EN_RXADDR) | _BV(child_pipe_enable[child]) );
  }
}


/*!
 * \brief   Turn on or off the special features
 *
 * \details The nrf2401(+) has certain 'features' which are
 *          only available when the 'features' are enabled.
 */
void nrfToggleFeatures(void)
{
  nrfCSn(NRF_SELECT);
  nrfspiTransfer(NRF_ACTIVATE);
  nrfspiTransfer(0x73);
  nrfCSn(NRF_DESELECT);
}


/*!
 * \brief   Enable dynamically-sized payloads
 *
 * \details This way you don't always have to send large packets.
 *          It enables dynamic payloads on ALL pipes.
 */
void nrfEnableDynamicPayloads(void)
{
  // Enable dynamic payload throughout the system
  nrfWriteRegister(REG_FEATURE, nrfReadRegister(REG_FEATURE) | NRF_FEATURE_EN_DPL_bm );

  // If it didn't work, the features are not enabled
  if ( ! nrfReadRegister(REG_FEATURE) )
  {
    // So enable them and try again
    nrfToggleFeatures();
    nrfWriteRegister(REG_FEATURE, nrfReadRegister(REG_FEATURE) | NRF_FEATURE_EN_DPL_bm );
  }

  // Enable dynamic payload on all pipes
  //
  // Not sure the use case of only having dynamic payload on certain
  // pipes, so the library does not support it.
  nrfWriteRegister(REG_DYNPD, nrfReadRegister(REG_DYNPD) | NRF_DYNPD_DPL_gm );

  dynamic_payloads_enabled = 1;
}


/*!
 * \brief   Enable custom payloads on the acknowledge packets
 *
 * \details Ack payloads are a handy way to return data back to senders
 *          without manually changing the radio modes on both units.
 */
void nrfEnableAckPayload(void)
{
  //
  // enable ack payload and dynamic payload features
  //

  nrfWriteRegister(REG_FEATURE, nrfReadRegister(REG_FEATURE) | NRF_FEATURE_EN_ACK_PAY_bm | NRF_FEATURE_EN_DYN_ACK_bm );

  // If it didn't work, the features are not enabled
  if ( ! nrfReadRegister(REG_FEATURE)  )
  {
    // So enable them and try again
    nrfToggleFeatures();
    nrfWriteRegister(REG_FEATURE, nrfReadRegister(REG_FEATURE) | NRF_FEATURE_EN_ACK_PAY_bm | NRF_FEATURE_EN_DYN_ACK_bm  );
  }

  //
  // Enable dynamic payload on pipes 0 & 1
  //

  nrfWriteRegister(REG_DYNPD, nrfReadRegister(REG_DYNPD) | NRF_DYNPD_DPL_P1_bm | NRF_DYNPD_DPL_P1_bm );

  dynamic_payloads_enabled = 1;
}

/*!
 * \brief   Get Dynamic Payload Size
 *
 * \details For dynamic payloads, this returns the size of the payload
 *
 * \return  Payload length of last-received dynamic payload
 */
uint8_t nrfGetDynamicPayloadSize(void)
{
  uint8_t result = 0;

  nrfCSn(NRF_SELECT);
  nrfspiTransfer(NRF_R_RX_PL_WID);    // Send Command
  result = nrfspiTransfer(NRF_NOP);   // Read byte
  nrfCSn(NRF_DESELECT);

  return result;
}


/*!
 * \brief   Get fixed payload size
 *
 * \return  Fixed payload size
 */
uint8_t nrfGetPayloadSize(void)
{
  return fixed_payload_size;
}


/*!
 * \brief   Get fixed payload size
 *
 * \return  Fixed payload size
 */
uint8_t nrfGetStatus(void)
{
  uint8_t status;

  nrfCSn(NRF_SELECT);
  status = nrfspiTransfer(NRF_NOP);
  nrfCSn(NRF_DESELECT);

  return status;
}


void nrfSetChannel(uint8_t channel)
{
  if ( channel <= NRF_MAX_CHANNEL ) {
    nrfWriteRegister(REG_RF_CH, channel);
  } else {
    nrfWriteRegister(REG_RF_CH, NRF_MAX_CHANNEL);
  }
}


uint8_t nrfGetChannel(void)
{
  return nrfReadRegister(REG_RF_CH);
}


void nrfSetPayloadSize(uint8_t size)
{
  if ( size < NRF_MAX_PAYLOAD_SIZE ) {
    fixed_payload_size = size;
  } else {
    fixed_payload_size = NRF_MAX_PAYLOAD_SIZE;
  }
}



/*!
 * \brief   Determine whether the hardware is an nRF24L01+ or not.
 *
 * \return  1 (true) if the hardware is nRF24L01+ (or compatible) and
 *          0 (false) if its not.
 */
// it's set in nrfBegin() by reading RF_SETUP if NRF_RF_SETUP_RF_DR_250K_gc
// exists it is a p-variant.
// Ik vraag mij af of dit werkt. Is alleen te testen met nRF24L01
uint8_t nrfIsPVariant(void)
{
  return p_variant ;
}


/*!
 * \brief   Enable or disable auto-acknowlede packets.
 *
 * \details Auto acknowledge is enabled by default, so it's only needed if
 *          you want to turn it off for some reason.
 *
 * \param   enable  Whether to enable (true, non 0) or disable (false, 0).
 */
void nrfSetAutoAck(uint8_t enable)
{
  if ( enable )
    nrfWriteRegister(REG_EN_AA, NRF_EN_AA_P_ALL_gm);
  else
    nrfWriteRegister(REG_EN_AA, 0);
}


/*!
 * \brief   Enable or disable auto-acknowlede packets for a distinct pipe.
 *
 * \details Auto acknowledge is enabled by default, so it's only needed if
 *          you want to turn it off for some reason.
 *
 * \param   pipe    Pipe number which you want to enable or disable
 * \param   enable  Whether to enable (true, non 0) or disable (false, 0).
 */
void nrfSetAutoAckPipe( uint8_t pipe, uint8_t enable )
{
  if ( pipe <= 6 )
  {
    uint8_t en_aa = nrfReadRegister( REG_EN_AA ) ;
    if( enable )  {
      en_aa |= _BV(pipe) ;
    } else {
      en_aa &= ~_BV(pipe) ;
    }
    nrfWriteRegister( REG_EN_AA, en_aa ) ;
  }
}


/*!
 * \brief   Test whether there was a carrier on the line for
 *          the previous listening period.
 *
 * \details Valid only on nRF24L01. On nRF24L01, use nrfTestRPD().
 *
 * \return  1 (true)  if there was a carrier
 *          0 (false) if not.
 */
uint8_t nrfTestCarrier(void)
{
  return ( nrfReadRegister(REG_CD) & NRF_CD_CD_bm );
}


/*!
 * \brief   Test whether a signal (carrier or otherwise) greater than or equal
 *          to -64dBm is present on the channel.
 *
 * \details Valid only on nRF24L01+(p). On nRF24L01, use nrfTestCarrier().
 *          Useful to check for interference on the current channel and
 *          channel hopping strategies.
 *
 * \return  1 (true)  if signal is greater or equal to -64dBm,
 *          0 (false) if not.
 */
uint8_t nrfTestRPD(void)
{
  return ( nrfReadRegister(REG_RPD) & NRF_RPD_RPD_bm ) ;
}


/*!
 * \brief   Set Power Amplifier (PA) level
 *
 * \details The power levels correspond to the following output levels respectively:
 *          - minimum is -18dBm, use \p NRF_RF_SETUP_PWR_18DBM_gc
 *          - low     is -12dBm, use \p NRF_RF_SETUP_PWR_12DBM_gc
 *          - high    is -6dBm,  use \p NRF_RF_SETUP_PWR_6DBM_gc
 *          - maximum is -0dBm,  use \p NRF_RF_SETUP_PWR_0DBM_gc
 *
 * \param   level  Desired level (is one of the group configurations)
 */
void nrfSetPALevel(nrf_rf_setup_pwr_t level)
{
  uint8_t setup = nrfReadRegister(REG_RF_SETUP);
  setup  = (setup & ~NRF_RF_SETUP_PWR_gm) |
           (level &  NRF_RF_SETUP_PWR_gm);

  nrfWriteRegister( REG_RF_SETUP, setup ) ;
}


/*!
 * \brief   Gets the current PA level
 *
 * \details The power levels correspond to the following output levels respectively:
 *          - minimum is -18dBm, use \p NRF_RF_SETUP_PWR_18DBM_gc
 *          - low     is -12dBm, use \p NRF_RF_SETUP_PWR_12DBM_gc
 *          - high    is -6dBm,  use \p NRF_RF_SETUP_PWR_6DBM_gc
 *          - maximum is -0dBm,  use \p NRF_RF_SETUP_PWR_0DBM_gc
 *
 * \return  Level (is one of the group configurations)
 */
nrf_rf_setup_pwr_t nrfGetPALevel(void)
{
  return (nrf_rf_setup_pwr_t) nrfReadRegister(REG_RF_SETUP) & NRF_RF_SETUP_PWR_gm ;
}


/*!
 * \brief   Set the transmission data rate
 *
 * \details The data rates correspond to the following speeds respectively:
 *          - 250kbps, use NRF_RF_SETUP_RF_DR_250K_gc (only for nRF24L01+)
 *          - 1Mpbs, use  NRF_RF_SETUP_RF_DR_1M_gc
 *          - 2Mpbs, use  NRF_RF_SETUP_RF_DR_2M_gc
 *
 * \warning Setting RF24_250KBPS will fail for a non nRF24L01+
 *
 * \param   speed (is one of the group configurations)
 *
 * \return  1 (true) if successfull, 0 (false) if not
 */
uint8_t nrfSetDataRate(nrf_rf_setup_rf_dr_t speed)
{
  uint8_t result = 0;
  uint8_t setup = nrfReadRegister(REG_RF_SETUP) ;

  setup  = (setup & ~NRF_RF_SETUP_RF_DR_gm) |
           (speed &  NRF_RF_SETUP_RF_DR_gm);

  nrfWriteRegister( REG_RF_SETUP, setup ) ;

  if ( nrfReadRegister(REG_RF_SETUP) == speed ) {
    result = 1;
  } else  {
    result = 0;
  }

  return result;
}


/*!
 * \brief   Get the transmission data rate
 *
 * \details The data rates correspond to the following speeds respectively:
 *          - 250kbps, use NRF_RF_SETUP_RF_DR_250K_gc (only for nRF24L01+)
 *          - 1Mpbs, use  NRF_RF_SETUP_RF_DR_1M_gc
 *          - 2Mpbs, use  NRF_RF_SETUP_RF_DR_2M_gc
 *
 * \warning Setting RF24_250KBPS will fail for a non nRF24L01+
 *
 * \return  Speed (is one of the group configurations)
 */
nrf_rf_setup_rf_dr_t nrfGetDataRate(void)
{
  return (nrf_rf_setup_rf_dr_t) nrfReadRegister(REG_RF_SETUP) & NRF_RF_SETUP_RF_DR_gm ;
}


/*!
 * \brief   Sets the CRC length
 *
 * \details The CRC correspond to the following lengths respectively:
 *          - Disable CRC, use NRF_CONFIG_CRC_DISABLED_gc
 *          - CRC 1 byte,  use NRF_CONFIG_CRC_8_gc
 *          - CRC 2 bytes, use NRF_CONFIG_CRC_16_gc
 */
void nrfSetCRCLength(nrf_config_crc_t length)
{
  uint8_t config = nrfReadRegister(REG_CONFIG) ;

  config = (config & ~NRF_CONFIG_CRC_gm) |
           (length &  NRF_CONFIG_CRC_gm);

  nrfWriteRegister( REG_CONFIG, config );
}


/*!
 * \brief   Get the CRC length
 *
 * \details The CRC correspond to the following lengths respectively:
 *          - Disable CRC, use NRF_CONFIG_CRC_DISABLED_gc
 *          - CRC 1 byte,  use NRF_CONFIG_CRC_8_gc
 *          - CRC 2 bytes, use NRF_CONFIG_CRC_16_gc
 *
 * \return  The CRC-configuration (is one of the group configurations)
 */
nrf_config_crc_t nrfGetCRCLength(void)
{
   return (nrf_config_crc_t) nrfReadRegister(REG_CONFIG) & NRF_CONFIG_CRC_gm;
}


/*!
 * \brief   Disbale CRC
 */
void nrfDisableCRC( void )
{
  uint8_t config = nrfReadRegister(REG_CONFIG) & ~NRF_CONFIG_EN_CRC_bm;
  nrfWriteRegister( REG_CONFIG, config );
}


/*!
 * \brief   Sets the number of retries and the delay between the retries
 *
 * \details The number of retries is 0 to 15
 *          The delay between the retries is 250 us to 4000 us with
 *          with a 250 us step size.
 *
 *          The groupsconfiguration for the retries is NRF_SETUP_ARC_#RETRANSMIT_gc
 *          where # is NO or 1 to 15.
 *
 *          The groupsconfiguration for the delays is NRF_SETUP_ARD_#US_gc
 *          where # is 250 to 4000 with a 250 step size.
 *
 * \param   delay   (groupsconfiguration NRF_SETUP_ARD_#US_gc for delay #)
 * \param   retries (groupsconfiguration NRF_SETUP_ARC_#RETRANSMIT_gc for retries #)
 */
void nrfSetRetries(uint8_t delay, uint8_t retries)
{
  nrfWriteRegister(REG_SETUP_RETR, (delay|retries));
}


/*!
 * \brief   Calculate the maximum timeout in us based on current configuration.
 *
 * \details This depends on the number of retries en the delays betweeen them.
 *          The maximum timeout is delay * (retries+1) us.
 *          It is a value between 250 us and 64000 us.
 *
 * @return  maximum timeout in us
 */

uint16_t nrfGetMaxTimeout(void){
  uint8_t retries = nrfReadRegister(REG_SETUP_RETR);
  uint8_t delay   = (retries & NRF_SETUP_ARD_gm) >> NRF_SETUP_ARD_gp;
  uint8_t count   = (retries & NRF_SETUP_ARC_gm) >> NRF_SETUP_ARC_gp;

  uint16_t to = 250 * (delay + 1) * (count + 1);

  return to ;
}

/*!
 * \brief   Clear Interrupt Bits
 *
 */
void nrfClearInterruptBits(void)
{
  nrfWriteRegister(REG_STATUS, NRF_STATUS_RX_DR_bm | NRF_STATUS_TX_DS_bm | NRF_STATUS_MAX_RT_bm );
}

/*!
 * \brief   Verify SPI Interface
 *
 * \details It sends and reads back a random value to the SETUP register
 *          It restores the original setup
 *
 * \return  1 (true) if SPI interface is correct
 *          0 (false) if not
 */

// from Wouter
uint8_t nrfVerifySPIConnection(void)
{
  uint8_t iBuffer = 0;
  uint8_t iDataBuffer = 0;

  iDataBuffer = nrfReadRegister(REG_SETUP_RETR);  // Buffer old value
  nrfWriteRegister(REG_SETUP_RETR, 0x48);         // Write random value
  _delay_ms(1);
  iBuffer = nrfReadRegister(REG_SETUP_RETR);      // Read value from SPI
  nrfWriteRegister(REG_SETUP_RETR, iDataBuffer);  // Restore old value

  if (iBuffer == 0x48) return(1);                  // 1 - Value is as expected
  else                 return(0);                  // 0 - Value is different
}

