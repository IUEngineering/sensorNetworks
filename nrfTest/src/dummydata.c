#include "iso.h"
#include "nrf24L01.h"
#include "nrf24spiXM2.h"
#include "serialF0.h"

//read data from potmeter placed on PA2/PA3
void ADCInit(void) {
    PORTA.DIRCLR     = PIN2_bm|PIN3_bm;
    ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN2_gc | ADC_CH_MUXNEG_GND_MODE3_gc;
    ADCA.CH0.CTRL    = ADC_CH_INPUTMODE_DIFF_gc;  
    ADCA.REFCTRL     = ADC_REFSEL_INTVCC_gc;
    ADCA.CTRLB       = ADC_RESOLUTION_12BIT_gc | ADC_CONMODE_bm;            
    ADCA.PRESCALER   = ADC_PRESCALER_DIV16_gc;
    ADCA.CTRLA       = ADC_ENABLE_bm;
}

// return a signed
int16_t ADCRead(void) {                                   
    int16_t res;

    ADCA.CH0.CTRL |= ADC_CH_START_bm;
    while ( !(ADCA.CH0.INTFLAGS & ADC_CH_CHIF_bm) ) ;
    res = ADCA.CH0.RES;
    ADCA.CH0.INTFLAGS |= ADC_CH_CHIF_bm;

    return res;
}


// // Send potmeter data from button PA0 to hardcoded XMEGA-ID
// if(PORTA.IN & PIN0_bm) {
//     int16_t result = read_adc();
//     send( (uint8_t *) &result, sizeof(uint16_t) );  // little endian: low byte is sent first
// }

// // Send data as broadcast
// if(PORTA.IN & PIN5_bm) {

// }