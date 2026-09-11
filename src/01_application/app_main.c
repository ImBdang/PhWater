#include "app_main.h"
#include "STATE.h"
#include "event.h"
#include "wifi_hsm.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define APP_TASK_STACK_SIZE    2048
#define APP_TASK_PRIORITY      5

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
    switch (event->id)
    {
        case EVT_CONFIG_DONE:
            hsm_transition(
                hsm,
                STATE_CONFIGURED
            );
            return true;

        default:
            return false;
    }
}

static bool app_configured_handler(
    hsm_t *hsm,
    const event_t *event
)
{
    switch (event->id)
    {
        case EVT_CONFIG_LOST:
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
}

static void app_configured_entry(hsm_t *hsm)
{
    (void)hsm;

    wifi_hsm_init();
}

static void app_configured_exit(hsm_t *hsm)
{
    (void)hsm;
}

static void app_task(void *arg)
{
    (void)arg;

    event_t event;

    hsm_init(
        &app_hsm,
        STATE_NOT_CONFIGURED
    );

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

bool app_start(void)
{
    BaseType_t ret = xTaskCreate(
        app_task,
        "app_task",
        APP_TASK_STACK_SIZE,
        NULL,
        APP_TASK_PRIORITY,
        NULL
    );

    return ret == pdPASS;
}
