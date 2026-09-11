#ifndef __M_DATABASE_H__
#define __M_DATABASE_H__

#include <stdbool.h>

#define DEVICE_ID       "ictu_haianh001"

#define SSID_SIZE       64
#define PASSWORD_SIZE   64

typedef struct
{
    char device_id[32];
    char ssid[SSID_SIZE];
    char password[PASSWORD_SIZE];

} device_info_t;

bool save_info(const device_info_t *info);

bool get_info(device_info_t *info);

#endif /* __M_DATABASE_H__ */
