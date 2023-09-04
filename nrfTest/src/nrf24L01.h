/*!
 *  \file    nrf24L01.h
 *  \author  Wim Dolman (<a href="mailto:w.e.dolman@hva.nl">w.e.dolman@hva.nl</a>)
 *  \date    19-02-2016
 *  \version 1.0
 *
 *  \brief   Header file for driver for Nordic NRF24L01p with Xmega
 *
 *  \details This file contains he definitions and prototypes for interfacing
 *           interfacing a Nordic NRF24L01+ and a Xmega.
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
 *               <a href="http://www.nordicsemi.com/eng/nordic/download_resource/8765/2/16812260">
 *               nRF24L01p_Product_Specification_v1_0.pdf</a>
 *
 */
#ifndef __nrf24L01_H_
#define __nrf24L01_H_

/*!
 *  \brief Definitions of commands, see table 20 datasheet
 */
#define NRF_R_REGISTER               0x00
#define NRF_W_REGISTER               0x20
#define NRF_R_RX_PAYLOAD             0x61
#define NRF_W_TX_PAYLOAD             0xA0
#define NRF_FLUSH_TX                 0xE1
#define NRF_FLUSH_RX                 0xE2
#define NRF_REUSE_TX_PL              0xE3
#define NRF_ACTIVATE                 0x50
#define NRF_R_RX_PL_WID              0x60
#define NRF_W_ACK_PAYLOAD            0xA8
#define NRF_W_TX_PAYLOAD_NO_ACK      0xB0
#define NRF_NOP                      0xFF

#define NRF_REGISTER_gm              0x1F
#define NRF_PIPE_gm                  0x07

/*!
 *  \brief Definitions of registers, see table 28 datasheet
 */
#define REG_CONFIG          0x00  //!< Config
#define REG_EN_AA           0x01  //!< Enable Auto Acknowledgment
#define REG_EN_RXADDR       0x02  //!< Enabled RX addresses
#define REG_SETUP_AW        0x03  //!< Setup address width
#define REG_SETUP_RETR      0x04  //!< Setup Auto. Retrans
#define REG_RF_CH           0x05  //!< RF channel
#define REG_RF_SETUP        0x06  //!< RF setup
#define REG_STATUS          0x07  //!< Status
#define REG_OBSERVE_TX      0x08  //!< Observe TX
#define REG_CD              0x09  //!< Carrier Detect            for nRF24L01
#define REG_RPD             0x09  //!< Received Power Detector   for nRF24L01p
#define REG_RX_ADDR_P0      0x0A  //!< RX address pipe0
#define REG_RX_ADDR_P1      0x0B  //!< RX address pipe1
#define REG_RX_ADDR_P2      0x0C  //!< RX address pipe2
#define REG_RX_ADDR_P3      0x0D  //!< RX address pipe3
#define REG_RX_ADDR_P4      0x0E  //!< RX address pipe4
#define REG_RX_ADDR_P5      0x0F  //!< RX address pipe5
#define REG_TX_ADDR         0x10  //!< TX address
#define REG_RX_PW_P0        0x11  //!< RX payload width pipe0
#define REG_RX_PW_P1        0x12  //!< RX payload width pipe1
#define REG_RX_PW_P2        0x13  //!< RX payload width pipe2
#define REG_RX_PW_P3        0x14  //!< RX payload width pipe3
#define REG_RX_PW_P4        0x15  //!< RX payload width pipe4
#define REG_RX_PW_P5        0x16  //!< RX payload width pipe5
#define REG_FIFO_STATUS     0x17  //!< FIFO Status Register
#define REG_DYNPD           0x1C  //!< Dynamic payload length
#define REG_FEATURE         0x1D  //!< Feature register

// Definitions and typedefinitions for CONFIG register
#define NRF_CONFIG_MASK_RX_DR_bm  (1<<6)
#define NRF_CONFIG_MASK_TX_DS_bm  (1<<5)
#define NRF_CONFIG_MASK_MAX_RT_bm (1<<4)
#define NRF_CONFIG_EN_CRC_bm      (1<<3)
#define NRF_CONFIG_CRC0_bm        (1<<2)
#define NRF_CONFIG_PWR_UP_bm      (1<<1)
#define NRF_CONFIG_PRIM_RX_bm     (1<<0)

