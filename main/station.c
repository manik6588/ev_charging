#include "station.h"
#include "driver/mcpwm.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"   // for esp_rom_delay_us()

#define TAG "STATION"

/* ================= CONFIG ================= */

#define PIN_HIN 21
#define PIN_LIN 22

#define PWM_FREQ 80000   // 80 kHz
#define DEAD_TIME_NS 800

// MCPWM clock ≈ 80MHz → 12.5ns per tick
#define DEAD_TIME_TICKS (DEAD_TIME_NS / 12.5)

/* ================= STATE ================= */

static station_state_t state = STATION_STATE_OFF;
static float current_duty = 0;

/* ================= INTERNAL SAFETY ================= */

// Force both outputs LOW immediately
static inline void force_all_low(void)
{
    mcpwm_set_signal_low(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A);
    mcpwm_set_signal_low(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B);
}

// Small guard delay (extra safety beyond hardware dead-time)
static inline void guard_delay(void)
{
    esp_rom_delay_us(2); // 2 µs safety margin
}

/* ================= INIT ================= */

void station_init(void)
{
    // Configure MCPWM pins
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, PIN_HIN);
    mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0B, PIN_LIN);

    // PWM configuration
    mcpwm_config_t pwm_config = {
        .frequency = PWM_FREQ,
        .cmpr_a = 0,
        .cmpr_b = 0,
        .counter_mode = MCPWM_UP_COUNTER,
        .duty_mode = MCPWM_DUTY_MODE_0,
    };

    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);

    // 🔥 DEAD-TIME (core safety)
    mcpwm_deadtime_enable(
        MCPWM_UNIT_0,
        MCPWM_TIMER_0,
        MCPWM_ACTIVE_HIGH_COMPLIMENT_MODE,
        DEAD_TIME_TICKS,
        DEAD_TIME_TICKS
    );

    // Start safe
    force_all_low();

    state = STATION_STATE_OFF;
    ESP_LOGI(TAG, "Initialized with 800ns dead-time");
}

/* ================= CONTROL ================= */

bool station_set_duty(float duty)
{
    if (state == STATION_STATE_FAULT) {
        ESP_LOGE(TAG, "Cannot set duty: FAULT state");
        return false;
    }

    if (duty < 0.0f || duty > 100.0f) {
        ESP_LOGE(TAG, "Invalid duty: %.2f", duty);
        return false;
    }

    current_duty = duty;

    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, duty);
    mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, duty);

    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, MCPWM_DUTY_MODE_0);
    mcpwm_set_duty_type(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_B, MCPWM_DUTY_MODE_0);

    return true;
}

void station_start(void)
{
    if (state == STATION_STATE_FAULT) {
        ESP_LOGE(TAG, "Cannot start: FAULT state");
        return;
    }

    // 🔒 Interlock: ensure both LOW before enabling
    force_all_low();
    guard_delay();

    // Apply last duty
    station_set_duty(current_duty);

    state = STATION_STATE_RUNNING;
    ESP_LOGI(TAG, "Started");
}

void station_stop(void)
{
    // 🔒 Interlock: immediate shutdown
    force_all_low();
    guard_delay();

    state = STATION_STATE_OFF;
    ESP_LOGI(TAG, "Stopped");
}

/* ================= FAULT HANDLING ================= */

void station_fault_shutdown(void)
{
    // 🚨 HARD STOP
    force_all_low();

    state = STATION_STATE_FAULT;

    ESP_LOGE(TAG, "FAULT: Emergency shutdown triggered");
}

void station_clear_fault(void)
{
    if (state != STATION_STATE_FAULT)
        return;

    // Ensure safe condition before re-enable
    force_all_low();
    guard_delay();

    state = STATION_STATE_OFF;

    ESP_LOGI(TAG, "Fault cleared");
}

station_state_t station_get_state(void)
{
    return state;
}