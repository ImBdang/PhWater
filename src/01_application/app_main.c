#include "app_main.h"
#include "STATE.h"
#include "event.h"
#include "wifi_hsm.h"
#include "provision_hsm.h"
#include "sensor_hsm.h"
#include "debug.h"
#include "m_led.h"
#include "m_button.h"
#include "m_database.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static hsm_t app_hsm;

static void app_button_event_cb(m_button_event_t event);

static bool app_root_handler(hsm_t *hsm, const event_t *event);
static bool app_not_configured_handler(hsm_t *hsm, const event_t *event);
static bool app_configured_handler(hsm_t *hsm, const event_t *event);

static void app_not_configured_entry(hsm_t *hsm);
static void app_configured_entry(hsm_t *hsm);
static void app_configured_exit(hsm_t *hsm);

const hsm_state_t g_state_root = {
    .parent  = NULL,
    .handler = app_root_handler,
    .entry   = NULL,
    .exit    = NULL,
};

const hsm_state_t g_state_not_configured = {
    .parent  = STATE_ROOT,
    .handler = app_not_configured_handler,
    .entry   = app_not_configured_entry,
    .exit    = NULL,
};

const hsm_state_t g_state_configured = {
    .parent  = STATE_ROOT,
    .handler = app_configured_handler,
    .entry   = app_configured_entry,
    .exit    = app_configured_exit,
};

static void app_button_event_cb(m_button_event_t event)
{
    if (event == M_BUTTON_EVENT_LONG_PRESS)
    {
        DEBUG_LOG("Button LONG_PRESS callback -> Posting EVT_START_PROVISION");
        event_t evt = {
            .id = EVT_START_PROVISION,
        };
        event_post(&evt);
    }
}

static bool app_root_handler(
    hsm_t *hsm,
    const event_t *event
)
{
    (void)hsm;

    switch (event->id)
    {
        case EVT_WIFI_CONNECT_REQ:
        case EVT_WIFI_RECONFIGURE_REQ:
        case EVT_WIFI_CONNECTED:
        case EVT_WIFI_UP:
        case EVT_WIFI_DISCONNECTED:
            wifi_hsm_dispatch(event);
            return true;

        default:
            return false;
    }
}

static bool app_not_configured_handler(
    hsm_t *hsm,
    const event_t *event
)
{
    if (event->id == EVT_START_PROVISION)
    {
        provision_hsm_set_origin(PROVISION_ORIGIN_NOT_CONFIGURED);
        hsm_transition(
            hsm,
            STATE_PROVISIONING
        );

        return true;
    }

    return false;
}

static void app_not_configured_entry(hsm_t *hsm)
{
    (void)hsm;
    DEBUG_LOG("State entered: NOT_CONFIGURED");
    led_set_status(LED_NOT_CONFIGURED);
}

static bool app_configured_handler(
    hsm_t *hsm,
    const event_t *event
)
{
    switch (event->id)
    {
        case EVT_WIFI_CONNECT_REQ:
        case EVT_WIFI_RECONFIGURE_REQ:
        case EVT_WIFI_CONNECTED:
            wifi_hsm_dispatch(event);
            return true;

        case EVT_WIFI_UP:
            led_set_status(LED_ONLINE);
            wifi_hsm_dispatch(event);
            return true;

        case EVT_WIFI_DISCONNECTED:
            led_set_status(LED_CONFIGURED);
            wifi_hsm_dispatch(event);
            return true;

        case EVT_SENSOR_START_REQ:
        case EVT_SENSOR_STOP_REQ:
        case EVT_SENSOR_READ_REQ:
            sensor_hsm_dispatch(event);
            return true;

        case EVT_PH_VOLTAGE_UPDATE:
            DEBUG_LOG("PH voltage update: %.3f V", event->data.voltage);
            return true;

        case EVT_PH_ERROR:
            DEBUG_LOG("PH read error: %ld", (long)event->data.error);
            return true;

        case EVT_START_PROVISION:
            DEBUG_LOG("EVT_START_PROVISION -> Transition CONFIGURED to PROVISIONING");
            provision_hsm_set_origin(PROVISION_ORIGIN_CONFIGURED);
            hsm_transition(hsm, STATE_PROVISIONING);
            return true;

        default:
            return false;
    }
}

static void app_configured_entry(hsm_t *hsm)
{
    (void)hsm;
    DEBUG_LOG("State entered: CONFIGURED");

    device_info_t info = {0};
    if (get_info(&info) && (info.ssid[0] != '\0'))
    {
        wifi_hsm_set_credentials(info.ssid, info.password);
    }

    if (wifi_hsm_is_online())
    {
        led_set_status(LED_ONLINE);
    }
    else
    {
        led_set_status(LED_CONFIGURED);
        event_t wifi_evt = {
            .id = EVT_WIFI_RECONFIGURE_REQ,
        };
        event_post(&wifi_evt);
    }

    event_t event = {
        .id = EVT_SENSOR_START_REQ,
    };
    event_post(&event);
}

static void app_configured_exit(hsm_t *hsm)
{
    (void)hsm;
    DEBUG_LOG("State exit: CONFIGURED");

    event_t event = {
        .id = EVT_SENSOR_STOP_REQ,
    };
    sensor_hsm_dispatch(&event);
}

void app_task(void *arg)
{
    (void)arg;

    event_t event;

    if (!event_init())
    {
        DEBUG_LOG("Fatal: event_init failed");
        ESP_ERROR_CHECK(ESP_FAIL);
    }

    provision_hsm_init();
    wifi_hsm_init();

    ESP_ERROR_CHECK(sensor_hsm_init());
    ESP_ERROR_CHECK(m_button_init(app_button_event_cb));

    device_info_t info = {0};
    if (get_info(&info) && (info.ssid[0] != '\0'))
    {
        wifi_hsm_set_credentials(
            info.ssid,
            info.password
        );

        DEBUG_LOG("SSID is configured -> Init CONFIGURED");
        hsm_init(
            &app_hsm,
            STATE_CONFIGURED
        );
        /* Note: app_configured_entry() called during hsm_init() already posts EVT_WIFI_CONNECT_REQ */
    }
    else
    {
        DEBUG_LOG("SSID is empty -> Init NOT_CONFIGURED");
        provision_hsm_set_origin(PROVISION_ORIGIN_NOT_CONFIGURED);
        hsm_init(
            &app_hsm,
            STATE_NOT_CONFIGURED
        );

        event_t evt = {
            .id = EVT_START_PROVISION
        };

        event_post(&evt);
    }

    while (1)
    {
        if (event_get(&event))
        {
            hsm_dispatch(
                &app_hsm,
                &event
            );
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
