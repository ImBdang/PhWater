#include "m_led.h"
#include "hardware.h"
#include "debug.h"

#include "driver/gpio.h"
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

/*
 * Mạch LED: 5V -> 1k -> LED -> D5 (Active-LOW)
 * Mức 0 (0V)   : LED SÁNG (ON)
 * Mức 1 (3.3V) : LED TẮT (OFF)
 */
#define LED_PIN_ON   0
#define LED_PIN_OFF  1

void led_task(void *arg)
{
    (void)arg;
    bool level = false;
    led_status_t last_status = (led_status_t)-1;

    DEBUG_LOG("LED task started on GPIO %d (Active-LOW: 0=ON, 1=OFF)", STATUS_LED_GPIO);

    while (1)
    {
        if (current_status != last_status)
        {
            DEBUG_LOG("LED active mode: %s | GPIO %d",
                      current_status == LED_ONLINE ? "ONLINE (Solid ON)" :
                      current_status == LED_CONFIGURED ? "CONFIGURED (Blink 1.5s)" : "PROVISIONING (Blink 0.5s)",
                      STATUS_LED_GPIO);
            last_status = current_status;
        }

        switch (current_status)
        {
            case LED_ONLINE:
                gpio_set_level(STATUS_LED_GPIO, LED_PIN_ON);
                vTaskDelay(pdMS_TO_TICKS(500));
                break;

            case LED_CONFIGURED:
                level = !level;
                gpio_set_level(STATUS_LED_GPIO, level ? LED_PIN_ON : LED_PIN_OFF);
                vTaskDelay(pdMS_TO_TICKS(LED_BLINK_CONNECTING_MS));
                break;

            case LED_NOT_CONFIGURED:
            default:
                level = !level;
                gpio_set_level(STATUS_LED_GPIO, level ? LED_PIN_ON : LED_PIN_OFF);
                vTaskDelay(pdMS_TO_TICKS(LED_BLINK_NOT_CONFIG_MS));
                break;
        }
    }
}
