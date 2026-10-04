#ifndef __M_BUTTON_H__
#define __M_BUTTON_H__

#include <stdbool.h>
#include "esp_err.h"

typedef enum
{
    M_BUTTON_EVENT_PRESSED,
    M_BUTTON_EVENT_RELEASED,
    M_BUTTON_EVENT_LONG_PRESS,
} m_button_event_t;

typedef void (*m_button_callback_t)(m_button_event_t event);

esp_err_t m_button_init(m_button_callback_t callback);

#endif /* __M_BUTTON_H__ */
