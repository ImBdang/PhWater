#ifndef __HARDWARE_H__
#define __HARDWARE_H__

#include "esp_err.h"

#define STATUS_LED_GPIO         2

#define ADC_RESOLUTION          1024
#define ADC_SAMPLE_COUNT        40

esp_err_t hardware_init(void);

#endif /* __HARDWARE_H__ */