typedef enum NRF_CONFIG_CRC_enum
{
    NRF_CONFIG_CRC_DISABLED_gc = ! NRF_CONFIG_EN_CRC_bm,
    NRF_CONFIG_CRC_8_gc        =   NRF_CONFIG_EN_CRC_bm | ! NRF_CONFIG_CRC0_bm,
    NRF_CONFIG_CRC_16_gc       =   NRF_CONFIG_EN_CRC_bm |   NRF_CONFIG_CRC0_bm
} nrf_config_crc_t;
#define NRF_CONFIG_CRC_gm         ( NRF_CONFIG_EN_CRC_bm | NRF_CONFIG_CRC0_bm )

// Definitions for EN_AA register
#define NRF_EN_AA_P5_bm           (1<<5)
#define NRF_EN_AA_P4_bm           (1<<4)
#define NRF_EN_AA_P3_bm           (1<<3)
#define NRF_EN_AA_P2_bm           (1<<2)
#define NRF_EN_AA_P1_bm           (1<<1)
#define NRF_EN_AA_P0_bm           (1<<0)
#define NRF_EN_AA_P_ALL_gm        (0x3F)

// Definitions for EN_RXADDR register
#define NRF_EN_RXADDR_P5_bm       (1<<5)
#define NRF_EN_RXADDR_P4_bm       (1<<4)
#define NRF_EN_RXADDR_P3_bm       (1<<3)
#define NRF_EN_RXADDR_P2_bm       (1<<2)
#define NRF_EN_RXADDR_P1_bm       (1<<1)
#define NRF_EN_RXADDR_P0_bm       (1<<0)

// Definitions and typedefinitions for SETUP register
typedef enum NRF_SETUP_AW_enum
{
    NRF_SETUP_AW_3BYTES_gc = (0x01<<0),
    NRF_SETUP_AW_4BYTES_gc = (0x02<<0),
    NRF_SETUP_AW_5BYTES_gc = (0x03<<0)
} NRF_SETUP_AW_t;

// Definitions and typedefinitions for SETUP_RETR register
typedef enum NRF_SETUP_AW_ARD_enum
{
    NRF_SETUP_ARD_250US_gc  = (0x00<<4),  /*  250 us */
    NRF_SETUP_ARD_500US_gc  = (0x01<<4),  /*  500 us */
    NRF_SETUP_ARD_7500US_gc = (0x02<<4),  /*  750 us */
    NRF_SETUP_ARD_1000US_gc = (0x03<<4),  /* 1000 us */
    NRF_SETUP_ARD_1250US_gc = (0x04<<4),  /* 1250 us */
    NRF_SETUP_ARD_1500US_gc = (0x05<<4),  /* 1500 us */
    NRF_SETUP_ARD_1750US_gc = (0x06<<4),  /* 1750 us */
    NRF_SETUP_ARD_2000US_gc = (0x07<<4),  /* 2000 us */
    NRF_SETUP_ARD_2250US_gc = (0x08<<4),  /* 2250 us */
    NRF_SETUP_ARD_2500US_gc = (0x09<<4),  /* 2500 us */
    NRF_SETUP_ARD_2750US_gc = (0x0A<<4),  /* 2750 us */
    NRF_SETUP_ARD_3000US_gc = (0x0B<<4),  /* 3000 us */
    NRF_SETUP_ARD_3250US_gc = (0x0C<<4),  /* 3250 us */
    NRF_SETUP_ARD_3500US_gc = (0x0D<<4),  /* 3500 us */
    NRF_SETUP_ARD_3750US_gc = (0x0E<<4),  /* 3750 us */
    NRF_SETUP_ARD_4000US_gc = (0x0F<<4)   /* 4000 us */
} NRF_SETUP_AW_ARD_t;
#define NRF_SETUP_ARD_gm           (0xF0)
#define NRF_SETUP_ARD_gp           4

