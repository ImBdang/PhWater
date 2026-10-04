#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#include "esp_err.h"
#include "hal/adc_types.h"

#define STATUS_LED_GPIO                 5
#define BUTTON_GPIO                     15

#define ANALOG_SENSOR_ADC_GPIO          34
#define ANALOG_SENSOR_ADC_CHANNEL       ADC_CHANNEL_6

esp_err_t hardware_init(void);

bool hardware_button_is_pressed(void);

esp_err_t hardware_adc_read_raw(int *out_raw);

esp_err_t hardware_adc_raw_to_voltage(int raw, int *out_mv);

#endif /* __HARDWARE_H__ */
