#include "m_ble.h"
#include "m_database.h"
#include "debug.h"

#include <stdint.h>
#include <string.h>

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "host/ble_hs.h"
#include "host/util/util.h"

#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"


static const ble_uuid128_t s_service_uuid =
    BLE_UUID128_INIT(
        BLE_PROVISION_SERVICE_UUID_BYTES
    );


static const ble_uuid128_t s_device_id_uuid =
    BLE_UUID128_INIT(
        0x53, 0xd8, 0xa4, 0xe6,
        0xb0, 0x71,
        0x25, 0x9c,
        0x8f, 0x4d,
        0x4b, 0x6a,
        0x92, 0x2e, 0x7c, 0x3f
    );


static const ble_uuid128_t s_ssid_uuid =
    BLE_UUID128_INIT(
        0x53, 0xd8, 0xa4, 0xe6,
        0xb0, 0x71,
        0x25, 0x9c,
        0x8f, 0x4d,
        0x4b, 0x6a,
        0x93, 0x2e, 0x7c, 0x3f
    );


static const ble_uuid128_t s_password_uuid =
    BLE_UUID128_INIT(
        0x53, 0xd8, 0xa4, 0xe6,
        0xb0, 0x71,
        0x25, 0x9c,
        0x8f, 0x4d,
        0x4b, 0x6a,
        0x94, 0x2e, 0x7c, 0x3f
    );


enum
{
    BLE_CHAR_DEVICE_ID = 1,
    BLE_CHAR_SSID,
    BLE_CHAR_PASSWORD,
};


static ble_wifi_info_t s_wifi_info;
static bool s_ssid_received = false;
static bool s_password_received = false;

static m_ble_provision_cb_t s_callback = NULL;

static uint8_t s_addr_type;

static bool s_start_requested = false;
static bool s_synced = false;
static uint16_t s_conn_handle = BLE_HS_CONN_HANDLE_NONE;


static void ble_advertise(void);


static int ble_access(
    uint16_t conn_handle,
    uint16_t attr_handle,
    struct ble_gatt_access_ctxt *ctxt,
    void *arg
)
{
    (void)conn_handle;
    (void)attr_handle;

    intptr_t type = (intptr_t)arg;

    uint16_t len;


    switch (type)
    {
        case BLE_CHAR_DEVICE_ID:

            if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR)
                return BLE_ATT_ERR_UNLIKELY;

            os_mbuf_append(
                ctxt->om,
                DEVICE_ID,
                strlen(DEVICE_ID)
            );

            return 0;


        case BLE_CHAR_SSID:

            if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR)
                return BLE_ATT_ERR_UNLIKELY;

            len = OS_MBUF_PKTLEN(ctxt->om);

            if ((len == 0) ||
                (len >= BLE_SSID_SIZE))
            {
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }

            if (os_mbuf_copydata(
                    ctxt->om,
                    0,
                    len,
                    s_wifi_info.ssid) != 0)
            {
                return BLE_ATT_ERR_UNLIKELY;
            }

            s_wifi_info.ssid[len] = '\0';
            DEBUG_LOG("BLE received SSID: %s", s_wifi_info.ssid);
            s_ssid_received = true;

            if (s_ssid_received && s_password_received && s_callback != NULL)
            {
                s_callback(&s_wifi_info);
            }

            return 0;


        case BLE_CHAR_PASSWORD:
        {
            if (ctxt->op != BLE_GATT_ACCESS_OP_WRITE_CHR)
                return BLE_ATT_ERR_UNLIKELY;

            len = OS_MBUF_PKTLEN(ctxt->om);

            if ((len == 0) ||
                (len >= BLE_PASSWORD_SIZE))
            {
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }

            if (os_mbuf_copydata(
                    ctxt->om,
                    0,
                    len,
                    s_wifi_info.password) != 0)
            {
                return BLE_ATT_ERR_UNLIKELY;
            }

            s_wifi_info.password[len] = '\0';
            DEBUG_LOG("BLE received WiFi password");
            s_password_received = true;

            if (s_ssid_received && s_password_received && s_callback != NULL)
            {
                s_callback(&s_wifi_info);
            }

            return 0;
        }
    }

    return BLE_ATT_ERR_UNLIKELY;
}


static const struct ble_gatt_svc_def s_services[] =
{
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &s_service_uuid.u,

        .characteristics =
            (struct ble_gatt_chr_def[])
            {
                {
                    .uuid = &s_device_id_uuid.u,
                    .access_cb = ble_access,
                    .flags = BLE_GATT_CHR_F_READ,
                    .arg = (void *)BLE_CHAR_DEVICE_ID,
                },

                {
                    .uuid = &s_ssid_uuid.u,
                    .access_cb = ble_access,
                    .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                    .arg = (void *)BLE_CHAR_SSID,
                },

                {
                    .uuid = &s_password_uuid.u,
                    .access_cb = ble_access,
                    .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_NO_RSP,
                    .arg = (void *)BLE_CHAR_PASSWORD,
                },

                {0}
            },
    },

    {0}
};


