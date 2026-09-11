#ifndef __PROVISION_HSM_H__
#define __PROVISION_HSM_H__

#include "hsm.h"

void provision_hsm_init(void);
const char *provision_get_ssid(void);
const char *provision_get_password(void);

#endif /* __PROVISION_HSM_H__ */
