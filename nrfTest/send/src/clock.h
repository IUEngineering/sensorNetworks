/*!
 *  \file    clock.h
 *  \author  Wim Dolman (<a href="mailto:w.e.dolman@hva.nl">w.e.dolman@hva.nl</a>)
 *  \date    10-07-2019
 *  \version 1.4
 *
 *  \brief   Clock functions for Xmega 
 *
 */
#ifndef CLOCK_H_
#define CLOCK_H_

void Config32MHzClock(void);
void Config32MHzClock_Ext16M(void);
void Config16MHzClock_Ext16M(void);

/*! \brief  Enables 32 MHz clock with an external 16 MHz crystal
 *
 *          This inline function is useful with het HvA-Xmegaboard version 2, 
 *          which has a 16 MHz crystal 
 *
 * \return void
 */
void inline init_clock(void) {
  Config32MHzClock_Ext16M();
}

void AutoCalibration32M(void);
void AutoCalibration2M(void);
void AutoCalibrationTosc32M(void);
void AutoCalibrationTosc2M(void);


#endif // CLOCK_H_ 