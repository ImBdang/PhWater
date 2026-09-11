#ifndef __WIFI_HSM_H__
#define __WIFI_HSM_H__

#include "event.h"

void wifi_hsm_init(void);
void wifi_hsm_dispatch(const event_t *event);

#endif /* __WIFI_HSM_H__ */
