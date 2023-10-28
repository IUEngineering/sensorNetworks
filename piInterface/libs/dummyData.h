#ifndef __DUMMYDATA_H__
#define __DUMMYDATA_H__

#include <stdint.h>

#define OPEN_WINDOW_bm 0x01
#define CLOSE_WINDOW_bm 0x02

typedef struct {
    uint8_t  airHumiddity;
    uint8_t  airQuality;
    uint16_t light;
    uint8_t  temperature;
    uint8_t  loudness;
}dummyData_t;

#endif // __DUMMYDATA_H__