typedef enum NRF_SETUP_AW_ARC_enum
{
    NRF_SETUP_ARC_NORETRANSMIT_gc = (0x00<<0),  /* no retransmit  */
    NRF_SETUP_ARC_1RETRANSMIT_gc  = (0x01<<0),  /*  1 retransmit  */
    NRF_SETUP_ARC_2RETRANSMIT_gc  = (0x02<<0),  /*  2 retransmits */
    NRF_SETUP_ARC_3RETRANSMIT_gc  = (0x03<<0),  /*  3 retransmits */
    NRF_SETUP_ARC_4RETRANSMIT_gc  = (0x04<<0),  /*  4 retransmits */
    NRF_SETUP_ARC_5RETRANSMIT_gc  = (0x05<<0),  /*  5 retransmits */
    NRF_SETUP_ARC_6RETRANSMIT_gc  = (0x06<<0),  /*  6 retransmits */
    NRF_SETUP_ARC_7RETRANSMIT_gc  = (0x07<<0),  /*  7 retransmits */
    NRF_SETUP_ARC_8RETRANSMIT_gc  = (0x08<<0),  /*  8 retransmits */
    NRF_SETUP_ARC_9RETRANSMIT_gc  = (0x09<<0),  /*  9 retransmits */
    NRF_SETUP_ARC_10RETRANSMIT_gc = (0x0A<<0),  /* 10 retransmits */
    NRF_SETUP_ARC_11RETRANSMIT_gc = (0x0B<<0),  /* 11 retransmits */
    NRF_SETUP_ARC_12RETRANSMIT_gc = (0x0C<<0),  /* 12 retransmits */
    NRF_SETUP_ARC_13RETRANSMIT_gc = (0x0D<<0),  /* 13 retransmits */
    NRF_SETUP_ARC_14RETRANSMIT_gc = (0x0E<<0),  /* 14 retransmits */
    NRF_SETUP_ARC_15RETRANSMIT_gc = (0x0F<<0)   /* 15 retransmits */
} NRF_SETUP_AW_ARC_t;
#define NRF_SETUP_ARC_gm           (0x0F)
#define NRF_SETUP_ARC_gp           0

// Definitions for RF_CH register
#define NRF_RF_CH_gm                0x7F
#define NRF_RF_CH_max               0x7F

// Definitions and typedefinitions for RF_SETUP register
#define NRF_RF_SETUP_CONT_WAVE_bm   (1<<7)
#define NRF_RF_SETUP_RF_DR_LOW_bm   (1<<5)
#define NRF_RF_SETUP_PLL_LOCK_bm    (1<<4)
#define NRF_RF_SETUP_RF_DR_HIGH_bm  (1<<3)
typedef enum NRF_RF_SETUP_RF_DR_enum
{
    NRF_RF_SETUP_RF_DR_1M_gc   = ((0<<5)|(0<<3)),
    NRF_RF_SETUP_RF_DR_2M_gc   = ((0<<5)|(1<<3)),
    NRF_RF_SETUP_RF_DR_250K_gc = ((1<<5)|(0<<3))
} nrf_rf_setup_rf_dr_t;
#define NRF_RF_SETUP_RF_DR_gm    ((1<<5)|(1<<3))
typedef enum NRF_RF_SETUP_PWR_enum
{
    NRF_RF_SETUP_PWR_18DBM_gc = (0x00<<1),
    NRF_RF_SETUP_PWR_12DBM_gc = (0x01<<1),
    NRF_RF_SETUP_PWR_6DBM_gc  = (0x02<<1),
    NRF_RF_SETUP_PWR_0DBM_gc  = (0x03<<1)
} nrf_rf_setup_pwr_t;
#define NRF_RF_SETUP_PWR_gm      (0x06)

// Definitions and typedefinitions for STATUS register
#define NRF_STATUS_RX_DR_bm      (1<<6)
#define NRF_STATUS_TX_DS_bm      (1<<5)
#define NRF_STATUS_MAX_RT_bm     (1<<4)
typedef enum NRF_STATUS_P_NO_enum
{
    NRF_STATUS_RX_P_NO_0_gc             = (0x00<<1),
    NRF_STATUS_RX_P_NO_1_gc             = (0x01<<1),
    NRF_STATUS_RX_P_NO_2_gc             = (0x02<<1),
    NRF_STATUS_RX_P_NO_3_gc             = (0x03<<1),
    NRF_STATUS_RX_P_NO_4_gc             = (0x04<<1),
    NRF_STATUS_RX_P_NO_5_gc             = (0x05<<1),
    NRF_STATUS_RX_P_NO_RX_FIFO_EMPTY_gc = (0x07<<1),
} NRF_STATUS_P_NO_t;
#define NRF_STATUS_RX_P_NO_gp        1
#define NRF_STATUS_RX_P_NO_gm        (0x0E)
#define NRF_STATUS_TX_FULL_bm        (1<<0)

// Definitions for OBSERVE_TX register
#define NRF_OBSERVE_TX_PLOS_CNT_gm   0xF0
#define NRF_OBSERVE_TX_ARC_CNT_gm    0x0F

// Definition for RPD-register (nRF24L01+) and CD-register (nRF24L01)
#define NRF_RPD_RPD_bm               (1<<0)
#define NRF_CD_CD_bm                 (1<<0)

// Definitions for RX_PW registers
#define NRF_RX_PW_P0_gm              (0x3F)
#define NRF_RX_PW_P1_gm              (0x3F)
#define NRF_RX_PW_P2_gm              (0x3F)
#define NRF_RX_PW_P3_gm              (0x3F)
#define NRF_RX_PW_P4_gm              (0x3F)
#define NRF_RX_PW_P5_gm              (0x3F)

