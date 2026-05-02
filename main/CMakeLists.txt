idf_component_register(
    SRCS 
        "ev_charging.cpp"
        "network/webserver.cpp"
        "network/api_controller.cpp"
        "network/websocket.cpp"
        "network/search.cpp"
        "network/wifi.c"
        "driver/motor.c"
        "driver/limit.c"
        "driver/ir.c"
    INCLUDE_DIRS 
        "."
        "network"
        "driver"
)

spiffs_create_partition_image(spiffs web FLASH_IN_PROJECT)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall -Wextra -Werror)

message(STATUS "MAIN CMAKE LOADED")