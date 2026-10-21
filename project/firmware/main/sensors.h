#ifndef HH_SENSORS_H
#define HH_SENSORS_H
#include "telemetry.h"
void hh_sensors_init(void);
void hh_sensors_read(hh_sample_t *sample);
#endif
