// Explenation of the hardware connections needed for this program:
//  - Sensor inputs are:
//      - PA0 -> Air moisture 
//      - PA1 -> Air quality
//      - PA2 -> Light
//      - PA3 -> Temperature
//      - PA4 -> Sound
//
//  - Offset calibration input:
//      - PA5 -> GND
//
//  - Enable inputs (enable = 0, disable = 1)
//      - PB0 (pullup) -> Air moisture
//      - PB1 (pullup) -> Air quality
//      - PB2 (pullup) -> Light
//      - PB3 (pullup) -> Temperature
//      - PB4 (pullup) -> Sound
//
//  - Force send all enabled dummy data (PB5 = 0)
//      - PB5 (pullup)
//
//  - All analog inputs must be between 0V and VCC/1.6 (VCC = 3V3) 

#include <avr/io.h>
#include <string.h>

#include "iso.h"
#include "encrypt.h"
#include "serialF0.h"

#define BASESTATION_ID  0x41

#define PAYLOAD_LENGTH  31

#define AIR_MOIST_MESSAGE   0x01
#define AIR_QUALITY_MESSAGE 0x02
#define LIGHT_MESSAGE       0x03
#define TEMP_MESSAGE        0x04
#define SOUND_MESSAGE       0x05

#define MAX_TEMP    30
#define MAX_VOCHT   100
#define MAX_SOUND   80
#define MAX_VOC     10
#define MAX_LIGHT   1000
#define MAX_ADC     3910 // I have terminal evidence for this value (I read it from the terminal)

#define TIME_1_SEC  1
#define TIME_5_SEC  5
#define TIME_10_SEC 10
#define TIME_10_MIN 600
#define TIME_30_MIN 1800

#define AIR_QUALITY_BAD  MAX_ADC * 1/3
#define AIR_QUALITY_MED  MAX_ADC * 2/3


static void ADCInit(void);
static uint16_t ADCReadCH0(uint8_t inputPin);

static void sendAirMoisture(void);
static void sendVOC(void);
static void sendLight(void);
static void sendTemp(void);
static void sendSound(void);

static void receivePayload(uint8_t *data);
static void checkButton(void);

static char key1[] = "VERON Zendamateur";
static char key2[] = "PI5VLE";

// Initialise the dummyData program
void dummyDataInit(void) {
    isoInit(receivePayload);
    ADCInit();

    PORTB.DIRCLR =  PIN0_bm |
                    PIN1_bm |
                    PIN2_bm |
                    PIN3_bm |
                    PIN4_bm |
                    PIN5_bm;
    
    PORTB.PIN0CTRL = PORT_OPC_PULLUP_gc | PORT_INVEN_bm;
    PORTB.PIN1CTRL = PORT_OPC_PULLUP_gc | PORT_INVEN_bm;
    PORTB.PIN2CTRL = PORT_OPC_PULLUP_gc | PORT_INVEN_bm;
    PORTB.PIN3CTRL = PORT_OPC_PULLUP_gc | PORT_INVEN_bm;
    PORTB.PIN4CTRL = PORT_OPC_PULLUP_gc | PORT_INVEN_bm;
    PORTB.PIN5CTRL = PORT_OPC_PULLUP_gc | PORT_INVEN_bm;

    // Configure TCE0 to set its interrupt flag every second
    TCE0.CTRLB     = TC_WGMODE_NORMAL_gc;       // Normal mode
    TCE0.CTRLA     = TC_CLKSEL_DIV1024_gc;      // prescaling 1024
    TCE0.PER       = 31249; // PER = (t*FCPU / N)-1 = (32000000/1024)-1=31249   
}

// The continues loop of the dummyData program 
void dummyDataLoop(void) {
    // The PER of period cannot be set high enough for 10sec so we need to count to ten.
    uint16_t timer = 0;

    while (1) {
        while(! (TCE0.INTFLAGS & TC0_OVFIF_bm)) {
            isoUpdate();
            checkButton();
        }

        TCE0.INTFLAGS = TC0_OVFIF_bm;
        timer++;

        // Lmao I hate this code with a passion.
        if ((timer % TIME_1_SEC == 0) && (PORTB.IN & PIN4_bm))
            sendSound();

        if ((timer % TIME_1_SEC == 0) && (PORTB.IN & PIN2_bm))
            sendLight();

        if ((timer % TIME_1_SEC == 0) && (PORTB.IN & PIN1_bm))
            sendVOC();

        if ((timer % TIME_1_SEC == 0) && (PORTB.IN & PIN3_bm))
            sendTemp();
            
        if ((timer % TIME_1_SEC == 0) && (PORTB.IN & PIN0_bm))
            sendAirMoisture();

        if(timer % TIME_30_MIN == 0) timer = 0;
    }
}


void checkButton(void) {
    static uint8_t prevButton = 0;
    if (PORTB.IN & PIN5_bm) {
        if(prevButton) return;
        prevButton = 1;

        PORTC.OUTTGL = PIN0_bm;

        if (PORTB.IN & PIN4_bm)
            sendSound();

        if (PORTB.IN & PIN2_bm)
            sendLight();

        if (PORTB.IN & PIN1_bm)
            sendVOC();
        
        if (PORTB.IN & PIN3_bm)
            sendTemp();
        
        if (PORTB.IN & PIN0_bm)
            sendAirMoisture();
    }
    else prevButton = 0;
}

