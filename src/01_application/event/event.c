#include "event.h"
#include "ring_buff.h"


static event_t event_storage[EVENT_BUFF];
static event_buffer_t event_buffer;


bool event_init(void)
{
    return m_event_init(
        &event_buffer,
        event_storage,
        EVENT_BUFF,
        sizeof(event_t)
    );
}


bool event_post(const event_t *event)
{
    if (event == NULL)
    {
        return false;
    }

    return m_event_post(
        &event_buffer,
        event
    );
}


bool event_get(event_t *event)
{
    if (event == NULL)
    {
        return false;
    }

    return m_event_pop(
        &event_buffer,
        event
    );
}
