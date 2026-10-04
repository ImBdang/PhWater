#ifndef __PROVISION_HSM_H__
#define __PROVISION_HSM_H__

#include "hsm.h"

typedef enum
{
    PROVISION_ORIGIN_NOT_CONFIGURED,
    PROVISION_ORIGIN_CONFIGURED,
} provision_origin_t;

void provision_hsm_init(void);
void provision_hsm_set_origin(provision_origin_t origin);
provision_origin_t provision_hsm_get_origin(void);
const char *provision_get_ssid(void);
const char *provision_get_password(void);

#endif /* __PROVISION_HSM_H__ */
