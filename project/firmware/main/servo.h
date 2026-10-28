#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include "telemetry.h"
/* Positional MG90S; pulse limits and mechanical angle must be calibrated. */
esp_err_t hh_servo_init(void);
esp_err_t hh_servo_set_angle(unsigned degrees);
/* Non-blocking request; the servo task moves gradually to the target. */
esp_err_t hh_servo_set_valve(bool open);
void hh_servo_snapshot(hh_sample_t *sample);
