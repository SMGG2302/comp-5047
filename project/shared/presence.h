#pragma once
#include <stdbool.h>

typedef struct {
    bool animal_present;
    bool presence_valid;
    unsigned near_count, far_count;
} hh_presence_t;

/* Invalid readings never imply departure, and break confirmation streaks.
 * Returns true only on a confirmed absent -> present transition. */
static inline bool hh_presence_update(hh_presence_t *s, bool valid, int mm,
    int enter_mm, int exit_mm, unsigned enter_count, unsigned exit_count)
{
    s->presence_valid = valid;
    if (!valid) { s->near_count=s->far_count=0; return false; }
    if (!s->animal_present) {
        s->far_count=0;
        s->near_count = mm <= enter_mm ? s->near_count+1 : 0;
        if (s->near_count >= enter_count) {
            s->animal_present=true; s->near_count=0; return true;
        }
    } else {
        s->near_count=0;
        s->far_count = mm >= exit_mm ? s->far_count+1 : 0;
        if (s->far_count >= exit_count) {
            s->animal_present=false; s->far_count=0;
        }
    }
    return false;
}
