#include "servo.h"
#include "project_config.h"
#include "driver/ledc.h"
#include <stdbool.h>
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
static bool ready;
static TaskHandle_t servo_task;
static portMUX_TYPE status_lock=portMUX_INITIALIZER_UNLOCKED;
static unsigned current_pulse, current_target;
static int64_t opened_at;
static void publish_position(unsigned pulse,unsigned target)
{
    int64_t now=esp_timer_get_time();
    portENTER_CRITICAL(&status_lock);
    current_pulse=pulse; current_target=target;
    if (pulse==HH_VALVE_OPEN_US && target==HH_VALVE_OPEN_US) {
        if (!opened_at) opened_at=now;
    } else opened_at=0;
    portEXIT_CRITICAL(&status_lock);
}

void hh_servo_snapshot(hh_sample_t *s)
{
    int64_t now=esp_timer_get_time();
    portENTER_CRITICAL(&status_lock);
    unsigned pulse=current_pulse,target=current_target;
    int64_t since=opened_at;
    portEXIT_CRITICAL(&status_lock);
    if (!ready || !HH_ENABLE_SERVO) return;
    s->valve_pulse_us=pulse;
    s->valve_state=pulse==target ? (pulse==HH_VALVE_OPEN_US ? HH_VALVE_OPEN : HH_VALVE_CLOSED)
        : (target<pulse ? HH_VALVE_OPENING : HH_VALVE_CLOSING);
    uint64_t elapsed=since && now>since ? (uint64_t)(now-since)/1000 : 0;
    s->valve_open_ms=(uint32_t)(elapsed>UINT32_MAX ? UINT32_MAX : elapsed);
}
static esp_err_t set_pulse_us(unsigned pulse)
{
    if (!ready) return ESP_ERR_INVALID_STATE;
    /* A 50 Hz PWM pulse must be shorter than its 20000 us period. */
    if (pulse>=20000u) return ESP_ERR_INVALID_ARG;
    unsigned duty=(pulse*65536u+10000u)/20000u;
    esp_err_t err=ledc_set_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0,duty);
    if (err!=ESP_OK) return err;
    return ledc_update_duty(LEDC_LOW_SPEED_MODE,LEDC_CHANNEL_0);
}
static void ramp_task(void *arg)
{
    (void)arg;
    unsigned pulse=HH_VALVE_CLOSED_US;
    uint32_t target=HH_VALVE_CLOSED_US;
    for (;;) {
        uint32_t requested;
        if (xTaskNotifyWait(0,UINT32_MAX,&requested,
                pdMS_TO_TICKS(HH_SERVO_RAMP_INTERVAL_MS))==pdTRUE) {
            target=requested;
            publish_position(pulse,target);
            ESP_LOGI("servo","Valve target=%lu us",(unsigned long)target);
        }
        if (pulse==target) continue;
        unsigned remaining=pulse<target ? target-pulse : pulse-target;
        unsigned step=remaining<HH_SERVO_RAMP_STEP_US ? remaining : HH_SERVO_RAMP_STEP_US;
        if (pulse<target) pulse+=step;
        else pulse-=step;
        ESP_ERROR_CHECK(set_pulse_us(pulse));
        publish_position(pulse,target);
        if (pulse==target)
            ESP_LOGI("servo","Valve reached commanded pulse=%u us",pulse);
    }
}
esp_err_t hh_servo_init(void)
{
    if (!HH_ENABLE_SERVO) return ESP_OK;
    ledc_timer_config_t timer = {.speed_mode=LEDC_LOW_SPEED_MODE,
        .duty_resolution=LEDC_TIMER_16_BIT,.timer_num=LEDC_TIMER_0,
        .freq_hz=50,.clk_cfg=LEDC_AUTO_CLK};
    esp_err_t err=ledc_timer_config(&timer);
    if (err!=ESP_OK) return err;
    ledc_channel_config_t channel={.gpio_num=HH_SERVO_GPIO,
        .speed_mode=LEDC_LOW_SPEED_MODE,.channel=LEDC_CHANNEL_0,
        .timer_sel=LEDC_TIMER_0,.duty=0,.hpoint=0};
    err=ledc_channel_config(&channel);
    if (err!=ESP_OK) return err;
    ready=true;
    err=set_pulse_us(HH_VALVE_CLOSED_US);
    if (err!=ESP_OK) return err;
    publish_position(HH_VALVE_CLOSED_US,HH_VALVE_CLOSED_US);
    if (xTaskCreate(ramp_task,"servo_valve",2048,NULL,2,&servo_task)!=pdPASS)
        return ESP_ERR_NO_MEM;
    ESP_LOGI("servo","Valve initialized CLOSED, pulse=%u us",(unsigned)HH_VALVE_CLOSED_US);
    return ESP_OK;
}
esp_err_t hh_servo_set_angle(unsigned degrees)
{
    if (!ready) return ESP_ERR_INVALID_STATE;
    if (degrees>180) return ESP_ERR_INVALID_ARG;
    unsigned pulse=HH_SERVO_MIN_US+(HH_SERVO_MAX_US-HH_SERVO_MIN_US)*degrees/180;
    if (!servo_task) return ESP_ERR_INVALID_STATE;
    return xTaskNotify(servo_task,pulse,eSetValueWithOverwrite)==pdPASS ? ESP_OK : ESP_FAIL;
}

esp_err_t hh_servo_set_valve(bool open)
{
    if (!HH_ENABLE_SERVO) return ESP_OK;
    if (!ready || !servo_task) return ESP_ERR_INVALID_STATE;
    unsigned pulse=open ? HH_VALVE_OPEN_US : HH_VALVE_CLOSED_US;
    return xTaskNotify(servo_task,pulse,eSetValueWithOverwrite)==pdPASS ? ESP_OK : ESP_FAIL;
}
