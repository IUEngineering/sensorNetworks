/*!
 *  \file    clock.c
 *  \author  Wim Dolman (<a href="mailto:w.e.dolman@hva.nl">w.e.dolman@hva.nl</a>)
 *  \date    10-07-2019
 *  \version 1.4
 *
 *  \brief   Clock functions for Xmega 
 *
 */
#include <avr/io.h>

/*! \brief  Enables clock with 32 MHz internal RC oscillator
 *
 *          This example is code 23.24 from 'De taal C en de Xmega' second edition,
 *          see <a href="http://www.dolman-wim.nl/xmega/book/index.php">Voorbeelden uit 'De taal C en de Xmega'</a>
 *
 *  \return void
 */
void Config32MHzClock(void)
{
  OSC.CTRL |= OSC_RC32MEN_bm;                  // Enable internal 32 MHz oscillator
  while(!(OSC.STATUS & OSC_RC32MRDY_bm));      // Wait for oscillator is ready
  CCP = CCP_IOREG_gc;                          // Security signature to modify clock
  CLK.CTRL = CLK_SCLKSEL_RC32M_gc;             // Select sysclock 32 MHz oscillator
}

/*! \brief  Enables 32 MHz clock with an external 16 MHz crystal
 *
 *          This function is useful with het HvA-Xmegaboard version 2, 
 *          which has a 16 MHz crystal 
 *
 *          This example is code 23.28 from 'De taal C en de Xmega' second edition,
 *          see <a href="http://www.dolman-wim.nl/xmega/book/index.php">Voorbeelden uit 'De taal C en de Xmega'</a>
 *
 * \return void
 */
void Config32MHzClock_Ext16M(void)
{
  OSC.XOSCCTRL = OSC_FRQRANGE_12TO16_gc |                   // Select frequency range
                 OSC_XOSCSEL_XTAL_16KCLK_gc;                // Select start-up time
  OSC.CTRL |= OSC_XOSCEN_bm;                                // Enable oscillator
  while ( ! (OSC.STATUS & OSC_XOSCRDY_bm) );                // Wait for oscillator is ready

  OSC.PLLCTRL = OSC_PLLSRC_XOSC_gc | (OSC_PLLFAC_gm & 2);   // Select PLL source and multipl. factor
  OSC.CTRL |= OSC_PLLEN_bm;                                 // Enable PLL
  while ( ! (OSC.STATUS & OSC_PLLRDY_bm) );                 // Wait for PLL is ready

  CCP = CCP_IOREG_gc;                                       // Security signature to modify clock
  CLK.CTRL = CLK_SCLKSEL_PLL_gc;                            // Select system clock source
  OSC.CTRL &= ~OSC_RC2MEN_bm;                               // Turn off 2MHz internal oscillator
  OSC.CTRL &= ~OSC_RC32MEN_bm;                              // Turn off 32MHz internal oscillator
}

/*! \brief  Enables 16 MHz clock with an external 16 MHz crystal
 *
 *          This function is useful with het HvA-Xmegaboard version 2, 
 *          which has a 16 MHz crystal 
 *
  * \return void
 */
void Config16MHzClock_Ext16M(void)
{
  OSC.XOSCCTRL = OSC_FRQRANGE_12TO16_gc |                   // Select frequency range
                 OSC_XOSCSEL_XTAL_16KCLK_gc;                // Select start-up time
  OSC.CTRL |= OSC_XOSCEN_bm;                                // Enable oscillator
  while ( ! (OSC.STATUS & OSC_XOSCRDY_bm) );                // Wait for oscillator is ready

  OSC.PLLCTRL = OSC_PLLSRC_XOSC_gc | (OSC_PLLFAC_gm & 1);   // Select PLL source and multipl. factor
  OSC.CTRL |= OSC_PLLEN_bm;                                 // Enable PLL
  while ( ! (OSC.STATUS & OSC_PLLRDY_bm) );                 // Wait for PLL is ready

  CCP = CCP_IOREG_gc;                                       // Security signature to modify clock
  CLK.CTRL = CLK_SCLKSEL_PLL_gc;                            // Select system clock source
  OSC.CTRL &= ~OSC_RC2MEN_bm;                               // Turn off 2MHz internal oscillator
  OSC.CTRL &= ~OSC_RC32MEN_bm;                              // Turn off 32MHz internal oscillator
}

