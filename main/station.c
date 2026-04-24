#include "station.h"
#include "driver/mcpwm_timer.h"
#include "driver/mcpwm_oper.h"
#include "driver/mcpwm_cmpr.h"
#include "driver/mcpwm_gen.h"
#include "driver/mcpwm_fault.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "pins.h"

#define TAG "STATION"

/* -------- CONFIG -------- */
#define TIMER_RES_HZ   80000000      // 80 MHz
#define MIN_FREQ_HZ    15000
#define MAX_FREQ_HZ    30000
#define FREQ_TO_TICKS(f) (TIMER_RES_HZ / (f))

/* -------- STATE -------- */
static station_state_t state = STATION_STATE_OFF;
static float current_duty = 50.0f;
static uint32_t current_freq = 15000;

/* -------- MCPWM HANDLES -------- */
static mcpwm_timer_handle_t timer = NULL;
static mcpwm_oper_handle_t oper = NULL;
static mcpwm_cmpr_handle_t comparator = NULL;
static mcpwm_gen_handle_t gen_hin = NULL;
static mcpwm_gen_handle_t gen_lin = NULL;

/* ============================================================ */
/* ===================== INIT FUNCTION ========================= */
/* ============================================================ */

void station_init(void)
{
    ESP_LOGI(TAG, "Initializing WPT Station (ESP-IDF v6)...");

    /* -------- TIMER -------- */
    mcpwm_timer_config_t tcfg = {
        .group_id = 0,
        .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
        .resolution_hz = TIMER_RES_HZ,
        .period_ticks = FREQ_TO_TICKS(current_freq),
        .count_mode = MCPWM_TIMER_COUNT_MODE_UP,
    };
    ESP_ERROR_CHECK(mcpwm_new_timer(&tcfg, &timer));

    /* -------- OPERATOR -------- */
    mcpwm_operator_config_t ocfg = {
        .group_id = 0,
    };
    ESP_ERROR_CHECK(mcpwm_new_operator(&ocfg, &oper));
    ESP_ERROR_CHECK(mcpwm_operator_connect_timer(oper, timer));

    /* -------- COMPARATOR -------- */
    mcpwm_comparator_config_t cpcfg = {
        .flags.update_cmp_on_tez = true,
    };
    ESP_ERROR_CHECK(mcpwm_new_comparator(oper, &cpcfg, &comparator));

    /* -------- GENERATORS -------- */
    mcpwm_generator_config_t gcfg_h = {
        .gen_gpio_num = PIN_HIN,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &gcfg_h, &gen_hin));

    mcpwm_generator_config_t gcfg_l = {
        .gen_gpio_num = PIN_LIN,
    };
    ESP_ERROR_CHECK(mcpwm_new_generator(oper, &gcfg_l, &gen_lin));

    /* -------- PWM ACTIONS -------- */
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

    /* Complementary output */
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
    uint32_t dead_ticks = (uint32_t)(((uint64_t)800 * TIMER_RES_HZ) / 1000000000ULL);

    mcpwm_dead_time_config_t dtcfg = {
        .posedge_delay_ticks = dead_ticks,
        .negedge_delay_ticks = dead_ticks,
        .flags.invert_output = true
    };

    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(gen_hin, gen_lin, &dtcfg));

    /* ============================================================ */
    /* ===================== FAULT CONFIG ========================== */
    /* ============================================================ */

    /* GPIO CONFIG (IMPORTANT in v6) */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_FAULT_INPUT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    /* MCPWM FAULT */
    mcpwm_fault_handle_t fault = NULL;

    mcpwm_gpio_fault_config_t fcfg = {
        .group_id = 0,
        .gpio_num = PIN_FAULT_INPUT,
    };
    ESP_ERROR_CHECK(mcpwm_new_gpio_fault(&fcfg, &fault));

    /* BRAKE CONFIG */
    mcpwm_brake_config_t bcfg = {
        .fault = fault,
        .brake_mode = MCPWM_OPER_BRAKE_MODE_CBC,
    };
    ESP_ERROR_CHECK(mcpwm_operator_set_brake_on_fault(oper, &bcfg));

    /* BRAKE ACTION */
    mcpwm_gen_brake_event_action_t brake_action = {
        .direction = MCPWM_TIMER_DIRECTION_UP,
        .brake_mode = MCPWM_OPER_BRAKE_MODE_CBC,
        .action = MCPWM_GEN_ACTION_LOW
    };

    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_brake_event(gen_hin, brake_action));
    ESP_ERROR_CHECK(mcpwm_generator_set_action_on_brake_event(gen_lin, brake_action));

    /* -------- SAFE START -------- */
    ESP_ERROR_CHECK(mcpwm_comparator_set_compare_value(comparator, 0));
    ESP_ERROR_CHECK(mcpwm_timer_enable(timer));
    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));

    state = STATION_STATE_OFF;

    ESP_LOGI(TAG, "Initialization complete");
}

/* ============================================================ */
/* ===================== DUTY CONTROL ========================== */
/* ============================================================ */

bool station_set_duty(float duty)
{
    if (state == STATION_STATE_FAULT) return false;

    if (duty < 0.0f) duty = 0.0f;
    if (duty > 100.0f) duty = 100.0f;

    current_duty = duty;

    uint32_t compare = (uint32_t)((FREQ_TO_TICKS(current_freq) * duty) / 100.0f);

    if (mcpwm_comparator_set_compare_value(comparator, compare) != ESP_OK) {
        return false;
    }

    return true;
}

/* ============================================================ */
/* ===================== PWM UPDATE ============================ */
/* ============================================================ */

void station_update_pwm(uint32_t freq_hz, uint32_t dead_time_ns)
{
    if (state == STATION_STATE_FAULT) return;

    if (freq_hz < MIN_FREQ_HZ) freq_hz = MIN_FREQ_HZ;
    if (freq_hz > MAX_FREQ_HZ) freq_hz = MAX_FREQ_HZ;

    current_freq = freq_hz;

    uint32_t dt_ticks = (uint32_t)(((uint64_t)dead_time_ns * TIMER_RES_HZ) / 1000000000ULL);

    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_STOP_EMPTY));

    mcpwm_dead_time_config_t dtcfg = {
        .posedge_delay_ticks = dt_ticks,
        .negedge_delay_ticks = dt_ticks,
        .flags.invert_output = true
    };

    ESP_ERROR_CHECK(mcpwm_generator_set_dead_time(gen_hin, gen_lin, &dtcfg));
    ESP_ERROR_CHECK(mcpwm_timer_set_period(timer, FREQ_TO_TICKS(freq_hz)));

    ESP_ERROR_CHECK(mcpwm_timer_start_stop(timer, MCPWM_TIMER_START_NO_STOP));

    station_set_duty(current_duty);
}

station_state_t station_get_state(void)
{
    return state;
}

float station_get_duty(void)
{
    return current_duty;
}

void station_stop(void)
{
    state = STATION_STATE_OFF;
    mcpwm_comparator_set_compare_value(comparator, 0);
}

void station_start(void)
{
    state = STATION_STATE_RUNNING;
}

void station_fault_shutdown(void)
{
    state = STATION_STATE_FAULT;
    mcpwm_comparator_set_compare_value(comparator, 0);
}

void station_clear_fault(void)
{
    if (state == STATION_STATE_FAULT) {
        state = STATION_STATE_OFF;
    }
}