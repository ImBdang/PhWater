#include "m_database.h"

#include <string.h>

#include "nvs.h"
#include "nvs_flash.h"

#define NVS_NAMESPACE   "device"
#define NVS_KEY_INFO    "info"


static bool database_init(void)
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        if (nvs_flash_erase() != ESP_OK)
            return false;

        ret = nvs_flash_init();
    }

    return ret == ESP_OK;
}


bool save_info(const device_info_t *info)
{
    if (info == NULL)
        return false;

    if (!database_init())
        return false;

    nvs_handle_t handle;

    if (nvs_open(
            NVS_NAMESPACE,
            NVS_READWRITE,
            &handle) != ESP_OK)
    {
        return false;
    }

    esp_err_t ret = nvs_set_blob(
        handle,
        NVS_KEY_INFO,
        info,
        sizeof(device_info_t)
    );

    if (ret == ESP_OK)
        ret = nvs_commit(handle);

    nvs_close(handle);

    return ret == ESP_OK;
}


bool get_info(device_info_t *info)
{
    if (info == NULL)
        return false;

    if (!database_init())
        return false;

    nvs_handle_t handle;

    if (nvs_open(
            NVS_NAMESPACE,
            NVS_READONLY,
            &handle) != ESP_OK)
    {
        return false;
    }

    size_t size = sizeof(device_info_t);

    esp_err_t ret = nvs_get_blob(
        handle,
        NVS_KEY_INFO,
        info,
        &size
    );

    nvs_close(handle);

    if (ret != ESP_OK)
        return false;

    if (size != sizeof(device_info_t))
        return false;

    return true;
}
