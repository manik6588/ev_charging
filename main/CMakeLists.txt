idf_component_register(SRCS "mdns_service.c" "websocket.c" "api.c" "ev_charging.c" "motor.c" "limit.c" "station.c" "wifi.c" "webserver.c" "sens.c"
    INCLUDE_DIRS "."
)

spiffs_create_partition_image(spiffs web FLASH_IN_PROJECT)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)

message(STATUS "MAIN CMAKE LOADED")