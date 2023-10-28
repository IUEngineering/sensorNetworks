#include <ncurses.h>
#include <stdint.h>

#include "dummyData.h"
#include "mcuComm.h"

#define AIR_HUMIDITY    0x01
#define AIR_QUALITY     0x02
#define LIGHT           0x03
#define TEMPERATURE     0x04
#define LOUDNESS        0x05

// Meta Conclusions defines
#define RED 2
#define YELLOW 1
#define GREEN 0



static dummyData_t data = {0};

dummyData_t accumulateData(uint8_t *payload) {
    
    switch(payload[0]) {
        case AIR_HUMIDITY:
            data.airHumiddity = payload[2];

            break;
        case AIR_QUALITY:
            data.airQuality = payload[2];

            break;
        case LIGHT:
            // light is 16bits and payload is 8 bit. So we need to bitshift it.
            data.light = payload[3] << 8;
            data.light |= payload[2];

            break;
        case TEMPERATURE:
            data.temperature = payload[2];

            break;
        case LOUDNESS:
            data.loudness = payload[2];

            break;  
    }    

    return data;
}

void initDummyData(WINDOW *debug, WINDOW *meta) {
    
}

void drawDummyData(WINDOW *debug) {

}


// Temp: low < 15 deg C, high = 22 deg C
// Air humidity: low < 40%, high > 60%

// Air quality: BAD    > 25 mg/m^2
//              OKAY  16 - > 25 mg/m^2
//              GOOD   > 25 mg/m^2

// Light:         > 1500 Lm

uint8_t metaConslusions(void) {
    uint8_t conclusions = 0x00; 

    if( (data.airHumiddity > 40 || data.airQuality > 25) && data.temperature > 22 ) { 
        conclusions |= OPEN_WINDOW_bm;
    }

    return conclusions;
}
