#ifndef __M_EVENT__
#define __M_EVENT__

#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"

typedef struct 
{
  uint8_t *buffer;

  uint8_t *head;
  uint8_t *tail;

  size_t  item_size;
  size_t  capacity;
} event_buffer_t;

bool m_event_init(event_buffer_t *q,
                    void *storage,
                    size_t capacity,
                    size_t item_size);

bool m_event_post(event_buffer_t *q,
                  const void *item);

bool m_event_pop(event_buffer_t *q,
                 void *item);

#endif
