#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#include "esp_err.h"
#include "hal/adc_types.h"

#define STATUS_LED_GPIO         2

#define PH_ADC_GPIO             34
#define PH_ADC_CHANNEL          ADC_CHANNEL_6

#define ADC_SAMPLE_COUNT        40
#define ADC_SATURATION_THRESH   4090

esp_err_t hardware_init(void);

esp_err_t hardware_adc_read_raw(int *out_raw);
esp_err_t hardware_adc_read_avg(int *out_avg);
esp_err_t hardware_adc_read_voltage(int *out_mv);
esp_err_t hardware_ph_read_voltage(float *out_voltage);

#endif /* __HARDWARE_H__ */