static int ble_gap_event(
    struct ble_gap_event *event,
    void *arg
)
{
    (void)arg;

    switch (event->type)
    {
        case BLE_GAP_EVENT_CONNECT:
            DEBUG_LOG("BLE client connected, status=%d", event->connect.status);
            if (event->connect.status == 0)
            {
                s_conn_handle = event->connect.conn_handle;
                memset(&s_wifi_info, 0, sizeof(s_wifi_info));
                s_ssid_received = false;
                s_password_received = false;
            }
            else if (s_start_requested)
            {
                ble_advertise();
            }

            break;


        case BLE_GAP_EVENT_DISCONNECT:
            DEBUG_LOG("BLE client disconnected, reason=%d", event->disconnect.reason);
            s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
            memset(&s_wifi_info, 0, sizeof(s_wifi_info));
            s_ssid_received = false;
            s_password_received = false;
            if (s_start_requested)
            {
                ble_advertise();
            }

            break;


        case BLE_GAP_EVENT_ADV_COMPLETE:
            DEBUG_LOG("BLE adv complete");
            if (s_start_requested)
            {
                ble_advertise();
            }

            break;


        default:
            break;
    }

    return 0;
}


static void ble_advertise(void)
{
    struct ble_hs_adv_fields fields = {0};
    struct ble_hs_adv_fields rsp_fields = {0};
    struct ble_gap_adv_params params = {0};
    int rc;

    fields.flags =
        BLE_HS_ADV_F_DISC_GEN |
        BLE_HS_ADV_F_BREDR_UNSUP;

    fields.uuids128 =
        (ble_uuid128_t *)&s_service_uuid;

    fields.num_uuids128 = 1;

    fields.uuids128_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0)
    {
        DEBUG_LOG("ble_gap_adv_set_fields failed: %d", rc);
        return;
    }

    rsp_fields.name =
        (uint8_t *)BLE_DEVICE_NAME;

    rsp_fields.name_len =
        strlen(BLE_DEVICE_NAME);

    rsp_fields.name_is_complete = 1;

    rc = ble_gap_adv_rsp_set_fields(&rsp_fields);
    if (rc != 0)
    {
        DEBUG_LOG("ble_gap_adv_rsp_set_fields failed: %d", rc);
        return;
    }

    params.conn_mode =
        BLE_GAP_CONN_MODE_UND;

    params.disc_mode =
        BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(
        s_addr_type,
        NULL,
        BLE_HS_FOREVER,
        &params,
        ble_gap_event,
        NULL
    );

    if (rc == 0)
    {
        DEBUG_LOG("BLE advertising started: %s", BLE_DEVICE_NAME);
    }
    else
    {
        DEBUG_LOG("BLE adv start failed: rc=%d", rc);
    }
}


static void ble_on_sync(void)
{
    if (ble_hs_util_ensure_addr(0) != 0)
        return;

    if (ble_hs_id_infer_auto(
            0,
            &s_addr_type) != 0)
    {
        return;
    }

    s_synced = true;
    DEBUG_LOG("BLE host synced");

    if (s_start_requested)
    {
        ble_advertise();
    }
}


static void ble_host_task(void *arg)
{
    (void)arg;

    nimble_port_run();

    nimble_port_freertos_deinit();
}


bool m_ble_init(m_ble_provision_cb_t callback)
{
    if (callback == NULL)
        return false;

    s_callback = callback;

    memset(
        &s_wifi_info,
        0,
        sizeof(s_wifi_info)
    );
    s_ssid_received = false;
    s_password_received = false;


    if (nimble_port_init() != ESP_OK)
        return false;


    ble_svc_gap_init();
    ble_svc_gatt_init();


    if (ble_svc_gap_device_name_set(
            BLE_DEVICE_NAME) != 0)
    {
        return false;
    }


    if (ble_gatts_count_cfg(s_services) != 0)
        return false;

    if (ble_gatts_add_svcs(s_services) != 0)
        return false;


    ble_hs_cfg.sync_cb =
        ble_on_sync;


    nimble_port_freertos_init(
        ble_host_task
    );


    return true;
}


bool m_ble_start(void)
{
    memset(
        &s_wifi_info,
        0,
        sizeof(s_wifi_info)
    );
    s_ssid_received = false;
    s_password_received = false;

    s_start_requested = true;

    if (s_synced)
    {
        ble_advertise();
    }

    return true;
}


void m_ble_stop(void)
{
    s_start_requested = false;

    if (s_conn_handle != BLE_HS_CONN_HANDLE_NONE)
    {
        DEBUG_LOG("Terminating BLE connection");
        ble_gap_terminate(s_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
    }

    if (ble_gap_adv_active())
    {
        ble_gap_adv_stop();
        DEBUG_LOG("BLE advertising stopped");
    }

    memset(
        &s_wifi_info,
        0,
        sizeof(s_wifi_info)
    );
    s_ssid_received = false;
    s_password_received = false;
}
