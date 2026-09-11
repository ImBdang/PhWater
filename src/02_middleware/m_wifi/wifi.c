#include "m_wifi.h"

#include <string.h>

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "ping/ping_sock.h"
#include "lwip/ip_addr.h"

#define WIFI_PING_IP    "8.8.8.8"
#define WIFI_PING_COUNT 4U

static bool ping_success = false;

static void ping_on_success(esp_ping_handle_t hdl, void *args)
{
    ping_success = true;
}

static void ping_on_end(esp_ping_handle_t hdl, void *args)
{
    esp_ping_delete_session(hdl);
}

bool m_wifi_ping(void)
{
    ip_addr_t target;

    if (!ipaddr_aton(WIFI_PING_IP, &target))
        return false;

    ping_success = false;

    esp_ping_config_t config = ESP_PING_DEFAULT_CONFIG();

    config.target_addr = target;
    config.count = WIFI_PING_COUNT;

    esp_ping_callbacks_t cb = {
        .on_ping_success = ping_on_success,
        .on_ping_end = ping_on_end,
        .cb_args = NULL,
    };

    esp_ping_handle_t ping;

    if (esp_ping_new_session(&config, &cb, &ping) != ESP_OK)
        return false;

    return esp_ping_start(ping) == ESP_OK;
}

bool m_wifi_is_online(void)
{
    return ping_success;
}

bool m_wifi_init(void)
{
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();

    if (esp_netif_init() != ESP_OK)
        return false;

    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
        return false;

    esp_netif_create_default_wifi_sta();

    if (esp_wifi_init(&config) != ESP_OK)
        return false;

    if (esp_wifi_set_storage(WIFI_STORAGE_RAM) != ESP_OK)
        return false;

    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK)
        return false;

    if (esp_wifi_start() != ESP_OK)
        return false;

    return true;
}

bool m_wifi_connect(const char *ssid, const char *password)
{
    if ((ssid == NULL) || (password == NULL))
        return false;

    wifi_config_t config = {0};

    strncpy(
        (char *)config.sta.ssid,
        ssid,
        sizeof(config.sta.ssid) - 1
    );

    strncpy(
        (char *)config.sta.password,
        password,
        sizeof(config.sta.password) - 1
    );

    if (esp_wifi_set_config(WIFI_IF_STA, &config) != ESP_OK)
        return false;

    if (esp_wifi_connect() != ESP_OK)
        return false;

    return true;
}

bool m_wifi_disconnect(void)
{
    return esp_wifi_disconnect() == ESP_OK;
}
