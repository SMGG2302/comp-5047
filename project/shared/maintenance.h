#pragma once
#include "telemetry.h"
#include "moisture.h"
/* Unknown readings never count as dry. Timing is measured on the device. */
static inline bool hh_outlet_blocked(unsigned valve, uint32_t open_ms, unsigned left1, unsigned left2)
{
    return valve==HH_VALVE_OPEN && open_ms>=60000 &&
        (left1==HH_MOISTURE_DRY || left2==HH_MOISTURE_DRY);
}
