#include <avr/io.h>

#include "iso.h"
#include "serialF0.h"

#define BASESTATION_ID  0x40

#define PAYLOAD_LENGTH  31

#define AIR_MOIST_MESSAGE   0x01
#define AIR_QUALITY_MESSAGE 0x02
#define LIGHT_MESSAGE       0x03
#define TEMP_MESSAGE        0x04
#define SOUND_MESSAGE       0x05

#define TIME_5_SEC  5
#define TIME_10_SEC 10
#define TIME_10_MIN 600
#define TIME_30_MIN 1800

static void ADCInit(void);
static uint16_t ADCReadCH0(void);

static void sendAirMoisture(void);
static void sendAirQuality(void);
static void sendLight(void);
static void sendTemp(void);
static void sendSound(void);

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
    // The PER of period cannot be set high enough for 10sec so we need to count to ten.
    uint16_t timer = 0;

    while (1) {

        while(! TCE0.INTFLAGS & TC0_OVFIF_bm)
            isoUpdate();

        TCE0.INTFLAGS = TC0_OVFIF_bm;
        timer++;

        if (timer % TIME_10_SEC == 0) {

            uint8_t payload[PAYLOAD_LENGTH];
            uint16_t temp, sound;

            temp  = ADCReadCH0();
            sound = ADCReadCH1();
            
            // Make temp payload
            payload[0] = TEMP_MESSAGE;
            payload[1] = temp >> 8;
            payload[2] = (uint8_t) temp;
            isoSendPacket(BASESTATION_ID, payload, PAYLOAD_LENGTH);

            // Make sound payload
            payload[0] = SOUND_MESSAGE;
            payload[1] = sound >> 8;
            payload[2] = (uint8_t) sound;
            isoSendPacket(BASESTATION_ID, payload, PAYLOAD_LENGTH);
        }

        if (timer % TIME_5_SEC == 0);

        if (timer % TIME_10_MIN == 0);

        if (timer % TIME_30_MIN == 0) {

            timer = 0;
        }
    }
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

static void sendAirMoisture(void) {
    return;
}

static void sendAirQuality(void) {
    return;
}

static void sendLight(void) {
    return;
}

static void sendTemp(void) {
    return;
}

static void sendSound(void) {
    return;
}


// Callback for iso when data is received for this node
void receivePayload(uint8_t *data) {
    return;
}