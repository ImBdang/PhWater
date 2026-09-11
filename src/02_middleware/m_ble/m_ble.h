#ifndef __M_BLE_H__
#define __M_BLE_H__

#include <stdbool.h>

#define BLE_DEVICE_NAME    "ICTU-HaiAnh"

#define BLE_PROVISION_SERVICE_UUID_STR \
    "3f7c2e91-6a4b-4d8f-9c25-71b0e6a4d853"

#define BLE_CHAR_DEVICE_ID_UUID_STR \
    "3f7c2e92-6a4b-4d8f-9c25-71b0e6a4d853"

#define BLE_CHAR_SSID_UUID_STR \
    "3f7c2e93-6a4b-4d8f-9c25-71b0e6a4d853"

#define BLE_CHAR_PASSWORD_UUID_STR \
    "3f7c2e94-6a4b-4d8f-9c25-71b0e6a4d853"

/*
 * NimBLE stores 128-bit UUID in little-endian byte order.
 *
 * UUID:
 * 3f7c2e91-6a4b-4d8f-9c25-71b0e6a4d853
 */
#define BLE_PROVISION_SERVICE_UUID_BYTES \
    0x53, 0xd8, 0xa4, 0xe6,             \
    0xb0, 0x71,                         \
    0x25, 0x9c,                         \
    0x8f, 0x4d,                         \
    0x4b, 0x6a,                         \
    0x91, 0x2e, 0x7c, 0x3f

#define BLE_SSID_SIZE       64U
#define BLE_PASSWORD_SIZE   64U

typedef struct
{
    char ssid[BLE_SSID_SIZE];
    char password[BLE_PASSWORD_SIZE];

} ble_wifi_info_t;

typedef void (*m_ble_provision_cb_t)(
    const ble_wifi_info_t *info
);

bool m_ble_init(m_ble_provision_cb_t callback);

bool m_ble_start(void);

void m_ble_stop(void);

#endif
