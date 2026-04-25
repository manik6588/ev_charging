<<<<<<< HEAD
idf_component_register(
    SRCS 
        "ev_charging.cpp"
        "network/webserver.cpp"
        "network/api_controller.cpp"
        "network/websocket.cpp"
        "network/wifi.c"
        "driver/motor.c"
    INCLUDE_DIRS 
        "."
        "network"
        "driver"
=======
idf_component_register(SRCS "mdns_service.c" "websocket.c" "api.c" "ev_charging.c" "motor.c" "limit.c" "station.c" "wifi.c" "webserver.c" "sens.c"
    INCLUDE_DIRS "."
>>>>>>> 0163fb06762cb1e63fe04f3e602cb5d5c3771051
)

spiffs_create_partition_image(spiffs web FLASH_IN_PROJECT)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)

message(STATUS "MAIN CMAKE LOADED")