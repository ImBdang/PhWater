#include "sensor_hsm.h"
#include "hsm.h"
#include "m_sensor.h"
#include "debug.h"

#include "esp_timer.h"

#define SENSOR_POLL_INTERVAL_MS     2000

static hsm_t s_sensor_hsm;
static esp_timer_handle_t s_sensor_timer = NULL;

static bool sensor_root_handler(hsm_t *hsm, const event_t *event);
static bool sensor_idle_handler(hsm_t *hsm, const event_t *event);
static bool sensor_running_handler(hsm_t *hsm, const event_t *event);

static void sensor_running_entry(hsm_t *hsm);
static void sensor_running_exit(hsm_t *hsm);

static const hsm_state_t g_sensor_state_root = {
    .parent  = NULL,
    .handler = sensor_root_handler,
    .entry   = NULL,
    .exit    = NULL,
};

static const hsm_state_t g_sensor_state_idle = {
    .parent  = &g_sensor_state_root,
    .handler = sensor_idle_handler,
    .entry   = NULL,
    .exit    = NULL,
};

static const hsm_state_t g_sensor_state_running = {
    .parent  = &g_sensor_state_root,
    .handler = sensor_running_handler,
    .entry   = sensor_running_entry,
    .exit    = sensor_running_exit,
};

static void sensor_timer_callback(void *arg)
{
    (void)arg;

    event_t event = {
        .id = EVT_SENSOR_READ_REQ,
    };

    if (!event_post(&event))
    {
        DEBUG_LOG("Failed to post EVT_SENSOR_READ_REQ");
    }
}

static bool sensor_root_handler(hsm_t *hsm, const event_t *event)
{
    (void)hsm;
    (void)event;
    return false;
}

static bool sensor_idle_handler(hsm_t *hsm, const event_t *event)
{
    if (event->id == EVT_SENSOR_START_REQ)
    {
        DEBUG_LOG("Sensor HSM: START -> RUNNING");
        hsm_transition(hsm, &g_sensor_state_running);
        return true;
    }
    return false;
}

static void sensor_running_entry(hsm_t *hsm)
{
    (void)hsm;

    if (s_sensor_timer != NULL)
    {
        esp_timer_start_periodic(s_sensor_timer, (uint64_t)SENSOR_POLL_INTERVAL_MS * 1000ULL);
    }

    event_t event = {
        .id = EVT_SENSOR_READ_REQ,
    };
    event_post(&event);
}

static void sensor_running_exit(hsm_t *hsm)
{
    (void)hsm;

    if (s_sensor_timer != NULL)
    {
        esp_timer_stop(s_sensor_timer);
    }
}

static bool sensor_running_handler(hsm_t *hsm, const event_t *event)
{
    switch (event->id)
    {
        case EVT_SENSOR_STOP_REQ:
            DEBUG_LOG("Sensor HSM: STOP -> IDLE");
            hsm_transition(hsm, &g_sensor_state_idle);
            return true;

        case EVT_SENSOR_READ_REQ:
        {
            sensor_sample_t sample = {0};
            esp_err_t ret = m_sensor_read(&sample);
            if (ret == ESP_OK)
            {
                DEBUG_LOG("Sensor raw_avg=%d | ADC=%d mV | PO=%.3f V",
                          sample.raw, sample.adc_mv, sample.po_voltage);

                event_t update_evt = {
                    .id = EVT_PH_VOLTAGE_UPDATE,
                    .data.voltage = sample.po_voltage,
                };
                if (!event_post(&update_evt))
                {
                    DEBUG_LOG("Failed to post EVT_PH_VOLTAGE_UPDATE");
                }
            }
            else
            {
                DEBUG_LOG("Sensor read error: %s", esp_err_to_name(ret));

                event_t err_evt = {
                    .id = EVT_PH_ERROR,
                    .data.error = ret,
                };
                if (!event_post(&err_evt))
                {
                    DEBUG_LOG("Failed to post EVT_PH_ERROR");
                }
            }
            return true;
        }

        default:
            return false;
    }
}

void sensor_hsm_init(void)
{
    if (s_sensor_timer == NULL)
    {
        const esp_timer_create_args_t timer_args = {
            .callback = sensor_timer_callback,
            .arg = NULL,
            .name = "sensor_tmr",
        };
        esp_timer_create(&timer_args, &s_sensor_timer);
    }

    hsm_init(&s_sensor_hsm, &g_sensor_state_idle);
    DEBUG_LOG("Sensor HSM initialized -> IDLE");
}

void sensor_hsm_dispatch(const event_t *event)
{
    if (event != NULL)
    {
        hsm_dispatch(&s_sensor_hsm, event);
    }
}
