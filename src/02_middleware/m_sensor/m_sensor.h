#ifndef __M_SENSOR_H__
#define __M_SENSOR_H__

#include "esp_err.h"

#define SENSOR_ADC_SAMPLE_COUNT         40
#define SENSOR_ADC_SATURATION_RAW       4090
#define SENSOR_DIVIDER_GAIN             2.0f

typedef struct
{
    int raw;            /* Average raw ADC sau filtering */
    int adc_mv;         /* Calibrated voltage tại GPIO34 (sau divider, mV) */
    float po_voltage;   /* Estimated voltage tại PH-4502C PO (trước divider, Volt) */
} sensor_sample_t;

esp_err_t m_sensor_read(sensor_sample_t *out_sample);

#endif /* __M_SENSOR_H__ */
