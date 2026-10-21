#include "presence_sensor.h"
#include "presence.h"
#include "project_config.h"
#include "audio_record.h"
#include "vl53l0x_api.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <assert.h>
#include <string.h>

static i2c_master_bus_handle_t bus;
static VL53L0X_Dev_t dev;
static SemaphoreHandle_t lock;
static hh_presence_t state;
static int distance_mm=-1;
static int64_t updated_us;
static unsigned connect_attempts;

static bool connect_sensor(void)
{
    if (dev.i2c) { i2c_master_bus_rm_device(dev.i2c); }
    memset(&dev,0,sizeof(dev));
    esp_err_t probe=i2c_master_probe(bus,0x29,50);
    if (probe!=ESP_OK) {
        if(connect_attempts++%10==0)
            ESP_LOGW("presence","No ACK at 0x29: %s; SDA27=%d SCL26=%d",
                esp_err_to_name(probe),gpio_get_level(HH_TOF_SDA_GPIO),gpio_get_level(HH_TOF_SCL_GPIO));
        return false;
    }
    connect_attempts=0;
    i2c_device_config_t cfg={.dev_addr_length=I2C_ADDR_BIT_LEN_7,
        .device_address=0x29,.scl_speed_hz=100000};
    if(i2c_master_bus_add_device(bus,&cfg,&dev.i2c)!=ESP_OK)return false;
    dev.I2cDevAddr=0x52; dev.comms_speed_khz=100;
    uint8_t id=0, vhv=0, phase=0, aperture=0; uint32_t spads=0;
    int error=VL53L0X_RdByte(&dev,0xc0,&id);
    if(error || id!=0xee) {
        ESP_LOGW("presence","Not a VL53L0X: model=0x%02x error=%d",id,error);
        return false;
    }
    error=VL53L0X_DataInit(&dev);
    if(!error)error=VL53L0X_StaticInit(&dev);
    if(!error)error=VL53L0X_PerformRefCalibration(&dev,&vhv,&phase);
    if(!error)error=VL53L0X_PerformRefSpadManagement(&dev,&spads,&aperture);
    if(!error)error=VL53L0X_SetDeviceMode(&dev,VL53L0X_DEVICEMODE_CONTINUOUS_RANGING);
    if(!error)error=VL53L0X_SetMeasurementTimingBudgetMicroSeconds(&dev,33000);
    if(!error)error=VL53L0X_StartMeasurement(&dev);
    ESP_LOGI("presence","VL53L0X init result=%d, SDA=%d SCL=%d",error,HH_TOF_SDA_GPIO,HH_TOF_SCL_GPIO);
    return error==0;
}

static void task(void *arg)
{
    (void)arg;
    hh_presence_t local={0};
    bool connected=false, pending_entry=false;
    int64_t last_frame=0, retry_at=0, report_at=0;
    for(;;) {
        int64_t now=esp_timer_get_time();
        if(!connected && now>=retry_at) {
            connected=connect_sensor();
            last_frame=esp_timer_get_time(); retry_at=last_frame+2000000;
        }
        bool valid=false; int mm=-1;
        if(connected) {
            uint8_t ready=0;
            int error=VL53L0X_GetMeasurementDataReady(&dev,&ready);
            if(!error && ready) {
                VL53L0X_RangingMeasurementData_t data={0};
                error=VL53L0X_GetRangingMeasurementData(&dev,&data);
                if(!error)error=VL53L0X_ClearInterruptMask(&dev,0);
                if(!error) {
                    last_frame=esp_timer_get_time();
                    valid=data.RangeStatus==0 && data.RangeMilliMeter>0 && data.RangeMilliMeter<=2000;
                    if(valid)mm=data.RangeMilliMeter;
                    if(!valid && now>=report_at) {
                        ESP_LOGW("presence","Range invalid: status=%u raw_mm=%u",
                            (unsigned)data.RangeStatus,(unsigned)data.RangeMilliMeter);
                        report_at=now+5000000;
                    }
                }
            }
            if(error || esp_timer_get_time()-last_frame>1000000) {
                ESP_LOGW("presence","Range read failed/stalled: error=%d",error);
                connected=false; retry_at=esp_timer_get_time()+2000000;
            }
        }
        bool previous=local.animal_present;
        if(hh_presence_update(&local,valid,mm,HH_ENTER_MM,HH_EXIT_MM,
                              HH_ENTER_READINGS,HH_EXIT_READINGS))pending_entry=true;
        if(!local.animal_present)pending_entry=false;
        if(valid && pending_entry && hh_audio_trigger())pending_entry=false;
        if(previous!=local.animal_present)
            ESP_LOGI("presence","animal_present=%s distance=%d mm",local.animal_present?"true":"false",mm);
        xSemaphoreTake(lock,portMAX_DELAY);
        state=local; distance_mm=mm; updated_us=esp_timer_get_time();
        xSemaphoreGive(lock);
        vTaskDelay(pdMS_TO_TICKS(HH_TOF_PERIOD_MS));
    }
}

void hh_presence_sensor_init(void)
{
    _Static_assert(HH_ENTER_MM < HH_EXIT_MM,"Presence hysteresis required");
    lock=xSemaphoreCreateMutex(); assert(lock);
    i2c_master_bus_config_t cfg={.i2c_port=I2C_NUM_1,
        .sda_io_num=HH_TOF_SDA_GPIO,.scl_io_num=HH_TOF_SCL_GPIO,
        .clk_source=I2C_CLK_SRC_DEFAULT,.glitch_ignore_cnt=7,
        .flags.enable_internal_pullup=true};
    ESP_ERROR_CHECK(i2c_new_master_bus(&cfg,&bus));
    unsigned found=0;
    for(uint8_t address=8;address<0x78;++address) {
        if(i2c_master_probe(bus,address,10)==ESP_OK) {
            ++found;
            ESP_LOGI("presence","I2C1 scan ACK address=0x%02x SDA27/SCL26",address);
        }
    }
    ESP_LOGI("presence","I2C1 scan complete: %u responding addresses",found);
    assert(xTaskCreate(task,"presence",8192,NULL,4,NULL)==pdPASS);
}

void hh_presence_sensor_snapshot(hh_sample_t *s)
{
    xSemaphoreTake(lock,portMAX_DELAY);
    s->animal_present=state.animal_present;
    s->presence_valid=state.presence_valid && esp_timer_get_time()-updated_us<500000;
    s->distance_mm=s->presence_valid?distance_mm:-1;
    if(s->presence_valid)s->valid_mask|=HH_VALID_TOF;
    xSemaphoreGive(lock);
    s->audio_recording=hh_audio_is_recording();
}
