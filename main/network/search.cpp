#include "search.hpp"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

extern "C" {
#include "pins.h"
#include "motor.h"
#include "limit.h"
#include "ir.h"
}

#define LOOP_DELAY_MS 10
static const char *TAG = "SEARCH";

static TaskHandle_t search_task_handle = NULL;
static volatile bool running = false;

typedef enum { DIR_FORWARD = 1, DIR_BACKWARD = 0 } direction_t;

// ===== CONSTANTS =====
#define MAX_SPEED           220   // High speed for general search
#define SLOW_SPEED          140    // Precision speed for alignment
#define ALIGNMENT_THRESHOLD 3650  // Target intensity
#define DETECTION_THRESHOLD 1500  // Transition to slow speed
#define SCAN_SETTLE_MS      500   // Time to confirm peak stability
#define PEAK_TOLERANCE      0.05  // +/- 5% tolerance

// ===== ALIGNMENT STATE =====
static int max_ir_recorded = 0;
static bool is_settling = false;
static bool is_aligned = false;
static TickType_t settle_start_time = 0;

/**
 * @brief Handles alignment and auto-restarts if signal is lost.
 * @return true if currently aligned/settled, false otherwise.
 */
static bool perform_ir_alignment() {
    int current_ir = ir_read_filtered();
    
    // 1. Monitor Alignment (Auto-Restart Logic)
    if (is_aligned) {
        // If intensity drops more than 5% below peak, restart search
        if (current_ir < (max_ir_recorded * (1.0 - PEAK_TOLERANCE))) {
            ESP_LOGW(TAG, "Alignment lost (%d < %d). Restarting search...", current_ir, (int)(max_ir_recorded * 0.95));
            is_aligned = false;
            is_settling = false;
            max_ir_recorded = 0; // Reset peak to find new position
            return false;
        }
        return true; // Still aligned
    }

    // 2. Tracking Peak Intensity
    if (current_ir > max_ir_recorded) {
        max_ir_recorded = current_ir;
    }

    // 3. State: Approaching Source (Slow Motion)
    if (!is_settling && current_ir > DETECTION_THRESHOLD && current_ir < ALIGNMENT_THRESHOLD) {
        motorX_forward(SLOW_SPEED + 50);
        motorY_forward(SLOW_SPEED);
        return false;
    }

    // 4. State: Reached Threshold (Stop and Settle)
    if (!is_settling && current_ir >= ALIGNMENT_THRESHOLD) {
        motor_stop_all();
        is_settling = true;
        settle_start_time = xTaskGetTickCount();
        return false;
    }

    // 5. State: Verifying Peak Stability
    if (is_settling) {
        if ((xTaskGetTickCount() - settle_start_time) < pdMS_TO_TICKS(SCAN_SETTLE_MS)) {
            return false; 
        }

        // Final 5% Tolerance check before locking alignment
        if (current_ir >= (max_ir_recorded * (1.0 - PEAK_TOLERANCE))) {
            ESP_LOGI(TAG, "🎯 ALIGNED AT PEAK: %d", current_ir);
            motor_stop_all();
            is_aligned = true;
            return true;
        } else {
            // Deviation during settling: continue slow motion
            is_settling = false;
            motorX_forward(SLOW_SPEED); 
            motorY_forward(SLOW_SPEED); 
            return false;
        }
    }
    return false;
}

static void set_x_direction(direction_t dir) {
    if (dir == DIR_FORWARD) motorX_forward(MAX_SPEED);
    else motorX_backward(MAX_SPEED);
}

static void set_y_direction(direction_t dir) {
    if (dir == DIR_FORWARD) motorY_forward(MAX_SPEED);
    else motorY_backward(MAX_SPEED);
}

static void search_task(void *arg) {
    motor_enable();
    max_ir_recorded = 0;
    is_settling = false;
    is_aligned = false;

    // Initial Search
    set_x_direction(DIR_FORWARD);
    set_y_direction(DIR_FORWARD);

    uint8_t prev_state = 0;
    TickType_t lock_x_until = 0;
    TickType_t lock_y_until = 0;

    while (running) {
        int current_ir = ir_read_filtered();

        // 1. Perform Alignment Logic (Includes Auto-Restart)
        bool aligned = perform_ir_alignment();

        // 2. Navigation Logic (Only if far from source and NOT aligned)
        if (!aligned && current_ir < DETECTION_THRESHOLD) {
            uint8_t current_state = limit_get_state();
            TickType_t now = xTaskGetTickCount();

            // X-Axis Navigation (Motor A)
            if (now >= lock_x_until) {
                if ((current_state & BIT_X_MAX) && !(prev_state & BIT_X_MAX)) {
                    set_x_direction(DIR_BACKWARD);
                    lock_x_until = now + pdMS_TO_TICKS(150);
                } else if ((current_state & BIT_X_MIN) && !(prev_state & BIT_X_MIN)) {
                    set_x_direction(DIR_FORWARD);
                    lock_x_until = now + pdMS_TO_TICKS(150);
                }
            }

            // Y-Axis Navigation (Motor B)
            if (now >= lock_y_until) {
                if ((current_state & BIT_Y_MAX) && !(prev_state & BIT_Y_MAX)) {
                    set_y_direction(DIR_BACKWARD);
                    lock_y_until = now + pdMS_TO_TICKS(150);
                } else if ((current_state & BIT_Y_MIN) && !(prev_state & BIT_Y_MIN)) {
                    set_y_direction(DIR_FORWARD);
                    lock_y_until = now + pdMS_TO_TICKS(150);
                }
            }
            prev_state = current_state; 
        }

        vTaskDelay(pdMS_TO_TICKS(LOOP_DELAY_MS)); 
    }

    motor_stop_all();
    search_task_handle = NULL;
    vTaskDelete(NULL); 
}

extern "C" esp_err_t search_init(void) {
    limit_init();
    motor_init();
    ir_init();
    return ESP_OK;
}

extern "C" void search_start(void) {
    if (running) return;
    running = true;
    xTaskCreate(search_task, "search_task", 4096, NULL, 5, &search_task_handle);
}

extern "C" void search_stop(void) { running = false; }