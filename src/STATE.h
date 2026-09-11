#ifndef __STATE_H__
#define __STATE_H__

#include "hsm.h"

extern const hsm_state_t g_state_root;
extern const hsm_state_t g_state_not_configured;
extern const hsm_state_t g_state_provisioning;
extern const hsm_state_t g_state_verifying;
extern const hsm_state_t g_state_configured;

#define STATE_ROOT              (&g_state_root)
#define STATE_NOT_CONFIGURED    (&g_state_not_configured)
#define STATE_PROVISIONING      (&g_state_provisioning)
#define STATE_VERIFYING         (&g_state_verifying)
#define STATE_CONFIGURED        (&g_state_configured)

#endif /* __STATE_H__ */
