#include "sens.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "esp_log.h"

#define TAG "SENS"

/* ================= CONFIG ================= */

// ADC channels
#define VOLTAGE_CHANNEL ADC1_CHANNEL_6   // GPIO34
#define CURRENT_CHANNEL ADC1_CHANNEL_7   // GPIO35

#define ADC_SAMPLES 32
#define DEFAULT_VREF 1100

// 🔧 Adjust based on your module
#define VOLTAGE_DIVIDER_RATIO 5.0f

// ACS712 version (change this!)
#define ACS712_SENSITIVITY 0.185f   // ✅ for 05B

/* ================= INTERNAL ================= */

static esp_adc_cal_characteristics_t adc_chars;
static power_data_t data = {0};

static float current_offset = 0;

/* ================= FILTER ================= */

static inline float low_pass(float prev, float input, float alpha)
{
    return prev + alpha * (input - prev);
}

/* ================= ADC ================= */

static uint32_t read_adc_avg(adc1_channel_t ch)
{
    uint32_t sum = 0;

    for (int i = 0; i < ADC_SAMPLES; i++)
        sum += adc1_get_raw(ch);

    return sum / ADC_SAMPLES;
}

/* ================= INIT ================= */

void sens_init(void)
{
    adc1_config_width(ADC_WIDTH_BIT_12);

    adc1_config_channel_atten(VOLTAGE_CHANNEL, ADC_ATTEN_DB_11);
    adc1_config_channel_atten(CURRENT_CHANNEL, ADC_ATTEN_DB_11);

    esp_adc_cal_characterize(
        ADC_UNIT_1,
        ADC_ATTEN_DB_11,
        ADC_WIDTH_BIT_12,
        DEFAULT_VREF,
        &adc_chars
    );

    ESP_LOGI(TAG, "Calibrating ACS712 offset...");

    // 🔥 Offset calibration (no current flowing)
    float sum = 0;

    for (int i = 0; i < 200; i++)
    {
        uint32_t raw = read_adc_avg(CURRENT_CHANNEL);
        float mv = esp_adc_cal_raw_to_voltage(raw, &adc_chars);
        sum += (mv / 1000.0f);
    }

    current_offset = sum / 200.0f;

    ESP_LOGI(TAG, "Offset = %.3f V", current_offset);
}

/* ================= UPDATE ================= */

void sens_update(void)
{
    static float v_f = 0;
    static float i_f = 0;

    // Read ADC
    uint32_t v_raw = read_adc_avg(VOLTAGE_CHANNEL);
    uint32_t i_raw = read_adc_avg(CURRENT_CHANNEL);

    float v_adc = esp_adc_cal_raw_to_voltage(v_raw, &adc_chars) / 1000.0f;
    float i_adc = esp_adc_cal_raw_to_voltage(i_raw, &adc_chars) / 1000.0f;

    // 🔌 Voltage scaling
    float voltage = v_adc * VOLTAGE_DIVIDER_RATIO;

    // ⚡ Current calculation (ACS712)
    float current = (i_adc - current_offset) / ACS712_SENSITIVITY;

    // 🔥 Filter
    v_f = low_pass(v_f, voltage, 0.1f);
    i_f = low_pass(i_f, current, 0.1f);

    // ⚡ Power
    float power = v_f * i_f;

    data.voltage = v_f;
    data.current = i_f;
    data.power   = power;
}

/* ================= GET ================= */

power_data_t sens_get_data(void)
{
    return data;
}