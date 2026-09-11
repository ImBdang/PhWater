#ifndef __M_WIFI__
#define __M_WIFI__

#include "stdbool.h"

bool m_wifi_init(void);

bool m_wifi_connect(const char *ssid, const char *password);

bool m_wifi_disconnect(void);

bool m_wifi_ping(void);
bool m_wifi_is_online(void);

#endif
