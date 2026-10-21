#include "analog.h"
#include "audio_record.h"
#include "esp_timer.h"
#include "project_config.h"
#include <assert.h>
#include <math.h>
#include <string.h>
#include "esp_adc/adc_continuous.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "soc/soc_caps.h"

/* Scan only enabled ADC1 inputs. With wick + microphone, audio is nominally
 * 10 kHz (actual rate measured when each clip is finalized). */
#if HH_WATER_SWITCH_ENABLED
#error "Continuous ADC version requires water switch disabled; GPIO33 is microphone AO"
#endif
static adc_continuous_handle_t adc;
static SemaphoreHandle_t lock;
static int channels[4];
typedef struct {
    uint64_t sum[4], sound_squares;
    uint32_t count[4], clipped;
    unsigned low, high;
} window_t;
static window_t window;
static uint32_t overflows;

static void clear_window(window_t *w)
{
    memset(w, 0, sizeof(*w));
    w->low = 4095;
}

static void adc_task(void *arg)
{
    (void)arg;
    uint8_t buffer[1024] __attribute__((aligned(4)));
    int32_t dc = 0;
    bool dc_ready = false;
    int16_t pcm[512];
    for (;;) {
        unsigned pcm_count = 0;
        uint32_t length = 0;
        esp_err_t error = adc_continuous_read(adc, buffer, sizeof(buffer), &length, 100);
        if (error == ESP_ERR_TIMEOUT) continue;
        if (error != ESP_OK) {
            hh_audio_gap();
            xSemaphoreTake(lock, portMAX_DELAY);
            ++overflows;
            clear_window(&window);
            xSemaphoreGive(lock);
            ESP_LOGW("audio_adc", "ADC gap: %s; discarding incomplete window", esp_err_to_name(error));
            ESP_ERROR_CHECK(adc_continuous_stop(adc));
            ESP_ERROR_CHECK(adc_continuous_flush_pool(adc));
            ESP_ERROR_CHECK(adc_continuous_start(adc));
            continue;
        }
        xSemaphoreTake(lock, portMAX_DELAY);
        for (uint32_t i = 0; i + SOC_ADC_DIGI_RESULT_BYTES <= length; i += SOC_ADC_DIGI_RESULT_BYTES) {
            const adc_digi_output_data_t *r = (const void *)(buffer + i);
            unsigned channel = r->type1.channel, value = r->type1.data;
            for (int j = 0; j < 4; ++j) {
                if (channel != (unsigned)channels[j]) continue;
                window.sum[j] += value;
                ++window.count[j];
                if (j == 3) {
                    if (!dc_ready) { dc = (int32_t)value * 256; dc_ready = true; }
                    dc += ((int32_t)value * 256 - dc) / 256;
                    int32_t audio = ((int32_t)value - dc / 256) * 16;
                    if (audio > 32767) audio = 32767;
                    if (audio < -32768) audio = -32768;
                    pcm[pcm_count++] = (int16_t)audio;
                    window.sound_squares += (uint64_t)value * value;
                    if (value < window.low) window.low = value;
                    if (value > window.high) window.high = value;
                    if (value <= 5 || value >= 4090) ++window.clipped;
                }
                break;
            }
        }
        xSemaphoreGive(lock);
        hh_audio_push(pcm, pcm_count, esp_timer_get_time());
    }
}

void hh_analog_init(void)
{
    const int pins[4] = {HH_WICK_GPIO, HH_LIGHT_GPIO, HH_WATER_GPIO, HH_SOUND_GPIO};
    adc_digi_pattern_config_t pattern[4] = {0};
    const bool enabled[4]={HH_ENABLE_WICK,HH_ENABLE_LIGHT,HH_ENABLE_WATER,HH_ENABLE_SOUND};
    unsigned count=0;
    for (int j = 0; j < 4; ++j) {
        channels[j]=-1;
        if(!enabled[j])continue;
        adc_unit_t unit;
        adc_channel_t channel;
        ESP_ERROR_CHECK(adc_continuous_io_to_channel(pins[j], &unit, &channel));
        assert(unit == ADC_UNIT_1);
        channels[j] = channel;
        pattern[count].atten = ADC_ATTEN_DB_12;
        pattern[count].channel = channel;
        pattern[count].unit = unit;
        pattern[count].bit_width = SOC_ADC_DIGI_MAX_BITWIDTH;
        ++count;
    }
    lock = xSemaphoreCreateMutex();
    assert(lock);
    clear_window(&window);
    adc_continuous_handle_cfg_t handle = {.max_store_buf_size = 16384, .conv_frame_size = 1024};
    ESP_ERROR_CHECK(adc_continuous_new_handle(&handle, &adc));
    adc_continuous_config_t config = {
        .sample_freq_hz = 20000, .conv_mode = ADC_CONV_SINGLE_UNIT_1,
        .format = ADC_DIGI_OUTPUT_FORMAT_TYPE1, .pattern_num = count, .adc_pattern = pattern,
    };
    ESP_ERROR_CHECK(adc_continuous_config(adc, &config));
    ESP_ERROR_CHECK(adc_continuous_start(adc));
    BaseType_t result = xTaskCreate(adc_task, "audio_adc", 6144, NULL, 6, NULL);
    assert(result == pdPASS);
}

void hh_analog_snapshot(hh_sample_t *s)
{
    window_t w;
    xSemaphoreTake(lock, portMAX_DELAY);
    w = window;
    clear_window(&window);
    s->adc_overflows = overflows;
    xSemaphoreGive(lock);
    int *outputs[] = {&s->wick_raw, &s->light_raw, &s->water_raw};
    const bool enabled[] = {HH_ENABLE_WICK, HH_ENABLE_LIGHT, HH_ENABLE_WATER};
    const unsigned flags[] = {HH_VALID_WICK, HH_VALID_LIGHT, HH_VALID_WATER};
    for (int j = 0; j < 3; ++j) {
        if (enabled[j] && w.count[j]) {
            *outputs[j] = (int)((w.sum[j] + w.count[j] / 2) / w.count[j]);
            s->valid_mask |= flags[j];
        }
    }
    if (HH_ENABLE_SOUND && w.count[3]) {
        double mean = (double)w.sum[3] / w.count[3];
        double variance = (double)w.sound_squares / w.count[3] - mean * mean;
        s->sound_rms = (float)sqrt(variance > 0 ? variance : 0);
        s->sound_p2p = (int)(w.high - w.low);
        s->sound_samples = w.count[3];
        s->sound_clipped = w.clipped;
        s->valid_mask |= HH_VALID_SOUND;
    }
}
