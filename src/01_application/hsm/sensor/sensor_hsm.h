#ifndef __SENSOR_HSM_H__
#define __SENSOR_HSM_H__

#include "event.h"

void sensor_hsm_init(void);

void sensor_hsm_dispatch(const event_t *event);

#endif /* __SENSOR_HSM_H__ */
