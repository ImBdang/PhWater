#include "wifi_hsm.h"
#include "hsm.h"
#include "m_wifi.h"
#include "debug.h"

#include <string.h>

#include "esp_wifi.h"
#include "esp_event.h"

static char s_ssid[64] = {0};
static char s_pass[64] = {0};
static bool s_reconfigure_pending = false;

static hsm_t s_wifi_hsm;

static bool wifi_root_handler(hsm_t *hsm, const event_t *event);
static bool wifi_idle_handler(hsm_t *hsm, const event_t *event);
static bool wifi_connecting_handler(hsm_t *hsm, const event_t *event);
static bool wifi_connected_handler(hsm_t *hsm, const event_t *event);
static bool wifi_online_handler(hsm_t *hsm, const event_t *event);

static void wifi_connecting_entry(hsm_t *hsm);

static const hsm_state_t g_wifi_state_root = {
    .parent  = NULL,
    .handler = wifi_root_handler,
    .entry   = NULL,
    .exit    = NULL,
};

static const hsm_state_t g_wifi_state_idle = {
    .parent  = &g_wifi_state_root,
    .handler = wifi_idle_handler,
    .entry   = NULL,
    .exit    = NULL,
};

static const hsm_state_t g_wifi_state_connecting = {
    .parent  = &g_wifi_state_root,
    .handler = wifi_connecting_handler,
    .entry   = wifi_connecting_entry,
    .exit    = NULL,
};

static const hsm_state_t g_wifi_state_connected = {
    .parent  = &g_wifi_state_root,
    .handler = wifi_connected_handler,
    .entry   = NULL,
    .exit    = NULL,
};

static const hsm_state_t g_wifi_state_online = {
    .parent  = &g_wifi_state_root,
    .handler = wifi_online_handler,
    .entry   = NULL,
    .exit    = NULL,
};

static bool wifi_root_handler(hsm_t *hsm, const event_t *event)
{
    (void)hsm;
    (void)event;
    return false;
}

static bool wifi_idle_handler(hsm_t *hsm, const event_t *event)
{
    switch (event->id)
    {
        case EVT_WIFI_CONNECT_REQ:
        case EVT_WIFI_RECONFIGURE_REQ:
            DEBUG_LOG("WiFi HSM: Connect/Reconfigure REQ in IDLE -> CONNECTING");
            s_reconfigure_pending = false;
            hsm_transition(hsm, &g_wifi_state_connecting);
            return true;

        default:
            return false;
    }
}

static void wifi_connecting_entry(hsm_t *hsm)
{
    (void)hsm;
    DEBUG_LOG("WiFi state entered: CONNECTING (SSID=%s)", s_ssid);
    if (s_ssid[0] != '\0')
    {
        m_wifi_connect(s_ssid, s_pass);
    }
}

static bool wifi_connecting_handler(hsm_t *hsm, const event_t *event)
{
    switch (event->id)
    {
        case EVT_WIFI_CONNECT_REQ:
            DEBUG_LOG("WiFi HSM: Already connecting");
            return true;

        case EVT_WIFI_RECONFIGURE_REQ:
            DEBUG_LOG("WiFi HSM: RECONFIGURE_REQ in CONNECTING");
            s_reconfigure_pending = true;
            if (!m_wifi_disconnect())
            {
                s_reconfigure_pending = false;
                if (s_ssid[0] != '\0')
                {
                    m_wifi_connect(s_ssid, s_pass);
                }
            }
            return true;

        case EVT_WIFI_CONNECTED:
            DEBUG_LOG("WiFi state: CONNECTED (associated)");
            s_reconfigure_pending = false;
            hsm_transition(hsm, &g_wifi_state_connected);
            return true;

        case EVT_WIFI_DISCONNECTED:
            DEBUG_LOG("WiFi state: DISCONNECTED while connecting");
            if (s_reconfigure_pending)
            {
                s_reconfigure_pending = false;
                DEBUG_LOG("WiFi state: Applying new credentials after disconnect");
                if (s_ssid[0] != '\0')
                {
                    m_wifi_connect(s_ssid, s_pass);
                }
            }
            return true;

        default:
            return false;
    }
}

