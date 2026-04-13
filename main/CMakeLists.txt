idf_component_register(SRCS "ev_charging.c" "motor.c" "limit.c" "station.c" "wifi.c" "webserver.c"
    INCLUDE_DIRS "."
)

spiffs_create_partition_image(spiffs web FLASH_IN_PROJECT)

message(STATUS "MAIN CMAKE LOADED")