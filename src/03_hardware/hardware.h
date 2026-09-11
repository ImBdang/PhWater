#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#include "esp_err.h"
#include "hal/adc_types.h"

#define STATUS_LED_GPIO         2

#define PH_ADC_GPIO             34
#define PH_ADC_CHANNEL          ADC_CHANNEL_6

#define ADC_RESOLUTION          4096
#define ADC_SAMPLE_COUNT        40

esp_err_t hardware_init(void);

#endif /* __HARDWARE_H__ */
