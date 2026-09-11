#include "ring_buff.h"
#include <string.h>


static uint8_t *m_event_next(event_buffer_t *q, uint8_t *ptr)
{
    ptr += q->item_size;

    if (ptr >= q->buffer + (q->capacity * q->item_size))
    {
        ptr = q->buffer;
    }

    return ptr;
}


bool m_event_init(event_buffer_t *q,
                  void *storage,
                  size_t capacity,
                  size_t item_size)
{
    if ((q == NULL) ||
        (storage == NULL) ||
        (capacity < 2U) ||
        (item_size == 0U))
    {
        return false;
    }

    q->buffer    = (uint8_t *)storage;
    q->head      = q->buffer;
    q->tail      = q->buffer;
    q->item_size = item_size;
    q->capacity  = capacity;

    return true;
}


bool m_event_post(event_buffer_t *q,
                  const void *item)
{
    if ((q == NULL) || (item == NULL))
    {
        return false;
    }

    uint8_t *next = m_event_next(q, q->head);

    /* Queue full */
    if (next == q->tail)
    {
        return false;
    }

    memcpy(q->head, item, q->item_size);

    q->head = next;

    return true;
}


bool m_event_pop(event_buffer_t *q,
                 void *item)
{
    if ((q == NULL) || (item == NULL))
    {
        return false;
    }

    /* Queue empty */
    if (q->head == q->tail)
    {
        return false;
    }

    memcpy(item, q->tail, q->item_size);

    q->tail = m_event_next(q, q->tail);

    return true;
}
