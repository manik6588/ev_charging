#include "limit.h"
#include "pins.h"
#include "driver/gpio.h"


void limit_init()
{
    // Pins with internal pull-up
    gpio_config_t io_pullup = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL<<PIN_X_MAX) | (1ULL<<PIN_Y_MIN),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    // Input-only pins (external pull-up required)
    gpio_config_t io_plain = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL<<PIN_X_MIN) | (1ULL<<PIN_Y_MAX),
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&io_pullup);
    gpio_config(&io_plain);
}

int x_min_pressed() { return gpio_get_level(PIN_X_MIN) == 0; }
int x_max_pressed() { return gpio_get_level(PIN_X_MAX) == 0; }
int y_min_pressed() { return gpio_get_level(PIN_Y_MIN) == 0; }
int y_max_pressed() { return gpio_get_level(PIN_Y_MAX) == 0; }