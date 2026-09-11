#include "provision_hsm.h"
#include "STATE.h"
#include "event.h"
#include "wifi_hsm.h"
#include "debug.h"
#include "m_led.h"
#include "m_database.h"
#include "m_ble.h"

#include <string.h>

static char s_ssid[64];
static char s_pass[64];

static bool provision_handler(hsm_t *hsm, const event_t *event);
static bool verifying_handler(hsm_t *hsm, const event_t *event);

static void provision_entry(hsm_t *hsm);
static void provision_exit(hsm_t *hsm);
static void verifying_entry(hsm_t *hsm);

static void ble_provision_cb(const ble_wifi_info_t *info);

const hsm_state_t g_state_provisioning = {
    .parent  = STATE_NOT_CONFIGURED,
    .handler = provision_handler,
    .entry   = provision_entry,
    .exit    = provision_exit,
};

const hsm_state_t g_state_verifying = {
    .parent  = STATE_NOT_CONFIGURED,
    .handler = verifying_handler,
    .entry   = verifying_entry,
    .exit    = NULL,
};

const char *provision_get_ssid(void)
{
    return s_ssid;
}

const char *provision_get_password(void)
{
    return s_pass;
}

static void ble_provision_cb(const ble_wifi_info_t *info)
{
    if (info == NULL)
        return;

    strncpy(s_ssid, info->ssid, sizeof(s_ssid) - 1);
    s_ssid[sizeof(s_ssid) - 1] = '\0';

    strncpy(s_pass, info->password, sizeof(s_pass) - 1);
    s_pass[sizeof(s_pass) - 1] = '\0';

    DEBUG_LOG("BLE received provision credentials: SSID=%s", s_ssid);

    event_t event = {
        .id = EVT_PROVISIONED
    };

    event_post(&event);
}

static void provision_entry(hsm_t *hsm)
{
    (void)hsm;
    DEBUG_LOG("State entered: PROVISIONING");
    led_set_status(LED_NOT_CONFIGURED);
    m_ble_start();
}

static bool provision_handler(hsm_t *hsm, const event_t *event)
{
    if (event->id == EVT_PROVISIONED)
    {
        hsm_transition(
            hsm,
            STATE_VERIFYING
        );

        return true;
    }

    return false;
}

static void provision_exit(hsm_t *hsm)
{
    (void)hsm;
    DEBUG_LOG("State exit: PROVISIONING");
    m_ble_stop();
}

static void verifying_entry(hsm_t *hsm)
{
    (void)hsm;

    DEBUG_LOG("State entered: VERIFYING");
    led_set_status(LED_CONFIGURED);

    wifi_hsm_set_credentials(
        s_ssid,
        s_pass
    );

    event_t event = {
        .id = EVT_WIFI_CONNECT_REQ
    };

    wifi_hsm_dispatch(&event);
}

static bool verifying_handler(hsm_t *hsm, const event_t *event)
{
    wifi_hsm_dispatch(event);

    if (event->id == EVT_WIFI_UP)
    {
        device_info_t info = {0};

        strncpy(
            info.device_id,
            DEVICE_ID,
            sizeof(info.device_id) - 1
        );

        strncpy(
            info.ssid,
            s_ssid,
            sizeof(info.ssid) - 1
        );

        strncpy(
            info.password,
            s_pass,
            sizeof(info.password) - 1
        );

        if (save_info(&info))
        {
            DEBUG_LOG("WiFi verified & saved to NVS -> transition CONFIGURED");
            hsm_transition(
                hsm,
                STATE_CONFIGURED
            );
        }

        return true;
    }

    return true;
}

void provision_hsm_init(void)
{
    m_ble_init(ble_provision_cb);
}
