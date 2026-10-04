#include "m_mqtt.h"
#include <string.h>
#include "esp_log.h"

static esp_mqtt_client_handle_t s_client;
static m_mqtt_callback_t s_callback;
static void *s_context;
static char s_topic[M_MQTT_TOPIC_MAX], s_payload[M_MQTT_PAYLOAD_MAX];
static int s_received;
static bool s_valid, s_retained;

static void on_event(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    (void)arg; (void)base;
    esp_mqtt_event_handle_t event = data;
    if (id == MQTT_EVENT_CONNECTED || id == MQTT_EVENT_DISCONNECTED) {
        s_callback(id == MQTT_EVENT_CONNECTED ? M_MQTT_CONNECTED : M_MQTT_DISCONNECTED,
                   NULL, NULL, 0, false, s_context);
    } else if (id == MQTT_EVENT_DATA) {
        if (event->current_data_offset == 0) {
            s_received = 0;
            s_retained = event->retain;
            s_valid = event->topic_len > 0 && event->topic_len < sizeof(s_topic)
                && event->total_data_len > 0 && event->total_data_len < sizeof(s_payload);
            if (s_valid) {
                memcpy(s_topic, event->topic, event->topic_len);
                s_topic[event->topic_len] = 0;
            }
        }
        if (!s_valid || event->current_data_offset != s_received
            || s_received + event->data_len >= sizeof(s_payload)) {
            s_valid = false;
            return;
        }
        memcpy(s_payload + s_received, event->data, event->data_len);
        s_received += event->data_len;
        if (s_received == event->total_data_len) {
            s_payload[s_received] = 0;
            s_callback(M_MQTT_MESSAGE, s_topic, s_payload, s_received, s_retained, s_context);
            s_valid = false;
        }
    } else if (id == MQTT_EVENT_ERROR) {
        /* Never log credentials or URI userinfo. */
        ESP_LOGW("m_mqtt", "Transport error type=%d", event->error_handle->error_type);
    }
}

esp_err_t m_mqtt_init(const esp_mqtt_client_config_t *config,
                      m_mqtt_callback_t callback, void *context)
{
    if (!config || !callback || s_client) return ESP_ERR_INVALID_ARG;
    s_callback = callback; s_context = context;
    s_client = esp_mqtt_client_init(config);
    if (!s_client) return ESP_ERR_NO_MEM;
    return esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, on_event, NULL);
}
esp_err_t m_mqtt_start(void) { return s_client ? esp_mqtt_client_start(s_client) : ESP_ERR_INVALID_STATE; }
esp_err_t m_mqtt_stop(void) { return s_client ? esp_mqtt_client_stop(s_client) : ESP_ERR_INVALID_STATE; }
int m_mqtt_subscribe(const char *topic, int qos) { return esp_mqtt_client_subscribe(s_client, topic, qos); }
int m_mqtt_publish(const char *topic, const char *payload, int qos, bool retain)
{
    return esp_mqtt_client_enqueue(s_client, topic, payload, 0, qos, retain, true);
}
