#include "nimble/ble.h"
#include "modlog/modlog.h"
#include "esp_log.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/util/util.h"
#include "host/ble_hs_pvcy.h"
#include "ble_prov.h"
#include "ble_utils.h"
#include "ble_prov_gatt.h"

static const char *device_name = CONFIG_BLE_DEVICE_NAME;
static uint8_t ble_prov_addr_type = BLE_OWN_ADDR_RANDOM;

/* Stores connection */
static uint16_t conn_handle;

static void ble_prov_manager_host_task(void *param);
static void ble_prov_on_reset(int reason);
static void ble_prov_on_sync(void);
static void ble_app_set_addr(void);
static void ble_prov_advertise();

/*
    Handles gap events
*/
static int ble_prov_gap_event(struct ble_gap_event *event, void *arg);

/*
    Starts ble
*/
void start_ble()
{
    int rc;
    esp_err_t ret;

    ret = nimble_port_init();
    if (ret != ESP_OK) {
        MODLOG_DFLT(ERROR, "Failed to init nimble %d \n", ret);
        return;
    }

    /* Initialize the NimBLE host configuration */
    ble_hs_cfg.sync_cb = ble_prov_on_sync;
    ble_hs_cfg.reset_cb = ble_prov_on_reset;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    // Security
    ble_hs_cfg.sm_io_cap = BLE_HS_IO_NO_INPUT_OUTPUT;
    /* Man-In-The-Middle protection */
    ble_hs_cfg.sm_mitm = 1;
    // Security Manager Secure Connections
    ble_hs_cfg.sm_sc = 1;

    // // Enable bonding
    // ble_hs_cfg.sm_bonding = 1;
    // ble_hs_cfg.sm_our_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    // ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;

    /* Initialize gatt */
    rc = ble_prov_gatt_svr_init();
    assert(rc == 0);

    nimble_port_freertos_init(ble_prov_manager_host_task);
}

/*
    Stops ble
*/
void stop_ble()
{
    int rc = nimble_port_stop();
    if (rc != 0)
    {
        ESP_LOGE(TAG, "Error at nimble port stop");
        return;
    }

    nimble_port_deinit();
}

static void ble_prov_manager_host_task(void *param)
{
    (void)param;

    ESP_LOGI(TAG, "BLE Host Task Started");
    /* This function will return only when nimble_port_stop() is executed */
    nimble_port_run();

    nimble_port_freertos_deinit();
}

static void ble_prov_on_reset(int reason)
{
    MODLOG_DFLT(ERROR, "Resetting state; reason=%d\n", reason);
}

static void ble_prov_on_sync(void)
{
    int rc;

    /* Generate a non-resolvable private address. */
    ble_app_set_addr();

    /* Make sure we have proper identity address set (public preferred) */
    // ble_hs_pvcy_rpa_config(1);
    rc = ble_hs_util_ensure_addr(1);

    /* Figure out address to use while advertising */
    rc = ble_hs_id_infer_auto(0, &ble_prov_addr_type);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error determining address type; rc=%d\n", rc);
        return;
    }

    /* For printing purposes */
    uint8_t addr_val[6] = {0};
    rc = ble_hs_id_copy_addr(ble_prov_addr_type, addr_val, NULL);

    MODLOG_DFLT(INFO, "Device Address: ");
    print_addr(addr_val);
    MODLOG_DFLT(INFO, "\n");

    /* Begin advertising */
    ble_prov_advertise();
}

static void ble_app_set_addr(void)
{
    ble_addr_t addr;
    int rc;

    /* generate new non-resolvable private address */
    rc = ble_hs_id_gen_rnd(0, &addr);
    assert(rc == 0);

    /* set generated address */
    rc = ble_hs_id_set_rnd(addr.val);

    assert(rc == 0);
}

