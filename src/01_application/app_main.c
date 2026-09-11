#include "app_main.h"
#include "STATE.h"
#include "event.h"
#include "wifi_hsm.h"
#include "debug.h"
#include "m_led.h"
#include "m_database.h"

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static char ssid[64];
static char pass[64];

static hsm_t app_hsm;

static bool app_root_handler(
    hsm_t *hsm,
    const event_t *event
);

static bool app_not_configured_handler(
    hsm_t *hsm,
    const event_t *event
);

static bool app_configured_handler(
    hsm_t *hsm,
    const event_t *event
);

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
    wifi_hsm_dispatch(event);

    if (event->id == EVT_WIFI_UP)
    {
        hsm_transition(
            hsm,
            STATE_CONFIGURED
        );
    }

    return true;
}

static bool app_configured_handler(
    hsm_t *hsm,
    const event_t *event
)
{
    switch (event->id)
    {
        case EVT_WIFI_UP:
            led_set_status(LED_ONLINE);
            wifi_hsm_dispatch(event);
            return true;

        case EVT_WIFI_DISCONNECTED:
            wifi_hsm_dispatch(event);

            hsm_transition(
                hsm,
                STATE_NOT_CONFIGURED
            );

            return true;

        default:
            wifi_hsm_dispatch(event);
            return true;
    }
}

static void app_not_configured_entry(hsm_t *hsm)
{
    (void)hsm;
    DEBUG_LOG("State entered: NOT_CONFIGURED");
    led_set_status(LED_NOT_CONFIGURED);
}

static void app_configured_entry(hsm_t *hsm)
{
    (void)hsm;
    DEBUG_LOG("State entered: CONFIGURED");
    led_set_status(LED_CONFIGURED);
    wifi_hsm_init();
}

static void app_configured_exit(hsm_t *hsm)
{
    (void)hsm;
}

void app_task(void *arg)
{
    (void)arg;

    event_t event;

    event_init();

    device_info_t info = {0};
    if (get_info(&info))
    {
        strncpy(ssid, info.ssid, sizeof(ssid) - 1);
        strncpy(pass, info.password, sizeof(pass) - 1);
    }

    if (ssid[0] == '\0')
    {
        DEBUG_LOG("SSID is empty -> Init NOT_CONFIGURED");
        hsm_init(
            &app_hsm,
            STATE_NOT_CONFIGURED
        );
    }
    else
    {
        DEBUG_LOG("SSID is configured -> Init CONFIGURED");
        hsm_init(
            &app_hsm,
            STATE_CONFIGURED
        );
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
