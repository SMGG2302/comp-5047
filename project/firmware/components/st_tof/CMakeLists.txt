file(GLOB ST_SOURCES "vl53l0x/api/core/src/*.c")
idf_component_register(SRCS ${ST_SOURCES} "platform/platform.c"
    INCLUDE_DIRS "vl53l0x/api/core/inc" "platform"
    REQUIRES esp_driver_i2c)
