#ifndef __SENSOR_HSM_H__
#define __SENSOR_HSM_H__

#include "esp_err.h"
#include "event.h"

esp_err_t sensor_hsm_init(void);

void sensor_hsm_dispatch(const event_t *event);

#endif /* __SENSOR_HSM_H__ */
