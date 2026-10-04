#include "provision_hsm.h"
#include "STATE.h"
#include "event.h"
#include "wifi_hsm.h"
#include "debug.h"
#include "m_led.h"
#include "m_database.h"
#include "m_ble.h"
#include "m_wifi.h"

#include <string.h>
#include "esp_timer.h"

#define PROVISION_TIMEOUT_US    (5ULL * 60ULL * 1000ULL * 1000ULL) /* 5 minutes */
#define VERIFY_TIMEOUT_US       (25ULL * 1000ULL * 1000ULL)        /* 25 seconds */

static char s_ssid[64];
static char s_pass[64];
static esp_timer_handle_t s_provision_timer = NULL;
static esp_timer_handle_t s_verify_timer = NULL;
static provision_origin_t s_provision_origin = PROVISION_ORIGIN_NOT_CONFIGURED;

static bool provision_handler(hsm_t *hsm, const event_t *event);
static bool verifying_handler(hsm_t *hsm, const event_t *event);

static void provision_entry(hsm_t *hsm);
static void provision_exit(hsm_t *hsm);
static void verifying_entry(hsm_t *hsm);
static void verifying_exit(hsm_t *hsm);

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
    .exit    = verifying_exit,
};

void provision_hsm_set_origin(provision_origin_t origin)
{
    s_provision_origin = origin;
}

provision_origin_t provision_hsm_get_origin(void)
{
    return s_provision_origin;
}

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

static void verify_timeout_cb(void *arg)
{
    (void)arg;
    DEBUG_LOG("WiFi verification 25-second timeout expired!");

    event_t event = {
        .id = EVT_VERIFY_TIMEOUT,
    };
    if (!event_post(&event))
    {
        DEBUG_LOG("Failed to post EVT_VERIFY_TIMEOUT");
    }
}

static void provision_entry(hsm_t *hsm)
{
    (void)hsm;
    DEBUG_LOG("State entered: PROVISIONING (origin: %s)",
              s_provision_origin == PROVISION_ORIGIN_CONFIGURED ? "CONFIGURED" : "NOT_CONFIGURED");
    led_set_status(LED_NOT_CONFIGURED);
    memset(s_ssid, 0, sizeof(s_ssid));
    memset(s_pass, 0, sizeof(s_pass));

    m_ble_start();

    if (s_provision_timer != NULL)
    {
        esp_timer_stop(s_provision_timer);
        if (s_provision_origin == PROVISION_ORIGIN_CONFIGURED)
        {
            esp_err_t err = esp_timer_start_once(s_provision_timer, PROVISION_TIMEOUT_US);
            if (err == ESP_OK)
            {
                DEBUG_LOG("Re-provisioning 5-minute timer started");
            }
            else
            {
                DEBUG_LOG("Failed to start provision timer: %s", esp_err_to_name(err));
            }
        }
        else
        {
            DEBUG_LOG("Initial provisioning: BLE remains available without 5-minute timeout");
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
        if (s_provision_origin == PROVISION_ORIGIN_CONFIGURED)
        {
            DEBUG_LOG("Provisioning timed out -> Returning to normal operation (CONFIGURED)");
            hsm_transition(
                hsm,
                STATE_CONFIGURED
            );
        }
        else
        {
            DEBUG_LOG("Provisioning timed out in unconfigured mode -> Ignored");
        }

        return true;
    }
    else if (event->id == EVT_START_PROVISION)
    {
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
        .id = EVT_WIFI_RECONFIGURE_REQ
    };

    wifi_hsm_dispatch(&event);

    if (s_verify_timer != NULL)
    {
        esp_timer_stop(s_verify_timer);
        esp_err_t err = esp_timer_start_once(s_verify_timer, VERIFY_TIMEOUT_US);
        if (err == ESP_OK)
        {
            DEBUG_LOG("Verification 25-second timer started");
        }
        else
        {
            DEBUG_LOG("Failed to start verify timer: %s", esp_err_to_name(err));
        }
    }
}

static void verifying_exit(hsm_t *hsm)
{
    (void)hsm;
    DEBUG_LOG("State exit: VERIFYING");

    if (s_verify_timer != NULL)
    {
        esp_timer_stop(s_verify_timer);
    }
}

static bool verifying_handler(hsm_t *hsm, const event_t *event)
{
    switch (event->id)
    {
        case EVT_WIFI_CONNECTED:
            wifi_hsm_dispatch(event);
            return true;

        case EVT_WIFI_UP:
        {
            wifi_hsm_dispatch(event);

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
            }
            else
            {
                DEBUG_LOG("Failed to save verified credentials to NVS!");
            }

            hsm_transition(
                hsm,
                STATE_CONFIGURED
            );

            return true;
        }

        case EVT_WIFI_RECONFIGURE_REQ:
            wifi_hsm_dispatch(event);
            return true;

        case EVT_WIFI_DISCONNECTED:
            wifi_hsm_dispatch(event);
            return true;

        case EVT_VERIFY_TIMEOUT:
            DEBUG_LOG("WiFi verification timed out -> returning to PROVISIONING");
            m_wifi_disconnect();
            hsm_transition(
                hsm,
                STATE_PROVISIONING
            );
            return true;

        case EVT_START_PROVISION:
            DEBUG_LOG("EVT_START_PROVISION during VERIFYING -> return to PROVISIONING");
            m_wifi_disconnect();
            hsm_transition(
                hsm,
                STATE_PROVISIONING
            );
            return true;

        default:
            return false;
    }
}

void provision_hsm_init(void)
{
    if (!m_ble_init(ble_provision_cb))
    {
        DEBUG_LOG("Failed to initialize BLE provisioning");
    }

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

    esp_timer_create_args_t verify_args = {
        .callback = verify_timeout_cb,
        .arg = NULL,
        .name = "ver_timer",
    };

    err = esp_timer_create(&verify_args, &s_verify_timer);
    if (err != ESP_OK)
    {
        DEBUG_LOG("Failed to create verify timer: %s", esp_err_to_name(err));
    }
}
