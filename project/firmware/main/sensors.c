#include "sensors.h"
#include "project_config.h"

#include <math.h>
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "analog.h"
#include "presence_sensor.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "onewire_bus.h"
#include "ds18b20.h"

static const char *TAG = "sensors";
static i2c_master_bus_handle_t i2c_bus;
static i2c_master_dev_handle_t sht;
static onewire_bus_handle_t ow_bus;
static ds18b20_device_handle_t ds;

static uint8_t sht_crc(const uint8_t *data, size_t count)
{
    uint8_t crc = 0xff;
    for (size_t i = 0; i < count; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit)
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
    }
    return crc;
}

static bool find_sht(void)
{
    const uint8_t addresses[] = {0x44, 0x45};
    for (size_t i = 0; i < sizeof(addresses); ++i) {
        if (i2c_master_probe(i2c_bus, addresses[i], 50) != ESP_OK) continue;
        i2c_device_config_t config = {
            .dev_addr_length = I2C_ADDR_BIT_LEN_7,
            .device_address = addresses[i],
            .scl_speed_hz = 100000,
        };
        if (i2c_master_bus_add_device(i2c_bus, &config, &sht) == ESP_OK) {
            ESP_LOGI(TAG, "SHT3x responding at 0x%02x", addresses[i]);
            return true;
        }
    }
    return false;
}

static bool read_sht(float *temperature, float *humidity)
{
    if (!sht && !find_sht()) return false;
    /* SHT3x single shot, high repeatability, clock stretching disabled. */
    const uint8_t command[] = {0x24, 0x00};
    uint8_t bytes[6];
    esp_err_t err = i2c_master_transmit(sht, command, sizeof(command), 100);
    if (err == ESP_OK) {
        vTaskDelay(pdMS_TO_TICKS(20));
        err = i2c_master_receive(sht, bytes, sizeof(bytes), 100);
    }
    if (err != ESP_OK) {
        i2c_master_bus_rm_device(sht);
        sht = NULL; /* Re-probe next cycle after a disconnect. */
        return false;
    }
    if (sht_crc(bytes, 2) != bytes[2] || sht_crc(bytes + 3, 2) != bytes[5]) return false;
    uint16_t raw_t = ((uint16_t)bytes[0] << 8) | bytes[1];
    uint16_t raw_h = ((uint16_t)bytes[3] << 8) | bytes[4];
    *temperature = -45.0f + 175.0f * raw_t / 65535.0f;
    *humidity = 100.0f * raw_h / 65535.0f;
    return true;
}

static bool find_ds(void)
{
    onewire_device_iter_handle_t iterator = NULL;
    if (onewire_new_device_iter(ow_bus, &iterator) != ESP_OK) return false;
    onewire_device_t device;
    while (onewire_device_iter_get_next(iterator, &device) == ESP_OK) {
        ds18b20_config_t config = {};
        if (ds18b20_new_device_from_enumeration(&device, &config, &ds) == ESP_OK) break;
    }
    onewire_del_device_iter(iterator);
    if (ds) ESP_LOGI(TAG, "DS18B20 detected on GPIO%d", HH_DS_GPIO);
    return ds != NULL;
}

static bool read_ds(float *temperature)
{
    if (!ds && !find_ds()) return false;
    /* Official RMT-based driver avoids bit-bang timing interference from Wi-Fi.
     * This conversion call includes the sensor's conversion wait. */
    esp_err_t err = ds18b20_trigger_temperature_conversion(ds);
    if (err == ESP_OK) err = ds18b20_get_temperature(ds, temperature);
    if (err != ESP_OK || !isfinite(*temperature) || *temperature < -55 || *temperature > 125) {
        ds18b20_del_device(ds);
        ds = NULL;
        *temperature = NAN;
        return false;
    }
    return true;
}

void hh_sensors_init(void)
{
    hh_analog_init();
    hh_presence_sensor_init();
    if (HH_ENABLE_SHT3X) {
        i2c_master_bus_config_t config = {
            .i2c_port = I2C_NUM_0,
            .sda_io_num = HH_SDA_GPIO,
            .scl_io_num = HH_SCL_GPIO,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .flags.enable_internal_pullup = true,
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&config, &i2c_bus));
    }
    if (HH_ENABLE_DS18B20) {
        onewire_bus_config_t config = {
            .bus_gpio_num = HH_DS_GPIO,
            .flags.en_pull_up = true,
        };
        onewire_bus_rmt_config_t rmt_config = {.max_rx_bytes = 10};
        ESP_ERROR_CHECK(onewire_new_bus_rmt(&config, &rmt_config, &ow_bus));
    }
}

void hh_sensors_read(hh_sample_t *s)
{
    hh_sample_clear(s);
    if (HH_ENABLE_SHT3X && read_sht(&s->inside_temp_c, &s->inside_rh_pct))
        s->valid_mask |= HH_VALID_SHT;
    if (HH_ENABLE_DS18B20 && read_ds(&s->outside_temp_c)) s->valid_mask |= HH_VALID_DS18B20;
    hh_analog_snapshot(s);
    hh_presence_sensor_snapshot(s);
}
