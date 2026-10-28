#include "moisture.h"
#include <stdio.h>
#define CHECK(x) do{if(!(x))return 1;}while(0)
int main(void){
 CHECK(hh_moisture_update(HH_MOISTURE_UNKNOWN,true,2400)==HH_MOISTURE_DRY);
 CHECK(hh_moisture_update(HH_MOISTURE_DRY,true,1999)==HH_MOISTURE_WET);
 CHECK(hh_moisture_update(HH_MOISTURE_WET,true,2000)==HH_MOISTURE_WET);
 CHECK(hh_moisture_update(HH_MOISTURE_DRY,true,2200)==HH_MOISTURE_DRY);
 CHECK(hh_moisture_update(HH_MOISTURE_UNKNOWN,true,2100)==HH_MOISTURE_UNKNOWN);
 CHECK(hh_moisture_update(HH_MOISTURE_WET,true,2300)==HH_MOISTURE_DRY);
 CHECK(hh_moisture_update(HH_MOISTURE_WET,false,1800)==HH_MOISTURE_UNKNOWN);
 CHECK(hh_moisture_update(HH_MOISTURE_WET,true,4096)==HH_MOISTURE_UNKNOWN);
 puts("PASS: moisture thresholds, hysteresis and invalid values");return 0;
}
