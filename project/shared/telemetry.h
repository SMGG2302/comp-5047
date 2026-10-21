#ifndef HOLLOWHAVEN_TELEMETRY_H
#define HOLLOWHAVEN_TELEMETRY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HH_LINE_CAP 384
#define HH_VERSION 3
#define HH_VALID_SHT 1u
#define HH_VALID_DS18B20 2u
#define HH_VALID_WICK 4u
#define HH_VALID_LIGHT 8u
#define HH_VALID_WATER 16u
#define HH_VALID_SOUND 32u
#define HH_VALID_TOF 64u
#define HH_VALID_ALL 127u
#define HH_COLUMNS "version,boot_id,sequence,uptime_ms,inside_temp_c,inside_rh_pct,outside_temp_c,wick_raw,light_raw,water_raw,valid_mask,sound_rms,sound_p2p,sound_samples,sound_clipped,adc_overflows,distance_mm,animal_present,presence_valid,audio_recording"
#define HH_GREETING "#HOLLOWHAVEN/3 " HH_COLUMNS

typedef struct {
    uint32_t boot_id;
    uint32_t sequence;
    uint64_t uptime_ms;
    float inside_temp_c;
    float inside_rh_pct;
    float outside_temp_c;
    int wick_raw;
    int light_raw;
    int water_raw;
    unsigned valid_mask;
    float sound_rms;
    int sound_p2p;
    uint32_t sound_samples, sound_clipped, adc_overflows;
    int distance_mm;
    bool animal_present, presence_valid, audio_recording;
} hh_sample_t;

/* Invalid floating readings are NAN, invalid ADC readings are -1. */
void hh_sample_clear(hh_sample_t *sample);
bool hh_sample_valid(const hh_sample_t *sample);
int hh_format_sample(char *dest, size_t capacity, const hh_sample_t *sample);
bool hh_parse_sample(const char *line, hh_sample_t *sample);

#endif
