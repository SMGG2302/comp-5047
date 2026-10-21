#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#define HH_AUDIO_HEADER 32u
#define HH_AUDIO_MAX_BYTES 640000u
static inline uint32_t hh_le32(const uint8_t *p)
{ return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24; }
static inline void hh_put32(uint8_t *p, uint32_t v)
{ p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8); p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24); }
static inline uint32_t hh_crc(uint32_t crc, const void *data, size_t n)
{
    const uint8_t *p = data;
    while (n--) {
        crc ^= *p++;
        for (int b=0;b<8;++b) crc=(crc>>1)^((0u-(crc&1u))&0xedb88320u);
    }
    return crc;
}
/* Header: HHW1, boot, sequence, sample_count, sample_rate, pcm_crc32,
 * gap_events, duration_ms. All integers little endian, PCM signed 16 bit LE. */
