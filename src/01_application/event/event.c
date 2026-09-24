#include "event.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

static QueueHandle_t s_event_queue = NULL;

bool event_init(void)
{
    if (s_event_queue != NULL)
    {
        return true;
    }

    s_event_queue = xQueueCreate(EVENT_BUFF, sizeof(event_t));
    return (s_event_queue != NULL);
}

bool event_post(const event_t *event)
{
    if (s_event_queue == NULL || event == NULL)
    {
        return false;
    }

    return (xQueueSend(s_event_queue, event, 0) == pdTRUE);
}

bool event_get(event_t *event)
{
    if (s_event_queue == NULL || event == NULL)
    {
        return false;
    }

    return (xQueueReceive(s_event_queue, event, 0) == pdTRUE);
}
