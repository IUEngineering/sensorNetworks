#ifndef __DUMMYDATA_H__
#define __DUMMYDATA_H__

#include <stdint.h>
#include <ncurses.h>

#define OPEN_WINDOW_bm 0x01
#define CLOSE_WINDOW_bm 0x02

void initDummyData(WINDOW *window);

typedef struct {
    uint8_t  airHumiddity;
    uint8_t  airQuality;
    uint16_t light;
    uint8_t  temperature;
    uint8_t  loudness;
}dummyData_t;

dummyData_t accumulateData(uint8_t *payload, uint8_t debugMode);
void initMetaWindow(WINDOW *window);
#endif // __DUMMYDATA_H__