static void ble_prov_advertise()
{
    struct ble_gap_adv_params adv_params;
    struct ble_hs_adv_fields fields;
    struct ble_hs_adv_fields scan_rsp_fields;
    int rc;

    /*
     *  Set the advertisement data included in our advertisements:
     *     o Flags (indicates advertisement type and other general info)
     *     o Advertising tx power
     *     o Device name
     */
    memset(&fields, 0, sizeof(fields));

    /*
     * Advertise two flags:
     *      o Discoverability in forthcoming advertisement (general)
     *      o BLE-only (BR/EDR unsupported)
     */
    fields.flags = BLE_HS_ADV_F_DISC_GEN |
                   BLE_HS_ADV_F_BREDR_UNSUP;

    /*
     * Indicate that the TX power level field should be included; have the
     * stack fill this value automatically.  This is done by assigning the
     * special value BLE_HS_ADV_TX_PWR_LVL_AUTO.
     */
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    fields.name = (uint8_t *)device_name;
    fields.name_len = strlen(device_name);
    fields.name_is_complete = 1;

    /// Set scan response data to advertise 128 bit uuid's
    memset(&scan_rsp_fields, 0, sizeof(scan_rsp_fields));

    scan_rsp_fields.uuids128 = (ble_uuid128_t[]) {
        PROV_SENSOR_SERVICE
    };
    scan_rsp_fields.num_uuids128 = 1;
    scan_rsp_fields.uuids128_is_complete = 1;

    rc = ble_gap_adv_rsp_set_fields(&scan_rsp_fields);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error setting scan response advertisement data; rc=%d\n", rc);
        return;
    }

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error setting advertisement data; rc=%d\n", rc);
        return;
    }

    /* Begin advertising */
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    rc = ble_gap_adv_start(ble_prov_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, ble_prov_gap_event, NULL);
    if (rc != 0) {
        MODLOG_DFLT(ERROR, "error enabling advertisement; rc=%d\n", rc);
        return;
    }

    MODLOG_DFLT(INFO, "Started advertising.");
}

static int ble_prov_gap_event(struct ble_gap_event *event, void *arg)
{
    struct ble_gap_conn_desc desc;
    int rc;

    switch (event->type) {
        case BLE_GAP_EVENT_CONNECT:
            /* A new connection was established or a connection attempt failed */
            /* log connection status */
            MODLOG_DFLT(INFO, "connection %s; status=%d\n",
                        event->connect.status == 0 ? "established" : "failed",
                        event->connect.status);

            /* Print connection information if it was successful */
            if (event->connect.status == 0) {
                rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
                assert(rc == 0);
                bleprph_print_conn_desc(&desc);
            }
            MODLOG_DFLT(INFO, "\n");

            if (event->connect.status != 0) {
                /* Connection failed; resume advertising */
                ble_prov_advertise();
            }
            conn_handle = event->connect.conn_handle;
            break;

        case BLE_GAP_EVENT_DISCONNECT:
            /* log connection status */
            MODLOG_DFLT(INFO, "disconnect; reason=%d\n", event->disconnect.reason);
            bleprph_print_conn_desc(&event->disconnect.conn);

            /* Connection terminated; resume advertising */
            ble_prov_advertise();
            break;

        case BLE_GAP_EVENT_CONN_UPDATE:
            /* The central has updated the connection parameters. */
            MODLOG_DFLT(INFO, "connection updated; status=%d ",
                        event->conn_update.status);
            rc = ble_gap_conn_find(event->conn_update.conn_handle, &desc);
            assert(rc == 0);
            bleprph_print_conn_desc(&desc);
            MODLOG_DFLT(INFO, "\n");
            break;

        case BLE_GAP_EVENT_ENC_CHANGE:
            /* Encryption has been enabled or disabled for this connection. */
            MODLOG_DFLT(INFO, "encryption change event; status=%d ",
                        event->enc_change.status);
            rc = ble_gap_conn_find(event->enc_change.conn_handle, &desc);
            assert(rc == 0);
            bleprph_print_conn_desc(&desc);
            MODLOG_DFLT(INFO, "\n");
            break;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            /* Advertisement complete or ble was stopped */
            if(ble_hs_is_enabled() == 1) {
                /* Advertisement completed */
                MODLOG_DFLT(INFO, "adv complete\n");
                ble_prov_advertise();
            } else {
                MODLOG_DFLT(INFO, "Ble stopped\n");
            }
            break;

        case BLE_GAP_EVENT_SUBSCRIBE:
            MODLOG_DFLT(INFO, "Subscribe event;");
            break;

        case BLE_GAP_EVENT_MTU:
            MODLOG_DFLT(INFO, "mtu update event; conn_handle=%d mtu=%d\n",
                        event->mtu.conn_handle,
                        event->mtu.value);
            break;
    }

    return 0;
}