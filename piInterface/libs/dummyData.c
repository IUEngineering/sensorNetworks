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

static void drawMetaConclusions(WINDOW *window);

static dummyData_t data = {0};

dummyData_t accumulateData(uint8_t *payload, uint8_t debugMode) {
    
    switch(payload[0]) {
        case AIR_HUMIDITY:
            data.airHumiddity = payload[2];
            data.airHumiddityLastUpdate = time(NULL);
 
 
            break;
        case AIR_QUALITY:
            data.airQuality = payload[2];
            data.airQualityLastUpdate = time(NULL);


            break;
        case LIGHT:
            // light is 16bits and payload is 8 bit. So we need to bitshift it.
            data.light =  ((uint16_t) payload[2]) << 8;
            data.light |= payload[3] ;
            data.lightLastUpdate = time(NULL);

            break;
        case TEMPERATURE:
            data.temperature = payload[2];
            data.temperatureLastUpdate = time(NULL);
 
 
            break;
        case LOUDNESS:
            data.loudness = payload[2];
            data.loudnessLastUpdate = time(NULL);
 
 
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
    time_t now = time(NULL);

    wmove(window, 1, 0);

    if(data.airHumiddityLastUpdate) {
        double timeDiff = difftime(now, data.airHumiddityLastUpdate);
        
        wprintw(window, "Air Humiddity: %u %%\t%ds ago\n", data.airHumiddity, (uint16_t)timeDiff);
    }
    else wprintw(window, "Air Humiddity: measuring..\n");

    if(data.airQualityLastUpdate) {
        double timeDiff = difftime(now, data.airQualityLastUpdate);
        
        wprintw(window, "Air Quality:   %u mg/m^3\t%ds ago\n", data.airQuality, (uint16_t)timeDiff);
    }
    else wprintw(window, "Air Quality:   measuring..\n");

    if(data.lightLastUpdate) {
        double timeDiff = difftime(now, data.lightLastUpdate);
        
        wprintw(window, "Light:         %u Lx\t%ds ago\n", data.light, (uint16_t)timeDiff);
    }
    else wprintw(window, "Light:         measuring..\n");

    if(data.temperatureLastUpdate) {
        double timeDiff = difftime(now, data.temperatureLastUpdate);
        
        wprintw(window, "%u ", data.temperature);
        waddch(window, ACS_DEGREE);
        wprintw(window, "C\t%ds ago\n", (uint16_t)timeDiff);
    }
    else wprintw(window, "Temperature:   measuring..\n");

    if(data.loudnessLastUpdate) {
        double timeDiff = difftime(now, data.loudnessLastUpdate);
        
        wprintw(window, "Loudness:      %u dBA\t%ds ago\n", data.loudness, (uint16_t)timeDiff);
    }
    else wprintw(window, "Loudness:      measuring..\n");

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

    if( (data.airHumiddity >= 60 || data.airQuality >= 3) && data.temperature >= 23 ) { 
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