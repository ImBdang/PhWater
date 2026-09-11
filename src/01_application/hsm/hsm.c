#include "hsm.h"
#include <stddef.h>

#define HSM_MAX_DEPTH    8U

static size_t hsm_build_path(
    const hsm_state_t *state,
    const hsm_state_t **path
)
{
    size_t depth = 0;

    while ((state != NULL) &&
           (depth < HSM_MAX_DEPTH))
    {
        path[depth++] = state;
        state = state->parent;
    }

    return depth;
}

void hsm_init(
    hsm_t *hsm,
    const hsm_state_t *initial
)
{
    if ((hsm == NULL) || (initial == NULL))
        return;

    const hsm_state_t *path[HSM_MAX_DEPTH];

    size_t depth = hsm_build_path(
        initial,
        path
    );

    hsm->current = initial;

    while (depth > 0)
    {
        depth--;

        if (path[depth]->entry != NULL)
        {
            path[depth]->entry(hsm);
        }
    }
}

void hsm_dispatch(
    hsm_t *hsm,
    const event_t *event
)
{
    if ((hsm == NULL) ||
        (event == NULL) ||
        (hsm->current == NULL))
    {
        return;
    }

    const hsm_state_t *state =
        hsm->current;

    while (state != NULL)
    {
        if ((state->handler != NULL) &&
            state->handler(hsm, event))
        {
            return;
        }

        state = state->parent;
    }
}

void hsm_transition(
    hsm_t *hsm,
    const hsm_state_t *target
)
{
    if ((hsm == NULL) ||
        (hsm->current == NULL) ||
        (target == NULL))
    {
        return;
    }

    const hsm_state_t *source_path[HSM_MAX_DEPTH];
    const hsm_state_t *target_path[HSM_MAX_DEPTH];

    size_t source_depth =
        hsm_build_path(
            hsm->current,
            source_path
        );

    size_t target_depth =
        hsm_build_path(
            target,
            target_path
        );

    size_t src = source_depth;
    size_t dst = target_depth;

    while ((src > 0) &&
           (dst > 0) &&
           (source_path[src - 1] ==
            target_path[dst - 1]))
    {
        src--;
        dst--;
    }

    for (size_t i = 0; i < src; i++)
    {
        if (source_path[i]->exit != NULL)
        {
            source_path[i]->exit(hsm);
        }
    }

    while (dst > 0)
    {
        dst--;

        if (target_path[dst]->entry != NULL)
        {
            target_path[dst]->entry(hsm);
        }
    }

    hsm->current = target;
}
