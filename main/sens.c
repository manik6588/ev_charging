#include "sens.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"

#define TAG "SENS"

/* ================= CONFIG ================= */

// ADC Channels (adjust GPIO mapping if needed)
#define VIN_CH ADC_CHANNEL_6  // GPIO34
#define IIN_CH ADC_CHANNEL_7  // GPIO35
#define VOUT_CH ADC_CHANNEL_4 // GPIO32 (peak detect)
#define IOUT_CH ADC_CHANNEL_5 // GPIO33 (ACS758)

// Sampling
#define ADC_SAMPLES 32

// Scaling
#define VIN_DIV_RATIO 5.0f
#define VOUT_DIV_RATIO 10.0f // adjust based on your divider

// Current Sensors
#define ACS712_SENS 0.185f // Input current (5A module)
#define ACS758_SENS 0.040f // Example: 50A version (~40mV/A)

/* ================= INTERNAL ================= */

static adc_oneshot_unit_handle_t adc_handle;
static adc_cali_handle_t cali_handle = NULL;

static power_data_t data = {0};

static float iin_offset = 0;
static float iout_offset = 0;

/* ================= FILTER ================= */

static inline float low_pass(float prev, float input, float alpha)
{
    return prev + alpha * (input - prev);
}

/* ================= ADC ================= */

static int read_adc_avg(adc_channel_t ch)
{
    int raw = 0, sum = 0;

    for (int i = 0; i < ADC_SAMPLES; i++)
    {
        adc_oneshot_read(adc_handle, ch, &raw);
        sum += raw;
    }

    return sum / ADC_SAMPLES;
}

static float adc_to_voltage(int raw)
{
    int mv = 0;
    adc_cali_raw_to_voltage(cali_handle, raw, &mv);
    return mv / 1000.0f;
}

/* ================= INIT ================= */

void sens_init(void)
{
    adc_oneshot_unit_init_cfg_t init_cfg = {
        .unit_id = ADC_UNIT_1,
    };
    adc_oneshot_new_unit(&init_cfg, &adc_handle);

    adc_oneshot_chan_cfg_t cfg = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = ADC_ATTEN_DB_12,
    };

    adc_oneshot_config_channel(adc_handle, VIN_CH, &cfg);
    adc_oneshot_config_channel(adc_handle, IIN_CH, &cfg);
    adc_oneshot_config_channel(adc_handle, VOUT_CH, &cfg);
    adc_oneshot_config_channel(adc_handle, IOUT_CH, &cfg);

    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id = ADC_UNIT_1,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };

    adc_cali_create_scheme_line_fitting(&cali_cfg, &cali_handle);

    ESP_LOGI(TAG, "Calibrating current offsets...");

    float sum_iin = 0;
    float sum_iout = 0;

    for (int i = 0; i < 200; i++)
    {
        sum_iin += adc_to_voltage(read_adc_avg(IIN_CH));
        sum_iout += adc_to_voltage(read_adc_avg(IOUT_CH));
    }

    iin_offset = sum_iin / 200.0f;
    iout_offset = sum_iout / 200.0f;

    ESP_LOGI(TAG, "IIN offset: %.3f V | IOUT offset: %.3f V", iin_offset, iout_offset);
}

/* ================= UPDATE ================= */

void sens_update(void)
{
    static float vin_f = 0, iin_f = 0;
    static float vout_f = 0, iout_f = 0;

    float vin_adc = adc_to_voltage(read_adc_avg(VIN_CH));
    float iin_adc = adc_to_voltage(read_adc_avg(IIN_CH));
    float vout_adc = adc_to_voltage(read_adc_avg(VOUT_CH));
    float iout_adc = adc_to_voltage(read_adc_avg(IOUT_CH));

    // Scaling
    float vin = vin_adc * VIN_DIV_RATIO;
    float vout = vout_adc * VOUT_DIV_RATIO;

    float iin = (iin_adc - iin_offset) / ACS712_SENS;
    float iout = (iout_adc - iout_offset) / ACS758_SENS;

    // Filtering (tuned for switching noise)
    vin_f = low_pass(vin_f, vin, 0.1f);
    iin_f = low_pass(iin_f, iin, 0.05f);
    vout_f = low_pass(vout_f, vout, 0.1f);
    iout_f = low_pass(iout_f, iout, 0.05f);

    // Power (approximation)
    float pin = vin_f * iin_f;
    float pout = vout_f * iout_f;

    data.vin = vin_f;
    data.iin = iin_f;
    data.vout = vout_f;
    data.iout = iout_f;
    data.pin = pin;
    data.pout = pout;
}

/* ================= GET ================= */

power_data_t sens_get_data(void)
{
    return data;
}