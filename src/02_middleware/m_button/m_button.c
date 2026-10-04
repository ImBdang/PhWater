#include "m_button.h"
#include "hardware.h"
#include "debug.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUTTON_POLL_INTERVAL_MS     25U
#define BUTTON_DEBOUNCE_MS          50U
#define BUTTON_HOLD_TRIGGER_MS      5000U

static m_button_callback_t s_button_callback = NULL;

static void button_task(void *arg)
{
    (void)arg;

    bool last_raw_pressed = false;
    bool debounced_pressed = false;
    TickType_t debounce_start_tick = xTaskGetTickCount();
    TickType_t press_start_tick = 0;
    bool long_press_sent = false;

    DEBUG_LOG("Button task started (Hold 5s for long press)");

    while (1)
    {
        bool raw_pressed = hardware_button_is_pressed();

        if (raw_pressed != last_raw_pressed)
        {
            last_raw_pressed = raw_pressed;
            debounce_start_tick = xTaskGetTickCount();
        }
        else
        {
            if ((xTaskGetTickCount() - debounce_start_tick) >= pdMS_TO_TICKS(BUTTON_DEBOUNCE_MS))
            {
                if (raw_pressed != debounced_pressed)
                {
                    debounced_pressed = raw_pressed;
                    if (debounced_pressed)
                    {
                        press_start_tick = xTaskGetTickCount();
                        long_press_sent = false;
                        if (s_button_callback != NULL)
                        {
                            s_button_callback(M_BUTTON_EVENT_PRESSED);
                        }
                    }
                    else
                    {
                        long_press_sent = false;
                        if (s_button_callback != NULL)
                        {
                            s_button_callback(M_BUTTON_EVENT_RELEASED);
                        }
                    }
                }
            }
        }

        if (debounced_pressed && !long_press_sent)
        {
            TickType_t elapsed = xTaskGetTickCount() - press_start_tick;
            if (elapsed >= pdMS_TO_TICKS(BUTTON_HOLD_TRIGGER_MS))
            {
                long_press_sent = true;
                DEBUG_LOG("Button held for 5s -> reporting LONG_PRESS");
                if (s_button_callback != NULL)
                {
                    s_button_callback(M_BUTTON_EVENT_LONG_PRESS);
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_INTERVAL_MS));
    }
}

esp_err_t m_button_init(m_button_callback_t callback)
{
    if (callback == NULL)
    {
        return ESP_ERR_INVALID_ARG;
    }

    s_button_callback = callback;

    BaseType_t ret = xTaskCreate(
        button_task,
        "button_task",
        2048,
        NULL,
        3,
        NULL
    );

    if (ret != pdPASS)
    {
        return ESP_ERR_NO_MEM;
    }

    return ESP_OK;
}