static bool wifi_connected_handler(hsm_t *hsm, const event_t *event)
{
    switch (event->id)
    {
        case EVT_WIFI_CONNECT_REQ:
            DEBUG_LOG("WiFi HSM: Already connecting/connected");
            return true;

        case EVT_WIFI_RECONFIGURE_REQ:
            DEBUG_LOG("WiFi HSM: RECONFIGURE_REQ in CONNECTED -> disconnecting");
            s_reconfigure_pending = true;
            if (!m_wifi_disconnect())
            {
                s_reconfigure_pending = false;
                hsm_transition(hsm, &g_wifi_state_connecting);
            }
            return true;

        case EVT_WIFI_UP:
            DEBUG_LOG("WiFi state: ONLINE (IP acquired)");
            hsm_transition(hsm, &g_wifi_state_online);
            return true;

        case EVT_WIFI_DISCONNECTED:
            DEBUG_LOG("WiFi state: DISCONNECTED before IP -> CONNECTING");
            s_reconfigure_pending = false;
            hsm_transition(hsm, &g_wifi_state_connecting);
            return true;

        default:
            return false;
    }
}

static bool wifi_online_handler(hsm_t *hsm, const event_t *event)
{
    switch (event->id)
    {
        case EVT_WIFI_CONNECT_REQ:
            DEBUG_LOG("WiFi HSM: Already ONLINE");
            return true;

        case EVT_WIFI_RECONFIGURE_REQ:
            DEBUG_LOG("WiFi HSM: RECONFIGURE_REQ in ONLINE -> disconnecting");
            s_reconfigure_pending = true;
            if (!m_wifi_disconnect())
            {
                s_reconfigure_pending = false;
                hsm_transition(hsm, &g_wifi_state_connecting);
            }
            return true;

        case EVT_WIFI_DISCONNECTED:
            DEBUG_LOG("WiFi state: DISCONNECTED from ONLINE -> CONNECTING");
            s_reconfigure_pending = false;
            hsm_transition(hsm, &g_wifi_state_connecting);
            return true;

        default:
            return false;
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    (void)arg;
    (void)event_data;

    if (event_base == WIFI_EVENT)
    {
        if (event_id == WIFI_EVENT_STA_CONNECTED)
        {
            event_t evt = { .id = EVT_WIFI_CONNECTED };
            event_post(&evt);
        }
        else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
        {
            event_t evt = { .id = EVT_WIFI_DISCONNECTED };
            event_post(&evt);
        }
    }
    else if (event_base == IP_EVENT)
    {
        if (event_id == IP_EVENT_STA_GOT_IP)
        {
            event_t evt = { .id = EVT_WIFI_UP };
            event_post(&evt);
        }
    }
}

bool wifi_hsm_set_credentials(const char *ssid, const char *password)
{
    if (ssid == NULL || password == NULL)
    {
        return false;
    }

    strncpy(s_ssid, ssid, sizeof(s_ssid) - 1);
    s_ssid[sizeof(s_ssid) - 1] = '\0';

    strncpy(s_pass, password, sizeof(s_pass) - 1);
    s_pass[sizeof(s_pass) - 1] = '\0';

    return true;
}

void wifi_hsm_init(void)
{
    m_wifi_init();

    esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        wifi_event_handler,
        NULL,
        NULL
    );

    esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        wifi_event_handler,
        NULL,
        NULL
    );

    hsm_init(&s_wifi_hsm, &g_wifi_state_idle);
    DEBUG_LOG("WiFi HSM initialized -> IDLE");
}

void wifi_hsm_dispatch(const event_t *event)
{
    if (event != NULL)
    {
        hsm_dispatch(&s_wifi_hsm, event);
    }
}

bool wifi_hsm_is_online(void)
{
    return (s_wifi_hsm.current == &g_wifi_state_online);
}
