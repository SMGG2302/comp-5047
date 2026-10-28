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
#define HH_WICK2_GPIO 36
#define HH_LIGHT_GPIO 39
#define HH_WATER_GPIO 32
#define HH_SOUND_GPIO 33
#define HH_ENABLE_SOUND 1

/* Set to 0 for sensors not connected; an unplugged ADC wire cannot be detected
 * reliably in software. Disabled values are -1/NAN with cleared valid bits. */
#define HH_ENABLE_SHT3X 1
#define HH_ENABLE_DS18B20 0
#define HH_ENABLE_WICK 1
#define HH_ENABLE_WICK2 1
#define HH_ENABLE_LIGHT 1
#if HH_ENABLE_WICK2 && HH_ENABLE_LIGHT && (HH_WICK2_GPIO == HH_LIGHT_GPIO)
#error "wick2 and light must use different GPIOs"
#endif
#define HH_ENABLE_WATER 1
/* Light/water AO: 0..3.3 V, raw ADC counts only; thresholds not calibrated. */

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
#define HH_ENTER_MM 189 /* mm < 190; presence update uses <= */
#define HH_EXIT_MM 220 /* measured empty hollow: 220..230 mm */
#define HH_ENTER_READINGS 3
#define HH_EXIT_READINGS 5
#define HH_AUDIO_DURATION_MS 30000
/* Light-only valve test; no automatic test sweep or timed cycling. */
#define HH_ENABLE_SERVO 1
#define HH_SERVO_GPIO 13
#define HH_SERVO_MIN_US 1200
#define HH_SERVO_MAX_US 2500
#define HH_SERVO_RAMP_STEP_US 10
#define HH_SERVO_RAMP_INTERVAL_MS 100
#define HH_SERVO_START_ANGLE 0
#define HH_VALVE_OPEN_US HH_SERVO_MIN_US
#define HH_VALVE_CLOSED_US HH_SERVO_MAX_US
#define HH_VALVE_OPEN_LIGHT_RAW 500
#endif
