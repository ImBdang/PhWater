#ifndef __M_LED_H__
#define __M_LED_H__

#include <stdbool.h>

#define LED_BLINK_NOT_CONFIG_MS   500U
#define LED_BLINK_CONNECTING_MS   1500U

typedef enum
{
    LED_NOT_CONFIGURED = 0,
    LED_CONFIGURED,
    LED_ONLINE,
} led_status_t;

void led_task(void *arg);
void led_set_status(led_status_t status);

void status_not_config(void);
void status_connecting(void);
void status_online(void);

#endif /* __M_LED_H__ */
