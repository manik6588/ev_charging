#include "limit.h"
#include "pins.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_log.h"

#define LIMIT_ACTIVE_LEVEL 0
#define DEBOUNCE_DELAY_US 5000

static const char *TAG = "LIMIT";
static uint8_t last_stable_state = 0;

void limit_init(void) {
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << PIN_X_MIN) | (1ULL << PIN_X_MAX) |
                        (1ULL << PIN_Y_MIN) | (1ULL << PIN_Y_MAX),
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    ESP_LOGI(TAG, "Limit switches initialized (Active-Low, Pull-ups enabled)");
}

uint8_t limit_get_state(void) {
    // 1. Capture first sample
    int xm1 = gpio_get_level(PIN_X_MIN);
    int xx1 = gpio_get_level(PIN_X_MAX);
    int ym1 = gpio_get_level(PIN_Y_MIN);
    int yx1 = gpio_get_level(PIN_Y_MAX);

    // 2. Delay for debounce
    esp_rom_delay_us(DEBOUNCE_DELAY_US);

    // 3. Capture second sample
    int xm2 = gpio_get_level(PIN_X_MIN);
    int xx2 = gpio_get_level(PIN_X_MAX);
    int ym2 = gpio_get_level(PIN_Y_MIN);
    int yx2 = gpio_get_level(PIN_Y_MAX);

    uint8_t current_state = 0;

    // 4. Logic: Must be stable across both samples and match Active Level (0V)
    if (xm1 == xm2 && xm1 == LIMIT_ACTIVE_LEVEL) current_state |= BIT_X_MIN;
    if (xx1 == xx2 && xx1 == LIMIT_ACTIVE_LEVEL) current_state |= BIT_X_MAX;
    if (ym1 == ym2 && ym1 == LIMIT_ACTIVE_LEVEL) current_state |= BIT_Y_MIN;
    if (yx1 == yx2 && yx1 == LIMIT_ACTIVE_LEVEL) current_state |= BIT_Y_MAX;

    if (current_state != last_stable_state) {
        ESP_LOGI(TAG, "Limit State Change: 0x%02X", current_state);
        last_stable_state = current_state;
    }

    return current_state;
}

bool x_min_pressed(void) { return (limit_get_state() & BIT_X_MIN); } 
bool x_max_pressed(void) { return (limit_get_state() & BIT_X_MAX); } 
bool y_min_pressed(void) { return (limit_get_state() & BIT_Y_MIN); } 
bool y_max_pressed(void) { return (limit_get_state() & BIT_Y_MAX); }