// Definitions for FIFO_STATUS register
#define NRF_FIFO_STATUS_TX_REUSE_bm  (1<<6)
#define NRF_FIFO_STATUS_TX_FULL_bm   (1<<5)
#define NRF_FIFO_STATUS_TX_EMPTY_bm  (1<<4)
#define NRF_FIFO_STATUS_RX_FULL_bm   (1<<1)
#define NRF_FIFO_STATUS_RX_EMPTY_bm  (1<<0)

// Definitions for DYNPD register
#define NRF_DYNPD_DPL_P5_bm          (1<<5)
#define NRF_DYNPD_DPL_P4_bm          (1<<4)
#define NRF_DYNPD_DPL_P3_bm          (1<<3)
#define NRF_DYNPD_DPL_P2_bm          (1<<2)
#define NRF_DYNPD_DPL_P1_bm          (1<<1)
#define NRF_DYNPD_DPL_P0_bm          (1<<0)
#define NRF_DYNPD_DPL_gm             (0x3F)

// Definitions for FEATURE register
#define NRF_FEATURE_EN_DPL_bm        (1<<2)
#define NRF_FEATURE_EN_ACK_PAY_bm    (1<<1)
#define NRF_FEATURE_EN_DYN_ACK_bm    (1<<0)

/*!
 *  \brief Some parameters of the NRF24L01p
 */
#define NRF_MAX_PAYLOAD_SIZE  32
#define NRF_MAX_CHANNEL       127

/*!
 *  \brief Prototypes of functions
 */
uint8_t nrfReadRegisterMulti(uint8_t reg, uint8_t* buf, uint8_t len);
uint8_t nrfReadRegister(uint8_t reg);
uint8_t nrfWriteRegisterMulti(uint8_t reg, const uint8_t* buf, uint8_t len);
uint8_t nrfWriteRegister(uint8_t reg, uint8_t value);
uint8_t nrfWritePayload(const void* buf, uint8_t len, const uint8_t writeType);
uint8_t nrfReadPayload(void* buf, uint8_t len);
uint8_t nrfFlushRx(void);
uint8_t nrfFlushTx(void);
void    nrfToggleFeatures(void);

void    nrfBegin(void);
void    nrfStartListening(void);
void    nrfStopListening(void);
uint8_t nrfGetStatus(void);
void    nrfSetChannel(uint8_t channel);
uint8_t nrfGetChannel(void);
void    nrfSetPayloadSize(uint8_t size);
uint8_t nrfGetPayloadSize(void);
void    nrfPowerDown(void);
void    nrfPowerUp(void);
uint8_t nrfWrite( uint8_t* buf, uint8_t len); // const void* buf ???
uint8_t nrfWaitForAck(void);
uint8_t nrfAvailable(uint8_t* pipe_num);
uint8_t nrfGetDynamicPayloadSize(void);
void    nrfWriteAckPayload(uint8_t pipe, uint8_t* buf, uint8_t len);
void    nrfStartWrite( const void* buf, uint8_t len, uint8_t multicast);
uint8_t nrfRead( void* buf, uint8_t len );
void    nrfWhatHappened(uint8_t *tx_ok, uint8_t *tx_fail, uint8_t *rx_ready);
void    nrfOpen64WritingPipe(uint64_t value);
void    nrfOpenWritingPipe(uint8_t *address);
void    nrfOpen64ReadingPipe(uint8_t child, uint64_t address);
void    nrfOpenReadingPipe(uint8_t child, uint8_t *address);
void    nrfEnableDynamicPayloads(void);
void    nrfEnableAckPayload(void);
uint8_t nrfIsPVariant(void);
void    nrfSetAutoAck(uint8_t enable);
void    nrfSetAutoAckPipe( uint8_t pipe, uint8_t enable );
uint8_t nrfTestCarrier(void);
uint8_t nrfTestRPD(void);
void    nrfSetPALevel(nrf_rf_setup_pwr_t level);
nrf_rf_setup_pwr_t nrfGetPALevel(void);
uint8_t nrfSetDataRate(nrf_rf_setup_rf_dr_t speed);
nrf_rf_setup_rf_dr_t nrfGetDataRate(void);
void    nrfSetCRCLength(nrf_config_crc_t length);
nrf_config_crc_t nrfGetCRCLength(void);
void    nrfDisableCRC( void );
void    nrfSetRetries(uint8_t delay, uint8_t retries);
uint16_t nrfGetMaxTimeout(void);
void    nrfClearInterruptBits(void);
uint8_t nrfVerifySPIConnection(void);

#endif




