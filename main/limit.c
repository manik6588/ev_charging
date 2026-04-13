#include "limit.h"
#include "pins.h"
#include "driver/gpio.h"


void limit_init()
{
    gpio_config_t io = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask =
            (1ULL<<PIN_X_MIN) | (1ULL<<PIN_X_MAX) |
            (1ULL<<PIN_Y_MIN) | (1ULL<<PIN_Y_MAX),
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&io);
}

int x_min_pressed() { return gpio_get_level(PIN_X_MIN) == 0; }
int x_max_pressed() { return gpio_get_level(PIN_X_MAX) == 0; }
int y_min_pressed() { return gpio_get_level(PIN_Y_MIN) == 0; }
int y_max_pressed() { return gpio_get_level(PIN_Y_MAX) == 0; }