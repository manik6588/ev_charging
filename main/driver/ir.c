#include "ir.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"

static const char *TAG = "IR";

#define ADC_CHANNEL ADC_CHANNEL_6   // GPIO34
#define ADC_UNIT ADC_UNIT_1

#define IR_THRESHOLD 2000   // 🔧 tune this!

static adc_oneshot_unit_handle_t adc_handle;

/* ================= INIT ================= */
esp_err_t ir_init(void)
{
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));

    ESP_LOGI(TAG, "IR sensor initialized (ADC)");

    return ESP_OK;
}

/* ================= RAW READ ================= */
esp_err_t ir_read(int *value)
{
    if (value == NULL)
        return ESP_ERR_INVALID_ARG;

    return adc_oneshot_read(adc_handle, ADC_CHANNEL, value);
}

/* ================= FILTERED READ ================= */
int ir_read_filtered(void)
{
    int sum = 0;
    int val = 0;

    for (int i = 0; i < 5; i++)
    {
        if (ir_read(&val) == ESP_OK)
            sum += val;
    }

    int avg = sum / 5;

    ESP_LOGD(TAG, "IR avg = %d", avg);

    return avg;
}

/* ================= DETECTION ================= */
bool ir_detected(void)
{
    int val = ir_read_filtered();

    if (val > IR_THRESHOLD)
    {
        ESP_LOGW(TAG, "IR DETECTED (%d)", val);
        return true;
    }

    return false;
}