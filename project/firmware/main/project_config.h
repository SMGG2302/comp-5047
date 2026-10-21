#ifndef PROJECT_CONFIG_H
#define PROJECT_CONFIG_H

/* Change credentials here before sharing the device. WPA2 password: 8..63 chars. */
#define HH_AP_SSID "HollowHaven-ESP32"
#define HH_AP_PASSWORD "HollowHaven5047"
#define HH_AP_CHANNEL 6
#define HH_TCP_PORT 9000
#define HH_SAMPLE_PERIOD_MS 2000

/* Classic ESP32-WROOM-32 only. All signal levels must be 3.3 V compatible. */
#define HH_SDA_GPIO 21
#define HH_SCL_GPIO 22
#define HH_DS_GPIO 4
#define HH_WICK_GPIO 34
#define HH_LIGHT_GPIO 35
#define HH_WATER_GPIO 32
#define HH_SOUND_GPIO 33
#define HH_ENABLE_SOUND 1

/* Set to 0 for sensors not connected; an unplugged ADC wire cannot be detected
 * reliably in software. Disabled values are -1/NAN with cleared valid bits. */
#define HH_ENABLE_SHT3X 1
#define HH_ENABLE_DS18B20 0
#define HH_ENABLE_WICK 1
#define HH_ENABLE_LIGHT 0
#define HH_ENABLE_WATER 0

/* 0: power the water board from 3V3 for short bench tests.
 * 1: GPIO33 drives an ACTIVE-HIGH high-side load-switch enable (NOT sensor VCC).
 * The switch must block signal back-power and have a defined off state.
 * Never turn this on unless that external circuit exists. */
#define HH_WATER_SWITCH_ENABLED 0
#define HH_WATER_SWITCH_GPIO 33
#define HH_WATER_SETTLE_MS 100

/* VL53L0X dedicated second I2C bus. Thresholds are provisional bench values. */
#define HH_TOF_SDA_GPIO 27
#define HH_TOF_SCL_GPIO 26
#define HH_TOF_PERIOD_MS 100
#define HH_ENTER_MM 200
#define HH_EXIT_MM 250
#define HH_ENTER_READINGS 3
#define HH_EXIT_READINGS 5
#define HH_AUDIO_DURATION_MS 30000
#endif
