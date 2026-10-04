#pragma once
#include "event.h"
#include "esp_err.h"
esp_err_t mqtt_hsm_init(void);
void mqtt_hsm_start(void);
void mqtt_hsm_stop(void);
void mqtt_hsm_dispatch(const event_t *event);
