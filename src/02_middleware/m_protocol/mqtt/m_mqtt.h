#pragma once
#include "mqtt_client.h"

#define M_MQTT_PAYLOAD_MAX 512
#define M_MQTT_TOPIC_MAX 128
typedef enum { M_MQTT_CONNECTED, M_MQTT_DISCONNECTED, M_MQTT_MESSAGE } m_mqtt_event_t;
typedef void (*m_mqtt_callback_t)(m_mqtt_event_t event, const char *topic,
                                const char *payload, size_t length, bool retained, void *context);
/* Transport only: no topics, JSON commands, timers or device policy here. */
esp_err_t m_mqtt_init(const esp_mqtt_client_config_t *config,
                      m_mqtt_callback_t callback, void *context);
esp_err_t m_mqtt_start(void);
esp_err_t m_mqtt_stop(void);
int m_mqtt_subscribe(const char *topic, int qos);
int m_mqtt_publish(const char *topic, const char *payload, int qos, bool retain);
