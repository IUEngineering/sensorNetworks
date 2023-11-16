#ifndef __DUMMYDATA_H__
#define __DUMMYDATA_H__

#include <stdint.h>
#include <time.h>
#include <ncurses.h>

#define OPEN_WINDOW_bm 0x01
#define CLOSE_WINDOW_bm 0x02

void initDummyData(WINDOW *window);

typedef struct {
    uint8_t  airHumiddity;
    time_t airHumiddityLastUpdate;
    uint8_t  airQuality;
    time_t airQualityLastUpdate;
    uint16_t light;
    time_t lightLastUpdate;
    uint8_t  temperature;
    time_t temperatureLastUpdate;
    uint8_t  loudness;
    time_t loudnessLastUpdate;
}dummyData_t;

dummyData_t accumulateData(uint8_t *payload, uint8_t debugMode);
void initMetaWindow(WINDOW *window);
void drawDummyData(WINDOW *window);

#endif // __DUMMYDATA_H__