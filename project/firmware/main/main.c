#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "project_config.h"
#include "sensors.h"
#include "audio_record.h"
#include "telemetry.h"

static const char *TAG = "hollowhaven";
static SemaphoreHandle_t sample_mutex;
static hh_sample_t latest;
static bool have_sample;
static uint32_t boot_id;

_Static_assert(sizeof(HH_AP_SSID) - 1 <= 32, "SSID is too long");
_Static_assert(sizeof(HH_AP_PASSWORD) - 1 >= 8 && sizeof(HH_AP_PASSWORD) - 1 <= 63,
               "WPA2 password must contain 8..63 characters");

static void start_access_point(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *ap = esp_netif_create_default_wifi_ap();
    assert(ap != NULL);
    ESP_ERROR_CHECK(esp_netif_dhcps_stop(ap));
    esp_netif_ip_info_t ip = {0};
    IP4_ADDR(&ip.ip, 192, 168, 4, 1);
    IP4_ADDR(&ip.gw, 192, 168, 4, 1);
    IP4_ADDR(&ip.netmask, 255, 255, 255, 0);
    ESP_ERROR_CHECK(esp_netif_set_ip_info(ap, &ip));
    ESP_ERROR_CHECK(esp_netif_dhcps_start(ap));
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));
    wifi_config_t config = {
        .ap = {
            .ssid = HH_AP_SSID,
            .ssid_len = sizeof(HH_AP_SSID) - 1,
            .password = HH_AP_PASSWORD,
            .channel = HH_AP_CHANNEL,
            .max_connection = 1,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {.required = false},
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &config));
    ESP_ERROR_CHECK(esp_wifi_start());
    /* Generate after Wi-Fi is started, when hardware entropy is available. */
    boot_id = esp_random();
    ESP_LOGI(TAG, "Connect your PC to '%s'; TCP 192.168.4.1:%d", HH_AP_SSID, HH_TCP_PORT);
}

static void acquisition_task(void *argument)
{
    (void)argument;
    uint32_t sequence = 0;
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        hh_sample_t s;
        hh_sensors_read(&s);
        s.boot_id = boot_id;
        s.sequence = ++sequence;
        /* Completion time, not a claim that all sensors were read simultaneously. */
        s.uptime_ms = (uint64_t)(esp_timer_get_time() / 1000);
        xSemaphoreTake(sample_mutex, portMAX_DELAY);
        latest = s;
        have_sample = true;
        xSemaphoreGive(sample_mutex);
        ESP_LOGI(TAG, "seq=%" PRIu32 " T=%.2f RH=%.2f outside=%.2f wick=%d light=%d water=%d valid=%u sound_rms=%.2f p2p=%d n=%" PRIu32 " clip=%" PRIu32 " adc_gaps=%" PRIu32 " distance=%d present=%d presence_valid=%d recording=%d",
                 s.sequence, (double)s.inside_temp_c, (double)s.inside_rh_pct,
                 (double)s.outside_temp_c, s.wick_raw, s.light_raw, s.water_raw, s.valid_mask, (double)s.sound_rms, s.sound_p2p,
                 s.sound_samples, s.sound_clipped, s.adc_overflows, s.distance_mm, s.animal_present, s.presence_valid, s.audio_recording);
        vTaskDelayUntil(&last, pdMS_TO_TICKS(HH_SAMPLE_PERIOD_MS));
    }
}

static bool send_all(int sock, const char *data, size_t length)
{
    while (length) {
        int n = send(sock, data, length, 0);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return false;
        data += n;
        length -= (size_t)n;
    }
    return true;
}

static void serve_client(int client)
{
    struct timeval timeout = {.tv_sec = 5, .tv_usec = 0};
    if (setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) return;
    int enabled = 1, idle = 5, interval = 2, probes = 3;
    setsockopt(client, SOL_SOCKET, SO_KEEPALIVE, &enabled, sizeof(enabled));
    setsockopt(client, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));
    setsockopt(client, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    setsockopt(client, IPPROTO_TCP, TCP_KEEPCNT, &probes, sizeof(probes));
    if (!send_all(client, HH_GREETING "\n", sizeof(HH_GREETING "\n") - 1)) return;
    bool sent_any = false;
    uint32_t previous = 0;
    for (;;) {
        /* Detect a clean peer close promptly; this protocol accepts no commands. */
        fd_set readable;
        FD_ZERO(&readable);
        FD_SET(client, &readable);
        struct timeval poll = {0};
        int result = select(client + 1, &readable, NULL, NULL, &poll);
        if (result < 0) break;
        if (result > 0) break;
        hh_sample_t s;
        xSemaphoreTake(sample_mutex, portMAX_DELAY);
        bool ready = have_sample;
        s = latest;
        xSemaphoreGive(sample_mutex);
        if (ready && (!sent_any || s.sequence != previous)) {
            char line[HH_LINE_CAP];
            int n = hh_format_sample(line, sizeof(line), &s);
            if (n < 0 || !send_all(client, line, (size_t)n)) break;
            previous = s.sequence;
            sent_any = true;
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void tcp_task(void *argument)
{
    (void)argument;
    for (;;) {
        int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        if (listener < 0) { vTaskDelay(pdMS_TO_TICKS(2000)); continue; }
        int reuse = 1;
        setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        struct sockaddr_in address = {
            .sin_family = AF_INET,
            .sin_port = htons(HH_TCP_PORT),
            .sin_addr.s_addr = htonl(INADDR_ANY),
        };
        if (bind(listener, (struct sockaddr *)&address, sizeof(address)) < 0 || listen(listener, 1) < 0) {
            ESP_LOGE(TAG, "TCP bind/listen failed: %d", errno);
            close(listener);
            vTaskDelay(pdMS_TO_TICKS(2000));
            continue;
        }
        ESP_LOGI(TAG, "TCP server listening");
        for (;;) {
            int client = accept(listener, NULL, NULL);
            if (client < 0) { if (errno == EINTR) continue; break; }
            ESP_LOGI(TAG, "Receiver connected");
            serve_client(client);
            shutdown(client, SHUT_RDWR);
            close(client);
            ESP_LOGI(TAG, "Receiver disconnected; awaiting reconnect");
        }
        close(listener);
    }
}

void app_main(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    sample_mutex = xSemaphoreCreateMutex();
    assert(sample_mutex != NULL);
    start_access_point();
    hh_audio_init(boot_id);
    hh_sensors_init();
    BaseType_t status = xTaskCreate(acquisition_task, "sensors", 4096, NULL, 5, NULL);
    assert(status == pdPASS);
    status = xTaskCreate(tcp_task, "telemetry_tcp", 4096, NULL, 4, NULL);
    assert(status == pdPASS);
}
