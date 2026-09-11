#ifndef __EVENT__ 
#define __EVENT__

#include "stdint.h"
#include "stdbool.h"

#define EVENT_BUFF    64

typedef enum
{
    EVT_START = 0,

    EVT_START_PROVISION,
    EVT_PROVISIONED,

    EVT_WIFI_CONNECT_REQ,
    EVT_WIFI_CONNECTED,
    EVT_WIFI_UP,
    EVT_WIFI_DISCONNECTED,

    EVT_MQTT_CONNECTED,
    EVT_MQTT_DISCONNECTED,

    EVT_PH_UPDATE,
    EVT_PH_ERROR,

} event_id_t; 

typedef struct
{
    event_id_t id;

    union
    {
        float ph;
        int32_t error;
        uint32_t value;
    } data;

} event_t;


bool event_init(void);

bool event_post(const event_t *event);

bool event_get(event_t *event);

#endif