// Configure ADCA:
//      - Reference to internal VCC/1.6 
//      - In 12 bit mode
//      - Prescaler to div 16
//      - Input mode to single ended
void ADCInit(void) {

    // congigure ADCA
    ADCA.REFCTRL     = ADC_REFSEL_INTVCC_gc;
    ADCA.CTRLB       = ADC_RESOLUTION_12BIT_gc;            
    ADCA.PRESCALER   = ADC_PRESCALER_DIV16_gc;
    
    // Configure input channels
    PORTA.DIRCLR     =  PIN0_bm |
                        PIN1_bm |
                        PIN2_bm | 
                        PIN3_bm |
                        PIN4_bm |
                        PIN5_bm;
    
    ADCA.CH0.CTRL    = ADC_CH_INPUTMODE_SINGLEENDED_gc;

    // Enable the ADC
    ADCA.CTRLA       = ADC_ENABLE_bm;
}

// Start a conversion on CH0, wait until the conversion finishes and return the result
// The inputPin argument selects the input signal for the ADC
uint16_t ADCReadCH0(uint8_t inputPin) { 
    uint16_t offset;                                  
    uint16_t res;

    // Measure the offset
    ADCA.CH0.MUXCTRL = ADC_CH_MUXPOS_PIN5_gc;

    ADCA.CH0.CTRL |= ADC_CH_START_bm;
    while ( !(ADCA.CH0.INTFLAGS & ADC_CH_CHIF_bm) );
    
    offset = ADCA.CH0.RES;
    ADCA.CH0.INTFLAGS |= ADC_CH_CHIF_bm;

    // Measure the signal
    ADCA.CH0.MUXCTRL = inputPin;

    ADCA.CH0.CTRL |= ADC_CH_START_bm;
    while ( !(ADCA.CH0.INTFLAGS & ADC_CH_CHIF_bm) );

    res = ADCA.CH0.RES;
    ADCA.CH0.INTFLAGS |= ADC_CH_CHIF_bm;

    if (offset > res)
        res = offset;

    return res - offset;
}

static void sendAirMoisture(void) {
    uint8_t payload[PAYLOAD_LENGTH] = {0};

    uint16_t airMoisture16 = ADCReadCH0(ADC_CH_MUXPOS_PIN0_gc);
    uint8_t  airMoisture8  = (uint8_t) ((uint32_t)airMoisture16 * MAX_VOCHT / MAX_ADC); 

    // Fill payload
    payload[0] = AIR_MOIST_MESSAGE;
    payload[1] = isoGetId();
    payload[2] = airMoisture8;

    isoSendPacket(BASESTATION_ID, keysEncrypt(payload, PAYLOAD_LENGTH, key1, strlen(key1), key2, strlen(key2)), PAYLOAD_LENGTH);
    return;
}

static void sendVOC(void) {
    uint8_t payload[PAYLOAD_LENGTH] = {0};

    uint16_t volatileOrganicCompounds = (uint16_t)ADCReadCH0(ADC_CH_MUXPOS_PIN1_gc) * MAX_VOC / MAX_ADC;

    // Fill payload
    payload[0] = AIR_QUALITY_MESSAGE;
    payload[1] = isoGetId();
    payload[2] = volatileOrganicCompounds;

        
    isoSendPacket(BASESTATION_ID, keysEncrypt(payload, PAYLOAD_LENGTH, key1, strlen(key1), key2, strlen(key2)), PAYLOAD_LENGTH);
    return;
}

static void sendLight(void) {
    uint8_t payload[PAYLOAD_LENGTH] = {0};

    uint16_t light = ADCReadCH0(ADC_CH_MUXPOS_PIN2_gc);
    light = (uint32_t)light * MAX_LIGHT / MAX_ADC;

    printf("%d\n", light);

    // Fill payload
    payload[0] = LIGHT_MESSAGE;
    payload[1] = isoGetId();
    payload[2] = (uint8_t) (light >> 8);      // Place MSB into the payload
    payload[3] = (uint8_t) (light); // Place LSB into the payload
    
    isoSendPacket(BASESTATION_ID, keysEncrypt(payload, PAYLOAD_LENGTH, key1, strlen(key1), key2, strlen(key2)), PAYLOAD_LENGTH);
    return;
}

static void sendTemp(void) {
    uint8_t payload[PAYLOAD_LENGTH] = {0};

    uint16_t temp16 = ADCReadCH0(ADC_CH_MUXPOS_PIN3_gc);

    // Map 12bit value to 8bit
    // (in - in_min) * (out_max - out_min) / (in_max - in_min) + out_min
    uint8_t temp8 = (uint8_t) ((uint32_t)temp16 * MAX_TEMP / MAX_ADC); 

    // Fill payload
    payload[0] = TEMP_MESSAGE;
    payload[1] = isoGetId();
    payload[2] = temp8;

    isoSendPacket(BASESTATION_ID, keysEncrypt(payload, PAYLOAD_LENGTH, key1, strlen(key1), key2, strlen(key2)), PAYLOAD_LENGTH);
    return;
}

static void sendSound(void) {
    uint8_t payload[PAYLOAD_LENGTH] = {0};

    uint16_t sound16 = ADCReadCH0(ADC_CH_MUXPOS_PIN4_gc);

    // Map 12bit value to 8bit
    // (in - in_min) * (out_max - out_min) / (in_max - in_min) + out_min
    uint8_t sound8 = (uint8_t) ((uint32_t)sound16 * MAX_SOUND / MAX_ADC); 

    // Fill payload
    payload[0] = SOUND_MESSAGE;
    payload[1] = isoGetId();
    payload[2] = sound8;

    isoSendPacket(BASESTATION_ID, keysEncrypt(payload, PAYLOAD_LENGTH, key1, strlen(key1), key2, strlen(key2)), PAYLOAD_LENGTH);
    return;
}


// Callback for iso when data is received for this node
void receivePayload(uint8_t *data) {
    return;
}