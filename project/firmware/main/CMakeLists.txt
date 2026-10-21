idf_component_register(
    SRCS "main.c" "sensors.c" "analog.c" "audio_record.c" "presence_sensor.c" "../../shared/telemetry.c"
    INCLUDE_DIRS "." "../../shared"
    REQUIRES esp_wifi esp_event esp_netif nvs_flash esp_timer esp_adc
             esp_driver_gpio esp_driver_i2c esp_partition lwip st_tof
)
