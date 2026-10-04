#include "m_sensor.h"
#include "hardware.h"

esp_err_t m_sensor_read(sensor_sample_t *out_sample)
{
    if (out_sample == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    int sum = 0;
    int valid_count = 0;

    for (int i = 0; i < SENSOR_ADC_SAMPLE_COUNT; i++)
    {
        int raw = 0;
        esp_err_t ret = hardware_adc_read_raw(&raw);
        if (ret != ESP_OK)
        {
            return ret;
        }

        if (raw >= SENSOR_ADC_SATURATION_RAW)
        {
            continue;
        }

        sum += raw;
        valid_count++;
    }

    if (valid_count < SENSOR_MIN_VALID_SAMPLES)
    {
        return ESP_ERR_INVALID_STATE;
    }

    int avg_raw = sum / valid_count;
    int adc_mv = 0;

    esp_err_t ret = hardware_adc_raw_to_voltage(avg_raw, &adc_mv);
    if (ret != ESP_OK)
    {
        return ret;
    }

    out_sample->raw = avg_raw;
    out_sample->adc_mv = adc_mv;
    out_sample->po_voltage = ((float)adc_mv / 1000.0f) * SENSOR_DIVIDER_GAIN;

    return ESP_OK;
}