/*! \brief  Autocalibrates 32 MHz internal RC oscillator
 *
 *          This example is code 23.25 from 'De taal C en de Xmega' second edition,
 *          see <a href="http://www.dolman-wim.nl/xmega/book/index.php">Voorbeelden uit 'De taal C en de Xmega'</a>
 * 
 * \return void
 */
void AutoCalibration32M(void)
{
  OSC.CTRL       |= OSC_RC32KEN_bm;                      // Enable internal 32 kHz oscillator
  do {} while ( (OSC.STATUS & OSC_RC32KRDY_bm ) == 0 ) ; // Wait for oscillator is ready
  OSC.DFLLCTRL   &= ~(OSC_RC32MCREF_gm);                 // Select 32 kHz calibration source
  OSC.DFLLCTRL   |=   OSC_RC32MCREF_RC32K_gc;            //    for 32 MHz oscillator
  DFLLRC32M.CTRL |= DFLL_ENABLE_bm;                      // Enable DFLL for 32 MHz oscillator
}

/*! \brief  Autocalibrates 2 MHz internal RC oscillator
 *
 *          This example is code 23.26 from 'De taal C en de Xmega' second edition,
 *          see <a href="http://www.dolman-wim.nl/xmega/book/index.php">Voorbeelden uit 'De taal C en de Xmega'</a>
 * 
 *  \return void
 */
void AutoCalibration2M(void)
{
  OSC.CTRL      |= OSC_RC32KEN_bm;              // Enable internal 32 kHz oscillator
  while(!(OSC.STATUS & OSC_RC32KRDY_bm));       // Wait for oscillator is ready
  OSC.DFLLCTRL  &= ~(OSC_RC2MCREF_bm);          // Select 32 kHz calibration source
  OSC.DFLLCTRL  |=   OSC_RC2MCREF_RC32K_gc;     //    for 2 MHz oscillator
  DFLLRC2M.CTRL |= DFLL_ENABLE_bm;              // Enable DFLL for 2 MHz oscillator
}

/*! \brief  Calibrates 32 MHz internal RC oscillator with external 32 kHz crystal.
 *
 *          This function is useful with het HvA-Xmegaboard version 1, 
 *          which has a 32 kHz crystal 
 *
 *          This example is code 23.27 from 'De taal C en de Xmega' second edition,
 *          see <a href="http://www.dolman-wim.nl/xmega/book/index.php">Voorbeelden uit 'De taal C en de Xmega'</a>
 *
 *  \return void
 */
void AutoCalibrationTosc32M(void)
{
  OSC.XOSCCTRL   |= OSC_XOSCSEL_32KHz_gc;       // Select extern 32 kHz TOSC
  OSC.CTRL       |= OSC_XOSCEN_bm;              // Enable extern 32 kHz TOSC
  while(!(OSC.STATUS & OSC_XOSCRDY_bm));        // Wait for oscillator is ready
  OSC.DFLLCTRL   &= ~(OSC_RC32MCREF_gm);        // Select 32 kHz TOSC calibration source
  OSC.DFLLCTRL   |=   OSC_RC32MCREF_XOSC32K_gc; //    for 32 MHz oscillator
  DFLLRC32M.CTRL |= DFLL_ENABLE_bm;             // Enable DFLL for 32 MHz oscillator
}

/*! \brief  Calibrates 2 MHz internal RC oscillator with external 32 kHz crystal
 *
 *          This function is useful with het HvA-Xmegaboard version 1, 
 *          which has a 32 kHz crystal 
 *
 *  \return void
 */
void AutoCalibrationTosc2M(void)
{
  OSC.XOSCCTRL  |= OSC_XOSCSEL_32KHz_gc;
  CCP = CCP_IOREG_gc;
  OSC.CTRL      |= OSC_XOSCEN_bm;
  while(!(OSC.STATUS & OSC_XOSCRDY_bm));
  OSC.DFLLCTRL  &= ~(OSC_RC2MCREF_bm);
  OSC.DFLLCTRL  |= OSC_RC2MCREF_XOSC32K_gc;
  DFLLRC2M.CTRL |= DFLL_ENABLE_bm;
}

