#ifndef __WIFI_HSM_H__
#define __WIFI_HSM_H__

#include <stdbool.h>
#include "event.h"

void wifi_hsm_init(void);
void wifi_hsm_dispatch(const event_t *event);
bool wifi_hsm_set_credentials(const char *ssid, const char *password);

#endif /* __WIFI_HSM_H__ */
