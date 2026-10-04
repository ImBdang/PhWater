#include "m_button.h"
#include "hardware.h"
#include "event.h"
#include "debug.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUTTON_POLL_INTERVAL_MS     50U
#define BUTTON_HOLD_TRIGGER_MS      5000U

void button_init(void)
{
    /* Hardware GPIO is initialized in hardware_gpio_init() */
}

void button_task(void *arg)
{
    (void)arg;

    uint32_t press_duration_ms = 0;
    bool hold_event_sent = false;

    DEBUG_LOG("Button task started on GPIO %d (Hold 5s to provision)", BUTTON_GPIO);

    while (1)
    {
        if (hardware_button_is_pressed())
        {
            press_duration_ms += BUTTON_POLL_INTERVAL_MS;

            /* Check if held continuously for 5 seconds */
            if ((press_duration_ms >= BUTTON_HOLD_TRIGGER_MS) && !hold_event_sent)
            {
                DEBUG_LOG("Button held for 5s -> Triggering EVT_START_PROVISION");

                event_t event = {
                    .id = EVT_START_PROVISION,
                };
                if (!event_post(&event))
                {
                    DEBUG_LOG("Failed to post EVT_START_PROVISION");
                }

                hold_event_sent = true;
            }
        }
        else
        {
            /* Button released -> reset counter and state */
            press_duration_ms = 0;
            hold_event_sent = false;
        }

        vTaskDelay(pdMS_TO_TICKS(BUTTON_POLL_INTERVAL_MS));
    }
}
