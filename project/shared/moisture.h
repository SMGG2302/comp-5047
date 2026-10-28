#pragma once
#include <stdbool.h>
typedef enum {HH_MOISTURE_UNKNOWN,HH_MOISTURE_DRY,HH_MOISTURE_WET} hh_moisture_t;
/* Hysteresis: preserve previous state in the middle, unknown on invalid data. */
static inline hh_moisture_t hh_moisture_update(hh_moisture_t previous,bool valid,int raw)
{
    if (!valid || raw<0 || raw>4095) return HH_MOISTURE_UNKNOWN;
    if (raw<2000) return HH_MOISTURE_WET;
    if (raw>=2300) return HH_MOISTURE_DRY;
    return previous;
}
