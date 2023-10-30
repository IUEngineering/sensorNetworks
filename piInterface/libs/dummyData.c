#include <ncurses.h>
#include <stdint.h>

#include "dummyData.h"
#include "mcuComm.h"
#include "interface.h"

#define AIR_HUMIDITY    0x01
#define AIR_QUALITY     0x02
#define LIGHT           0x03
#define TEMPERATURE     0x04
#define LOUDNESS        0x05

// Meta Conclusions defines
#define RED 2
#define YELLOW 1
#define GREEN 0

WINDOW *dummyDataWindow;
WINDOW *metaWindow;

static void drawDummyData(WINDOW *window);
static void drawMetaConclusions(WINDOW *window);

static dummyData_t data = {0};

dummyData_t accumulateData(uint8_t *payload, uint8_t debugMode) {
    
    switch(payload[0]) {
        case AIR_HUMIDITY:
            data.airHumiddity = payload[2];

            break;
        case AIR_QUALITY:
            if (payload[2] == 0) data.airQuality = 'X';
            else data.airQuality = payload[2];  
            fprintf(stderr, "Data air quality: %c\n", data.airQuality);

            break;
        case LIGHT:
            // light is 16bits and payload is 8 bit. So we need to bitshift it.
            data.light =  ((uint16_t) payload[2]) << 8;
            data.light |= payload[3] ;

            break;
        case TEMPERATURE:
            data.temperature = payload[2];

            break;
        case LOUDNESS:
            data.loudness = payload[2];

            break;  
    }    
    drawDummyData(dummyDataWindow);
    if(!debugMode) drawMetaConclusions(metaWindow);
    return data;
}

void initDummyData(WINDOW *window) {
    dummyDataWindow = window; 
    wmove(window, 0, 0);
    wattrset(window, COLOR_PAIR(LINE_PAIR));
    for(uint8_t i = 0; i < getmaxx(window); i++) {
        waddch(window, ' ');
    }
    wmove(window, 0, 0);
    wprintw(window, " Data from nodes: "); 
    wattrset(window, 0);
    drawDummyData(window);
}

void initMetaWindow(WINDOW *window) {
    metaWindow = window;
    drawMetaConclusions(window);
}

void drawDummyData(WINDOW *window) {
    wmove(window, 1, 0);
    wprintw(window, " Air Humiddity: %u %%\n", data.airHumiddity);
    wprintw(window, " Air Quality:   %c \n", data.airQuality ? data.airQuality : 'X');
    wprintw(window, " Light:         %u Lm\n", data.light);
    wprintw(window, " Temperature:   %u ", data.temperature);
    waddch(window, ACS_DEGREE);
    wprintw(window, "C\n");
    wprintw(window, " Loudness:      %u dB\n", data.loudness);
    wrefresh(window);
    fprintf(stderr, "kanekr\n");
}


// Temp: low < 15 deg C, high = 22 deg C
// Air humidity: low < 40%, high > 60%

// Air quality: BAD    > 25 mg/m^2
//              OKAY  16 - > 25 mg/m^2
//              GOOD   > 25 mg/m^2

// Light:         > 1500 Lm

uint8_t metaConslusions(void) {
    uint8_t conclusions = 0x00; 

    if( (data.airHumiddity > 40 || data.airQuality == 'R') && data.temperature > 22 ) { 
        conclusions |= OPEN_WINDOW_bm;
    }

    return conclusions;
}

void drawMetaConclusions(WINDOW *window){
    uint8_t conclusions = metaConslusions();
    if(conclusions & OPEN_WINDOW_bm) {
        wmove(window, 0, 0);
        wprintw(window, "Doe een raam open, de luchtkwaliteit is niet goed!!!");
    }
    else wclear(window);
    
    wrefresh(window);
}