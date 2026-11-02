#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

#define HH_VALVE_OPEN_TEMP_C 30.0f
#define HH_VALVE_CLOSE_TEMP_C 26.0f
#define HH_VALVE_CHECK_INTERVAL_MS 60000u

typedef struct {
    bool open;
    bool timing_started;
    uint64_t next_check_ms;
} hh_valve_control_t;

/* Light gates entry only. Time starts once the servo reaches its open command.
 * Invalid temperature cannot establish cooling; retry at the next minute.
 * Call with monotonic milliseconds. No catch-up checks after delayed calls. */
static inline bool hh_valve_control_update(hh_valve_control_t *state,
    uint64_t now_ms, bool light_qualified, bool temperature_valid,
    float temperature_c, bool fully_open)
{
    bool valid=temperature_valid && isfinite(temperature_c);
    if (!state->open) {
        if (light_qualified && valid && temperature_c>HH_VALVE_OPEN_TEMP_C) {
            state->open=true;
            state->timing_started=false;
        }
    } else if (!state->timing_started) {
        if (fully_open) {
            state->timing_started=true;
            state->next_check_ms=now_ms+HH_VALVE_CHECK_INTERVAL_MS;
        }
    } else if (now_ms>=state->next_check_ms) {
        if (valid && temperature_c<HH_VALVE_CLOSE_TEMP_C) {
            state->open=false;
            state->timing_started=false;
            state->next_check_ms=0;
        } else {
            state->next_check_ms=now_ms+HH_VALVE_CHECK_INTERVAL_MS;
        }
    }
    return state->open;
}
