#include "provision_hsm.h"
#include "STATE.h"
#include "event.h"
#include "wifi_hsm.h"
#include "debug.h"
#include "m_led.h"
#include "m_database.h"
#include "m_ble.h"

#include <string.h>
#include "esp_timer.h"

#define PROVISION_TIMEOUT_US    (5ULL * 60ULL * 1000ULL * 1000ULL) /* 5 minutes */

static char s_ssid[64];
static char s_pass[64];
static esp_timer_handle_t s_provision_timer = NULL;

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

static void provision_timeout_cb(void *arg)
{
    (void)arg;
    DEBUG_LOG("Provisioning 5-minute timeout expired!");

    event_t event = {
        .id = EVT_PROVISION_TIMEOUT,
    };
    if (!event_post(&event))
    {
        DEBUG_LOG("Failed to post EVT_PROVISION_TIMEOUT");
    }
}

static void provision_entry(hsm_t *hsm)
{
    (void)hsm;
    DEBUG_LOG("State entered: PROVISIONING");
    led_set_status(LED_NOT_CONFIGURED);
    m_ble_start();

    if (s_provision_timer != NULL)
    {
        esp_timer_stop(s_provision_timer);
        esp_err_t err = esp_timer_start_once(s_provision_timer, PROVISION_TIMEOUT_US);
        if (err == ESP_OK)
        {
            DEBUG_LOG("Provisioning 5-minute timer started");
        }
        else
        {
            DEBUG_LOG("Failed to start provision timer: %s", esp_err_to_name(err));
        }
    }
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
    else if (event->id == EVT_PROVISION_TIMEOUT)
    {
        DEBUG_LOG("Provisioning timed out -> Returning to normal operation (CONFIGURED)");
        hsm_transition(
            hsm,
            STATE_CONFIGURED
        );

        return true;
    }

    return false;
}

static void provision_exit(hsm_t *hsm)
{
    (void)hsm;
    DEBUG_LOG("State exit: PROVISIONING");

    if (s_provision_timer != NULL)
    {
        esp_timer_stop(s_provision_timer);
    }

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

    esp_timer_create_args_t timer_args = {
        .callback = provision_timeout_cb,
        .arg = NULL,
        .name = "prov_timer",
    };

    esp_err_t err = esp_timer_create(&timer_args, &s_provision_timer);
    if (err != ESP_OK)
    {
        DEBUG_LOG("Failed to create provision timer: %s", esp_err_to_name(err));
    }
}
