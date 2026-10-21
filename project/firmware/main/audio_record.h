#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
void hh_audio_init(uint32_t boot);
void hh_audio_push(const int16_t *samples, size_t count, int64_t end_us);
void hh_audio_gap(void);

bool hh_audio_trigger(void);
bool hh_audio_is_recording(void);
