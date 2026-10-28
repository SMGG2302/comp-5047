#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <math.h>
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
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "lwip/sockets.h"
#include "project_config.h"
#include "sensors.h"
#include "audio_record.h"
#include "telemetry.h"
#include "servo.h"
#include "moisture.h"
#include "maintenance.h"
#include "wifi_credentials.h"

static const char *TAG = "hollowhaven";
static SemaphoreHandle_t sample_mutex;
static hh_sample_t latest;
static bool have_sample;
static uint32_t boot_id;

_Static_assert(sizeof(HH_WIFI_SSID) - 1 <= 32, "SSID is too long");
_Static_assert(sizeof(HH_WIFI_PASSWORD) - 1 >= 8 && sizeof(HH_WIFI_PASSWORD) - 1 <= 63,
               "WPA2 password must contain 8..63 characters");

static void wifi_event(void *arg,esp_event_base_t base,int32_t id,void *data)
{
    (void)arg;
    if (base==WIFI_EVENT && id==WIFI_EVENT_STA_START) esp_wifi_connect();
    else if (base==WIFI_EVENT && id==WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t *event=data;
        ESP_LOGW(TAG,"Wi-Fi disconnected reason=%u; reconnecting",event->reason);
        esp_wifi_connect();
    } else if (base==IP_EVENT && id==IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event=data;
        ESP_LOGI(TAG,"Wi-Fi connected: IP=" IPSTR " TCP=%d",IP2STR(&event->ip_info.ip),HH_TCP_PORT);
    }
}

/* LAN discovery replies only to the local receiver's explicit probe. */
static void discovery_task(void *arg)
{
    (void)arg;
    for (;;) {
        int sock=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
        if (sock<0) { vTaskDelay(pdMS_TO_TICKS(2000)); continue; }
        struct sockaddr_in address={.sin_family=AF_INET,.sin_port=htons(9002),.sin_addr.s_addr=htonl(INADDR_ANY)};
        if (bind(sock,(struct sockaddr *)&address,sizeof(address))==0) {
            for (;;) {
                char buffer[64]; struct sockaddr_in peer; socklen_t size=sizeof(peer);
                int n=recvfrom(sock,buffer,sizeof(buffer),0,(struct sockaddr *)&peer,&size);
                if (n<0) break;
                if (n==13 && memcmp(buffer,"HH_DISCOVER/4",13)==0)
                    sendto(sock,"HH_DEVICE/4",11,0,(struct sockaddr *)&peer,size);
            }
        }
        close(sock);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

static void start_station(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *station = esp_netif_create_default_wifi_sta();
    assert(station != NULL);
    ESP_ERROR_CHECK(esp_netif_set_hostname(station,"hollowhaven"));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT,ESP_EVENT_ANY_ID,wifi_event,NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT,IP_EVENT_STA_GOT_IP,wifi_event,NULL));
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));
    wifi_config_t config = {
        .sta = {
            .ssid = HH_WIFI_SSID,
            .password = HH_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {.capable=true,.required = false},
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &config));
    ESP_ERROR_CHECK(esp_wifi_start());
    /* Generate after Wi-Fi is started, when hardware entropy is available. */
    boot_id = esp_random();
    ESP_LOGI(TAG,"Joining Wi-Fi '%s'; waiting for DHCP",HH_WIFI_SSID);
    assert(xTaskCreate(discovery_task,"discovery",3072,NULL,3,NULL)==pdPASS);
}

static void acquisition_task(void *argument)
{
    (void)argument;
    uint32_t sequence = 0;
    bool valve_open=false;
    hh_moisture_t wet1=HH_MOISTURE_UNKNOWN,wet2=HH_MOISTURE_UNKNOWN;
    TickType_t last = xTaskGetTickCount();
    for (;;) {
        hh_sample_t s;
        hh_sensors_read(&s);
        bool light_valid=(s.valid_mask & HH_VALID_LIGHT)!=0 && s.light_raw>=0;
        bool request_open=light_valid && s.light_raw<HH_VALVE_OPEN_LIGHT_RAW;
        if (request_open!=valve_open) {
            esp_err_t valve_err=hh_servo_set_valve(request_open);
            if (valve_err==ESP_OK) valve_open=request_open;
            else ESP_LOGE(TAG,"Valve request failed: %s",esp_err_to_name(valve_err));
        }
        ESP_LOGI(TAG,"valve_target=%s light_valid=%d T=%.2f light_raw=%d water_raw=%d",
            valve_open?"OPEN":"CLOSED",light_valid,(double)s.inside_temp_c,
            s.light_raw,s.water_raw);
        wet1=hh_moisture_update(wet1,(s.valid_mask & HH_VALID_WICK)!=0,s.wick_raw);
        wet2=hh_moisture_update(wet2,s.wick2_valid,s.wick2_raw);
        s.moisture1_state=wet1; s.moisture2_state=wet2;
        hh_servo_snapshot(&s);
        s.outlet_blocked=hh_outlet_blocked(s.valve_state,s.valve_open_ms,wet1,wet2);
        ESP_LOGI(TAG,"moisture1=%s moisture2=%s",wet1==HH_MOISTURE_WET?"wet":wet1==HH_MOISTURE_DRY?"dry":"unknown",wet2==HH_MOISTURE_WET?"wet":wet2==HH_MOISTURE_DRY?"dry":"unknown");
        s.boot_id = boot_id;
        s.sequence = ++sequence;
        /* Completion time, not a claim that all sensors were read simultaneously. */
        s.uptime_ms = (uint64_t)(esp_timer_get_time() / 1000);
        xSemaphoreTake(sample_mutex, portMAX_DELAY);
        latest = s;
        have_sample = true;
        xSemaphoreGive(sample_mutex);
        ESP_LOGI(TAG, "seq=%" PRIu32 " T=%.2f RH=%.2f outside=%.2f wick=%d wick2=%d wick2_valid=%d light=%d water=%d valid=%u sound_rms=%.2f p2p=%d n=%" PRIu32 " clip=%" PRIu32 " adc_gaps=%" PRIu32 " distance=%d present=%d presence_valid=%d recording=%d",
                 s.sequence, (double)s.inside_temp_c, (double)s.inside_rh_pct,
                 (double)s.outside_temp_c, s.wick_raw, s.wick2_raw, s.wick2_valid, s.light_raw, s.water_raw, s.valid_mask, (double)s.sound_rms, s.sound_p2p,
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
    esp_reset_reason_t reason=esp_reset_reason();
    ESP_LOGI(TAG,"Reset reason=%d%s; servo GPIO%d; light-only valve test",(int)reason,
        reason==ESP_RST_BROWNOUT?" (BROWNOUT: supply voltage dropped)":"",HH_SERVO_GPIO);
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    sample_mutex = xSemaphoreCreateMutex();
    assert(sample_mutex != NULL);
    start_station();
    hh_audio_init(boot_id);
    hh_sensors_init();
    ESP_ERROR_CHECK(hh_servo_init());
    BaseType_t status = xTaskCreate(acquisition_task, "sensors", 4096, NULL, 5, NULL);
    assert(status == pdPASS);
    status = xTaskCreate(tcp_task, "telemetry_tcp", 4096, NULL, 4, NULL);
    assert(status == pdPASS);
}
