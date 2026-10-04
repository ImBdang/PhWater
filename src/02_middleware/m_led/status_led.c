#include "m_led.h"
#include "hardware.h"
#include "debug.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static led_status_t current_status = LED_NOT_CONFIGURED;

void led_set_status(led_status_t status)
{
    DEBUG_LOG("LED status changed -> %s (%d)",
              status == LED_ONLINE ? "ONLINE (Solid ON)" :
              status == LED_CONFIGURED ? "CONFIGURED (Blink 1.5s)" : "PROVISIONING / NOT_CONFIGURED (Blink 0.5s)",
              status);
    current_status = status;
}

void status_not_config(void)
{
    led_set_status(LED_NOT_CONFIGURED);
}

void status_connecting(void)
{
    led_set_status(LED_CONFIGURED);
}

void status_online(void)
{
    led_set_status(LED_ONLINE);
}

void led_task(void *arg)
{
    (void)arg;
    bool led_on = false;
    led_status_t last_status = (led_status_t)-1;

    DEBUG_LOG("LED task started");

    while (1)
    {
        if (current_status != last_status)
        {
            DEBUG_LOG("LED active mode: %s",
                      current_status == LED_ONLINE ? "ONLINE (Solid ON)" :
                      current_status == LED_CONFIGURED ? "CONFIGURED (Blink 1.5s)" : "PROVISIONING (Blink 0.5s)");
            last_status = current_status;
        }

        switch (current_status)
        {
            case LED_ONLINE:
                hardware_status_led_set(true);
                vTaskDelay(pdMS_TO_TICKS(500));
                break;

            case LED_CONFIGURED:
                led_on = !led_on;
                hardware_status_led_set(led_on);
                vTaskDelay(pdMS_TO_TICKS(LED_TOGGLE_CONFIGURED_MS));
                break;

            case LED_NOT_CONFIGURED:
            default:
                led_on = !led_on;
                hardware_status_led_set(led_on);
                vTaskDelay(pdMS_TO_TICKS(LED_TOGGLE_NOT_CONFIGURED_MS));
                break;
        }
    }
}
