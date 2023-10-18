#include <avr/io.h>

#include "iso.h"
#include "serialF0.h"


static uint16_t ADCReadCH0(void);
static uint16_t ADCReadCH1(void);

static void receivePayload(uint8_t *data);

// Initialise the dummyData program
void dummyDataInit(void) {
    isoInit(receivePayload);
    ADCInit();

    // Configure TCE0 to set its interrupt flag every second
    TCE0.CTRLB     = TC_WGMODE_NORMAL_gc;  // Normal mode
    TCE0.CTRLA     = TC_CLKSEL_DIV1024_gc;    // prescaling 8
    TCE0.PER       = 31249; // PER = (t*FCPU / N)-1 = (32000000/1024)-1=31249
}

// The continues loop of the dummyData program 
void dummyDataLoop(void) {

}

// Configure ADCA:
//      - Reference to internal VCC/1.6 
//      - In 12 bit mode
//      - Prescaler to div 16
//      - Input PA2 to channel 0
//      - Input PA3 to channel 1
//      - Input mode to single ended
//      - Configure multiplexer
void ADCInit(void) {

    // congigure ADCA
    ADCA.REFCTRL     = ADC_REFSEL_INTVCC_gc;
    ADCA.CTRLB       = ADC_RESOLUTION_12BIT_gc | ADC_CONMODE_bm;            
    ADCA.PRESCALER   = ADC_PRESCALER_DIV16_gc;
    ADCA.CTRLA       = ADC_ENABLE_bm;
    
    // Configure input channels
    PORTA.DIRCLR     = PIN2_bm | PIN3_bm;
    ADCA.CH0.CTRL    = ADC_CH_INPUTMODE_SINGLEENDED_gc;
    ADCA.CH1.CTRL    = ADC_CH_INPUTMODE_SINGLEENDED_gc;

    ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN2_gc | ADC_CH_MUXNEG_GND_MODE3_gc;
    ADCA.CH1.MUXCTRL = ADC_CH_MUXPOS_PIN3_gc | ADC_CH_MUXNEG_GND_MODE3_gc;    
}

// Start a conversion on CH0, wait until the conversion finishes and return the result
uint16_t ADCReadCH0(void) {                                   
    uint16_t res;

    ADCA.CH0.CTRL |= ADC_CH_START_bm;
    while ( !(ADCA.CH0.INTFLAGS & ADC_CH_CHIF_bm) );

    res = ADCA.CH0.RES;
    ADCA.CH0.INTFLAGS |= ADC_CH_CHIF_bm;

    return res;
}

// Start a conversion on CH1, wait until the conversion finishes and return the result
uint16_t ADCReadCH1(void) {                                   
    uint16_t res;

    ADCA.CH0.CTRL |= ADC_CH_START_bm;
    while ( !(ADCA.CH0.INTFLAGS & ADC_CH_CHIF_bm) );

    res = ADCA.CH0.RES;
    ADCA.CH0.INTFLAGS |= ADC_CH_CHIF_bm;

    return res;
}

// Callback for iso when data is received for this node
void receivePayload(uint8_t *data) {
    return;
}

// // Send potmeter data from button PA0 to hardcoded XMEGA-ID
// if(PORTA.IN & PIN0_bm) {
//     int16_t result = read_adc();
//     send( (uint8_t *) &result, sizeof(uint16_t) );  // little endian: low byte is sent first
// }

// // Send data as broadcast
// if(PORTA.IN & PIN5_bm) {

// }