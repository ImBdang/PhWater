#include "m_led.h"
#include "hardware.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "debug.h"

static led_status_t current_status = LED_NOT_CONFIGURED;

void led_set_status(led_status_t status)
{
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
    bool level = false;

    DEBUG_LOG("LED task started on GPIO 2 & GPIO 0");

    while (1)
    {
        switch (current_status)
        {
            case LED_ONLINE:
                gpio_set_level(STATUS_LED_GPIO, 1);
                gpio_set_level(0, 1);
                vTaskDelay(pdMS_TO_TICKS(100));
                break;

            case LED_CONFIGURED:
                level = !level;
                gpio_set_level(STATUS_LED_GPIO, (uint32_t)level);
                gpio_set_level(0, (uint32_t)level);
                vTaskDelay(pdMS_TO_TICKS(LED_BLINK_CONNECTING_MS));
                break;

            case LED_NOT_CONFIGURED:
            default:
                level = !level;
                gpio_set_level(STATUS_LED_GPIO, (uint32_t)level);
                gpio_set_level(0, (uint32_t)level);
                vTaskDelay(pdMS_TO_TICKS(LED_BLINK_NOT_CONFIG_MS));
                break;
        }
    }
}
