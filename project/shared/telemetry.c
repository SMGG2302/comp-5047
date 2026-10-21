#include "telemetry.h"

#include <errno.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void hh_sample_clear(hh_sample_t *s)
{
    memset(s, 0, sizeof(*s));
    s->sound_rms = NAN;
    s->sound_p2p = -1;
    s->distance_mm = -1;
    s->inside_temp_c = NAN;
    s->inside_rh_pct = NAN;
    s->outside_temp_c = NAN;
    s->wick_raw = s->light_raw = s->water_raw = -1;
}

static bool valid_float(float value, bool present, float low, float high)
{
    return present ? (isfinite(value) && value >= low && value <= high) : isnan(value);
}

static bool valid_adc(int value, bool present)
{
    return present ? (value >= 0 && value <= 4095) : value == -1;
}

bool hh_sample_valid(const hh_sample_t *s)
{
    if (s->valid_mask & ~HH_VALID_ALL) return false;
    if(s->presence_valid != ((s->valid_mask & HH_VALID_TOF)!=0))return false;
    if(s->presence_valid ? (s->distance_mm<=0 || s->distance_mm>2000) : s->distance_mm!=-1)return false;
    bool sound = (s->valid_mask & HH_VALID_SOUND) != 0;
    if (!valid_float(s->sound_rms, sound, 0, 4095)
        || !valid_adc(s->sound_p2p, sound)
        || (sound ? s->sound_samples == 0 : s->sound_samples != 0)
        || s->sound_clipped > s->sound_samples) return false;
    return valid_float(s->inside_temp_c, (s->valid_mask & HH_VALID_SHT) != 0, -45, 130)
        && valid_float(s->inside_rh_pct, (s->valid_mask & HH_VALID_SHT) != 0, 0, 100)
        && valid_float(s->outside_temp_c, (s->valid_mask & HH_VALID_DS18B20) != 0, -55, 125)
        && valid_adc(s->wick_raw, (s->valid_mask & HH_VALID_WICK) != 0)
        && valid_adc(s->light_raw, (s->valid_mask & HH_VALID_LIGHT) != 0)
        && valid_adc(s->water_raw, (s->valid_mask & HH_VALID_WATER) != 0);
}

int hh_format_sample(char *dest, size_t capacity, const hh_sample_t *s)
{
    if (!hh_sample_valid(s)) return -1;
    int n = snprintf(dest, capacity,
        "3,%" PRIu32 ",%" PRIu32 ",%" PRIu64 ",%.2f,%.2f,%.2f,%d,%d,%d,%u,%.2f,%d,%" PRIu32 ",%" PRIu32 ",%" PRIu32 ",%d,%d,%d,%d\n",
        s->boot_id, s->sequence, s->uptime_ms, (double)s->inside_temp_c,
        (double)s->inside_rh_pct, (double)s->outside_temp_c,
        s->wick_raw, s->light_raw, s->water_raw, s->valid_mask, (double)s->sound_rms, s->sound_p2p,
        s->sound_samples, s->sound_clipped, s->adc_overflows,
        s->distance_mm, s->animal_present, s->presence_valid, s->audio_recording);
    return n < 0 || (size_t)n >= capacity ? -1 : n;
}

static bool parse_uint(const char *s, uint64_t max, uint64_t *value)
{
    if (!*s) return false;
    for (const char *p = s; *p; ++p) if (*p < '0' || *p > '9') return false;
    errno = 0;
    char *end;
    unsigned long long n = strtoull(s, &end, 10);
    if (errno || *end || n > max) return false;
    *value = (uint64_t)n;
    return true;
}

static bool parse_float(const char *s, float *value)
{
    if (strcmp(s, "nan") == 0) { *value = NAN; return true; }
    if (!*s) return false;
    /* Reject whitespace, CSV formulas and non-numeric payloads. */
    for (const char *p = s; *p; ++p)
        if (!((*p >= '0' && *p <= '9') || *p == '-' || *p == '+' ||
              *p == '.' || *p == 'e' || *p == 'E')) return false;
    errno = 0;
    char *end;
    float f = strtof(s, &end);
    if (errno || *end || end == s || !isfinite(f)) return false;
    *value = f;
    return true;
}

static bool parse_adc(const char *s, int *value)
{
    if (strcmp(s, "-1") == 0) { *value = -1; return true; }
    uint64_t v;
    if (!parse_uint(s, 4095, &v)) return false;
    *value = (int)v;
    return true;
}

bool hh_parse_sample(const char *line, hh_sample_t *result)
{
    size_t n = strlen(line);
    if (!n || n >= HH_LINE_CAP) return false;
    char buf[HH_LINE_CAP];
    memcpy(buf, line, n + 1);
    while (n && (buf[n - 1] == '\r' || buf[n - 1] == '\n')) buf[--n] = '\0';
    char *fields[20];
    size_t count = 1;
    fields[0] = buf;
    for (char *p = buf; *p; ++p) {
        if (*p == ',') {
            if (count == 20) return false;
            *p = '\0';
            fields[count++] = p + 1;
        }
    }
    if (count != 20 || strcmp(fields[0], "3") != 0) return false;
    hh_sample_t s;
    hh_sample_clear(&s);
    uint64_t boot, seq, uptime, mask, samples, clipped, overflows, present, valid, recording;
    if (!parse_uint(fields[1], UINT32_MAX, &boot)
        || !parse_uint(fields[2], UINT32_MAX, &seq)
        || !parse_uint(fields[3], UINT64_MAX, &uptime)
        || !parse_float(fields[4], &s.inside_temp_c)
        || !parse_float(fields[5], &s.inside_rh_pct)
        || !parse_float(fields[6], &s.outside_temp_c)
        || !parse_adc(fields[7], &s.wick_raw)
        || !parse_adc(fields[8], &s.light_raw)
        || !parse_adc(fields[9], &s.water_raw)
        || !parse_uint(fields[10], HH_VALID_ALL, &mask)
        || !parse_float(fields[11], &s.sound_rms)
        || !parse_adc(fields[12], &s.sound_p2p)
        || !parse_uint(fields[13], UINT32_MAX, &samples)
        || !parse_uint(fields[14], UINT32_MAX, &clipped)
        || !parse_uint(fields[15], UINT32_MAX, &overflows)
        || !parse_adc(fields[16], &s.distance_mm)
        || !parse_uint(fields[17], 1, &present)
        || !parse_uint(fields[18], 1, &valid)
        || !parse_uint(fields[19], 1, &recording)) return false;
    s.animal_present=present!=0; s.presence_valid=valid!=0; s.audio_recording=recording!=0;
    s.sound_samples = (uint32_t)samples;
    s.sound_clipped = (uint32_t)clipped;
    s.adc_overflows = (uint32_t)overflows;
    s.boot_id = (uint32_t)boot;
    s.sequence = (uint32_t)seq;
    s.uptime_ms = uptime;
    s.valid_mask = (unsigned)mask;
    if (!hh_sample_valid(&s)) return false;
    *result = s;
    return true;
}
