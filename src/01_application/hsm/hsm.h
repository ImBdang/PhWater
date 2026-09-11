#ifndef __HSM_H__
#define __HSM_H__

#include <stdbool.h>
#include "event.h"

typedef struct hsm        hsm_t;
typedef struct hsm_state  hsm_state_t;

typedef bool (*hsm_handler_t)(hsm_t *hsm, const event_t *event);
typedef void (*hsm_action_t)(hsm_t *hsm);

struct hsm_state
{
    const hsm_state_t *parent;

    hsm_handler_t handler;

    hsm_action_t entry;
    hsm_action_t exit;
};


struct hsm
{
    const hsm_state_t *current;
};


void hsm_init(
    hsm_t *hsm,
    const hsm_state_t *initial
);

void hsm_dispatch(
    hsm_t *hsm,
    const event_t *event
);

void hsm_transition(
    hsm_t *hsm,
    const hsm_state_t *target
);

#endif
