#include <math.h>
#include <stdio.h>
#include <string.h>
#include "telemetry.h"

#define CHECK(test) do { if (!(test)) { fprintf(stderr, "Failed line %d: %s\n", __LINE__, #test); return 1; } } while (0)

int main(void)
{
    hh_sample_t s, decoded;
    hh_sample_clear(&s);
    s.boot_id = UINT32_MAX;
    s.sequence = UINT32_MAX;
    s.uptime_ms = UINT64_MAX;
    char line[HH_LINE_CAP];
    CHECK(hh_format_sample(line, sizeof(line), &s) > 0);
    CHECK(hh_parse_sample(line, &decoded));
    CHECK(decoded.boot_id == UINT32_MAX && decoded.uptime_ms == UINT64_MAX);
    CHECK(isnan(decoded.inside_temp_c) && decoded.wick_raw == -1);
    s.valid_mask = HH_VALID_ALL;
    s.distance_mm=100; s.presence_valid=true; s.animal_present=true; s.audio_recording=true;
    s.inside_temp_c = 32.5f; s.inside_rh_pct = 62.25f; s.outside_temp_c = -5.0f;
    s.sound_rms = 12.5f; s.sound_p2p = 64; s.sound_samples = 10000; s.sound_clipped = 2;
    s.wick_raw = 2000; s.light_raw = 4000; s.water_raw = 1400;
    CHECK(hh_format_sample(line, sizeof(line), &s) > 0);
    CHECK(hh_parse_sample(line, &decoded));
    CHECK(decoded.inside_temp_c == 32.5f && decoded.water_raw == 1400);
    CHECK(decoded.sound_rms == 12.5f && decoded.sound_samples == 10000 && decoded.sound_clipped == 2);
    CHECK(decoded.animal_present && decoded.presence_valid && decoded.distance_mm==100 && decoded.audio_recording);
    s.distance_mm=3000; CHECK(!hh_sample_valid(&s)); s.distance_mm=100;
    s.presence_valid=false; CHECK(!hh_sample_valid(&s)); s.presence_valid=true;
    s.sound_clipped = 10001; CHECK(!hh_sample_valid(&s)); s.sound_clipped = 2;
    s.sound_samples = 0; CHECK(!hh_sample_valid(&s)); s.sound_samples = 10000;
    CHECK(hh_format_sample(line, 5, &s) == -1);
    const char *invalid[] = {
        "3,1,1,1,30,40,20,1,2,3,31,extra",
        "3,1,1,1,30,40,20,1,2,3",
        "1,1,1,1,30,40,20,1,2,3,63,12.50,64,10000,2,0",
        "3,1,1,1,30,40,20,1,2,3,63,nan,64,10000,2,0",
        "3,1,1,1,30,40,20,1,2,3,63,12.50,4096,10000,2,0",
        "3,1,1,1,30,40,20,1,2,3,63,12.50,64,10000,10001,0",
        "3,4294967296,1,1,30,40,20,1,2,3,63,12.50,64,10000,2,0",
        "3,1,1,18446744073709551616,30,40,20,1,2,3,63,12.50,64,10000,2,0",
        "3,-1,1,1,30,40,20,1,2,3,63,12.50,64,10000,2,0",
        "3,1,1,1,nan,40,20,1,2,3,63,12.50,64,10000,2,0",
        "3,1,1,1,30,101,20,1,2,3,63,12.50,64,10000,2,0",
        "3,1,1,1,30,40,20,4096,2,3,63,12.50,64,10000,2,0",
        "3,1,1,1,30,40,20,1,2,3,64,12.50,64,10000,2,0",
        "3,1,1,1,30,40,20,1,2,3,0,12.50,64,10000,2,0",
        "3,1,1,1,=SUM(1),40,20,1,2,3,63,12.50,64,10000,2,0",
        "3,1,1,1,inf,40,20,1,2,3,63,12.50,64,10000,2,0",
        "3,1,1,1,,40,20,1,2,3,63,12.50,64,10000,2,0",
    };
    for (size_t i = 0; i < sizeof(invalid) / sizeof(invalid[0]); ++i)
        CHECK(!hh_parse_sample(invalid[i], &decoded));
    CHECK(hh_parse_sample("3,1,1,1,nan,nan,nan,-1,-1,-1,0,nan,-1,0,0,0,-1,0,0,0\r\n", &decoded));
    puts("Telemetry serialization/validation tests passed.");
    return 0;
}
