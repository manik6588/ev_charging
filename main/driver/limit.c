#include "limit.h"
#include "pins.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h" // for delay

#define LIMIT_ACTIVE_LEVEL 0   // 0 = pressed (pull-up), 1 = pressed (pull-down)
#define DEBOUNCE_DELAY_US 5000 // 5ms

static int debounce_read(gpio_num_t pin)
{
    int first = gpio_get_level(pin);
    esp_rom_delay_us(DEBOUNCE_DELAY_US);
    int second = gpio_get_level(pin);

    return (first == second) ? first : !LIMIT_ACTIVE_LEVEL;
}

void limit_init(void)
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask =
            (1ULL << PIN_X_MIN) |
            (1ULL << PIN_X_MAX) |
            (1ULL << PIN_Y_MIN) |
            (1ULL << PIN_Y_MAX),

        // 🔥 Use SAME configuration for all
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};

    gpio_config(&io_conf);
}

// Raw read (no debounce)
int limit_read_raw(uint8_t pin)
{
    return gpio_get_level(pin);
}

// Debounced + normalized
static int is_pressed(gpio_num_t pin)
{
    int level = debounce_read(pin);
    return (level == LIMIT_ACTIVE_LEVEL);
}

int x_min_pressed(void) { return is_pressed(PIN_X_MIN); }
int x_max_pressed(void) { return is_pressed(PIN_X_MAX); }
int y_min_pressed(void) { return is_pressed(PIN_Y_MIN); }
int y_max_pressed(void) { return is_pressed(PIN_Y_MAX); }

// Bitmask (useful for fast checks)
uint8_t limit_get_state(void)
{
    return (x_min_pressed() << 0) |
           (x_max_pressed() << 1) |
           (y_min_pressed() << 2) |
           (y_max_pressed() << 3);
}