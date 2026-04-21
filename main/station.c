#include "station.h"
#include "driver/mcpwm_prelude.h"
#include "esp_log.h"
#include "esp_rom_sys.h"

#define TAG "STATION"

/* ================= CONFIG ================= */

// GPIO
#define PIN_HIN 26
#define PIN_LIN 27

// PWM CONFIG
#define PWM_FREQ        15000          // 15 kHz
#define TIMER_RES_HZ    10000000       // 10 MHz

#define PERIOD_TICKS    (TIMER_RES_HZ / PWM_FREQ)   // ≈ 666 ticks

// DEAD TIME
#define DEAD_TIME_NS    800
#define DEAD_TICKS ((uint32_t)(((uint64_t)DEAD_TIME_NS * TIMER_RES_HZ) / 1000000000ULL)) // ≈ 8 ticks

/* ================= STATE ================= */

static station_state_t state = STATION_STATE_OFF;
static float current_duty = 50.0f;   // default 50%

/* MCPWM Handles */
static mcpwm_timer_handle_t timer = NULL;
static mcpwm_oper_handle_t oper = NULL;
static mcpwm_cmpr_handle_t comparator = NULL;
static mcpwm_gen_handle_t gen_hin = NULL;
static mcpwm_gen_handle_t gen_lin = NULL;

/* ================= INTERNAL ================= */

static inline void guard_delay(void)
{
    esp_rom_delay_us(2); // short safety delay
}

/* ================= INIT ================= */

void station_init(void)
{
    ESP_LOGI(TAG, "Init start");

    // Sanity check
    if (PERIOD_TICKS == 0 || PERIOD_TICKS > 1000000) {
        ESP_LOGE(TAG, "Invalid PERIOD_TICKS: %lu", PERIOD_TICKS);
        return;
    }

    ESP_LOGI(TAG, "Freq=%d Hz | Period=%lu | Dead=%lu ticks",
             PWM_FREQ, PERIOD_TICKS, DEAD_TICKS);

    /* -------- TIMER -------- */
    mcpwm_timer_config_t tcfg = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = TIMER_RES_HZ,
        .period_ticks = PERIOD_TICKS,
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&tcfg, &timer));

    /* -------- OPERATOR -------- */
    mcpwm_operator_config_t ocfg = {
        .group_id = 0
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&ocfg, &oper));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

    /* -------- COMPARATOR -------- */
    mcpwm_comparator_config_t ccfg = {};
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &ccfg, &comparator));

    /* -------- GENERATORS -------- */
    mcpwm_generator_config_t gcfg_h = {
        .gen_gpio_num = PIN_HIN
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &gcfg_h, &gen_hin));

    mcpwm_generator_config_t gcfg_l = {
        .gen_gpio_num = PIN_LIN
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &gcfg_l, &gen_lin));

    /* -------- PWM ACTIONS -------- */

    // HIN: HIGH at start, LOW at compare
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(
        gen_hin,
        MCPWM_GEN_TIMER_EVENT_ACTION(
            MCPWM_TIMER_DIRECTION_UP,
            MCPWM_TIMER_EVENT_EMPTY,
            MCPWM_GEN_ACTION_HIGH
        )
    ));

    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(
        gen_hin,
        MCPWM_GEN_COMPARE_EVENT_ACTION(
            MCPWM_TIMER_DIRECTION_UP,
            comparator,
            MCPWM_GEN_ACTION_LOW
        )
    ));

    // LIN: complementary (LOW at start, HIGH at compare)
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_timer_event(
        gen_lin,
        MCPWM_GEN_TIMER_EVENT_ACTION(
            MCPWM_TIMER_DIRECTION_UP,
            MCPWM_TIMER_EVENT_EMPTY,
            MCPWM_GEN_ACTION_LOW
        )
    ));

    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_compare_event(
        gen_lin,
        MCPWM_GEN_COMPARE_EVENT_ACTION(
            MCPWM_TIMER_DIRECTION_UP,
            comparator,
            MCPWM_GEN_ACTION_HIGH
        )
    ));

    /* -------- DEAD TIME -------- */
    mcpwm_dead_time_config_t dtcfg = {
        .posedge_delay_ticks = DEAD_TICKS,
        .negedge_delay_ticks = DEAD_TICKS
    };

    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(gen_hin, gen_lin, &dtcfg));

    /* -------- SAFE START -------- */
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, 0));

    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));

    state = STATION_STATE_OFF;

    ESP_LOGI(TAG, "Initialized successfully");
}

/* ================= CONTROL ================= */

bool station_set_duty(float duty)
{
    if (state == STATION_STATE_FAULT) {
        ESP_LOGE(TAG, "Cannot set duty: FAULT");
        return false;
    }

    // Clamp duty for safety
    if (duty < 5.0f)  duty = 5.0f;
    if (duty > 95.0f) duty = 95.0f;

    current_duty = duty;

    uint32_t cmp = (uint32_t)((duty / 100.0f) * PERIOD_TICKS);

    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, cmp));

    return true;
}

void station_start(void)
{
    if (state == STATION_STATE_FAULT) {
        ESP_LOGE(TAG, "Cannot start: FAULT");
        return;
    }

    guard_delay();

    station_set_duty(current_duty);

    state = STATION_STATE_RUNNING;

    ESP_LOGI(TAG, "Started (Duty=%.1f%%)", current_duty);
}

void station_stop(void)
{
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, 0));

    guard_delay();

    state = STATION_STATE_OFF;

    ESP_LOGI(TAG, "Stopped");
}

/* ================= FAULT ================= */

void station_fault_shutdown(void)
{
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, 0));

    state = STATION_STATE_FAULT;

    ESP_LOGE(TAG, "FAULT: Shutdown");
}

void station_clear_fault(void)
{
    if (state != STATION_STATE_FAULT)
        return;

    guard_delay();

    state = STATION_STATE_OFF;

    ESP_LOGI(TAG, "Fault cleared");
}

station_state_t station_get_state(void)
{
    return state;
}