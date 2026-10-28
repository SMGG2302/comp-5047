#include <assert.h>
#include "maintenance.h"
int main(void)
{
    assert(!hh_outlet_blocked(HH_VALVE_OPEN,59999,HH_MOISTURE_DRY,HH_MOISTURE_DRY));
    assert(hh_outlet_blocked(HH_VALVE_OPEN,60000,HH_MOISTURE_DRY,HH_MOISTURE_WET));
    assert(hh_outlet_blocked(HH_VALVE_OPEN,60000,HH_MOISTURE_WET,HH_MOISTURE_DRY));
    assert(!hh_outlet_blocked(HH_VALVE_OPEN,60000,HH_MOISTURE_WET,HH_MOISTURE_WET));
    assert(!hh_outlet_blocked(HH_VALVE_OPEN,60000,HH_MOISTURE_UNKNOWN,HH_MOISTURE_UNKNOWN));
    assert(!hh_outlet_blocked(HH_VALVE_CLOSED,70000,HH_MOISTURE_DRY,HH_MOISTURE_DRY));
    assert(!hh_outlet_blocked(HH_VALVE_OPENING,70000,HH_MOISTURE_DRY,HH_MOISTURE_DRY));
    assert(!hh_outlet_blocked(HH_VALVE_OPEN,0,HH_MOISTURE_DRY,HH_MOISTURE_DRY));
    return 0;
}
