#include "app_main.h"
#include "STATE.h"
#include "event.h"
#include "wifi_hsm.h"
#include "provision_hsm.h"
#include "sensor_hsm.h"
#include "debug.h"
#include "m_led.h"
#include "m_database.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static hsm_t app_hsm;

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

static bool app_root_handler(
    hsm_t *hsm,
    const event_t *event
)
{
    (void)hsm;
    (void)event;

    return false;
}

static bool app_not_configured_handler(
    hsm_t *hsm,
    const event_t *event
)
{
    if (event->id == EVT_START_PROVISION)
    {
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

        default:
            return false;
    }
}

static void app_configured_entry(hsm_t *hsm)
{
    (void)hsm;
    DEBUG_LOG("State entered: CONFIGURED");
    led_set_status(LED_CONFIGURED);

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
    event_post(&event);
}

void app_task(void *arg)
{
    (void)arg;

    event_t event;

    event_init();
    provision_hsm_init();
    wifi_hsm_init();
    sensor_hsm_init();

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

        event_t evt = {
            .id = EVT_WIFI_CONNECT_REQ
        };

        event_post(&evt);
    }
    else
    {
        DEBUG_LOG("SSID is empty -> Init NOT_CONFIGURED");
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
