#include "m_led.h"
#include "hardware.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static bool led_state = false;


void status_online(void)
{
    gpio_set_level(STATUS_LED_GPIO, 0);

    led_state = true;
}


void status_offline(void)
{
    led_state = !led_state;

    gpio_set_level(
        STATUS_LED_GPIO,
        led_state ? 0 : 1
    );

    vTaskDelay(pdMS_TO_TICKS(LED_BLINK_DELAY_MS));
}
