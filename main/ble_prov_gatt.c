#include <string.h>
#include "nimble/ble.h"
#include "modlog/modlog.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "ble_prov_gatt.h"
#include "ble_prov.h"
#include "wifi.h"

/**
 * 
 *  TODO: Describe services and characteristics
 * 
*/

/// Holds data gotten through ble
static struct prov_data pdata;

static int gatt_svr_prov_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt *ctxt,
                             void *arg);
                             
static int gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len, uint16_t max_len,
                   void *dst, uint16_t *len);

/* Sensor provisioning service */
/* c4b21f63-e59e-44af-a637-5c05f58ac6a3 */
static const ble_uuid128_t gatt_svr_svc_prov_uuid = PROV_SENSOR_SERVICE;

/* wifi ssid characteristic */
/* c4b21f63-e59e-44af-a637-5c05f58ac6a4 */
static const ble_uuid128_t gatt_svr_char_wifi_ssid_uuid =
    BLE_UUID128_INIT(0xa4, 0xc6, 0x8a, 0xf5, 0x05, 0x5c, 0x37, 0xa6,
                     0xaf, 0x44, 0x9e, 0xe5, 0x63, 0x1f, 0xb2, 0xc4);

/* wifi password characteristic */
/* c4b21f63-e59e-44af-a637-5c05f58ac6a5 */
static const ble_uuid128_t gatt_svr_char_wifi_pwd_uuid =
    BLE_UUID128_INIT(0xa5, 0xc6, 0x8a, 0xf5, 0x05, 0x5c, 0x37, 0xa6,
                     0xaf, 0x44, 0x9e, 0xe5, 0x63, 0x1f, 0xb2, 0xc4);

/* aws account name characteristic */
/* c4b21f63-e59e-44af-a637-5c05f58ac6a6 */
static const ble_uuid128_t gatt_svr_char_aws_acc_name_uuid =
    BLE_UUID128_INIT(0xa6, 0xc6, 0x8a, 0xf5, 0x05, 0x5c, 0x37, 0xa6,
                     0xaf, 0x44, 0x9e, 0xe5, 0x63, 0x1f, 0xb2, 0xc4);

/* aws thing name characteristic */
/* c4b21f63-e59e-44af-a637-5c05f58ac6a7 */
static const ble_uuid128_t gatt_svr_char_aws_thing_name_uuid =
    BLE_UUID128_INIT(0xa7, 0xc6, 0x8a, 0xf5, 0x05, 0x5c, 0x37, 0xa6,
                     0xaf, 0x44, 0x9e, 0xe5, 0x63, 0x1f, 0xb2, 0xc4);

/* Provisioning complete characteristic */
/* c4b21f63-e59e-44af-a637-5c05f58ac6a8 */
static const ble_uuid128_t gatt_svr_char_prov_cpl_uuid =
    BLE_UUID128_INIT(0xa8, 0xc6, 0x8a, 0xf5, 0x05, 0x5c, 0x37, 0xa6,
                     0xaf, 0x44, 0x9e, 0xe5, 0x63, 0x1f, 0xb2, 0xc4);


static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /*** Service: Sensor provisioning. */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &gatt_svr_svc_prov_uuid.u,
        .characteristics = (struct ble_gatt_chr_def[])
        { {
                /*** Characteristic: Wifi ssid. */
                .uuid = &gatt_svr_char_wifi_ssid_uuid.u,
                .access_cb = gatt_svr_prov_access_cb,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC,
            }, {
                /*** Characteristic: Wifi password. */
                .uuid = &gatt_svr_char_wifi_pwd_uuid.u,
                .access_cb = gatt_svr_prov_access_cb,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC,
            }, {
                /*** Characteristic: AWS account name. */
                .uuid = &gatt_svr_char_aws_acc_name_uuid.u,
                .access_cb = gatt_svr_prov_access_cb,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC,
            }, {
                /*** Characteristic: AWS thing name. */
                .uuid = &gatt_svr_char_aws_thing_name_uuid.u,
                .access_cb = gatt_svr_prov_access_cb,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC,
            }, {
                /*** Characteristic: Provisioning complete. */
                .uuid = &gatt_svr_char_prov_cpl_uuid.u,
                .access_cb = gatt_svr_prov_access_cb,
                .flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_WRITE_ENC,
            }, {
                0, /* No more characteristics in this service. */
            }
        },
    },

    {
        0, /* No more services. */
    },
};

/**
 * 
 * Writes to characteristic
 * 
*/
static int gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len, uint16_t max_len,
                   void *dst, uint16_t *len)
{
    uint16_t om_len;
    int rc;

    memset(dst, 0, max_len);

    om_len = OS_MBUF_PKTLEN(om);
    if (om_len < min_len || om_len > max_len) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
    if (rc != 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    return 0;
}

static int gatt_svr_prov_access_cb(uint16_t conn_handle, uint16_t attr_handle,
                             struct ble_gatt_access_ctxt *ctxt,
                             void *arg)
{
    const ble_uuid_t *uuid;
    int rc;
    static int cpl = 0;

    uuid = ctxt->chr->uuid;

    /* Determine which characteristic is being accessed by examining its
     * 128-bit UUID.
     */
    if(ble_uuid_cmp(uuid, &gatt_svr_char_wifi_ssid_uuid.u) == 0) {
        /// Write wifi ssid to buffer
        rc = gatt_svr_chr_write(ctxt->om, 1, sizeof pdata.ssid, pdata.ssid, NULL);
        return rc; // return return code of write top buffer operation
    } else if(ble_uuid_cmp(uuid, &gatt_svr_char_wifi_pwd_uuid.u) == 0) {
        /// Write wifi password to buffer
        rc = gatt_svr_chr_write(ctxt->om, 1, sizeof pdata.pwd, pdata.pwd, NULL);
        return rc;
    } else if(ble_uuid_cmp(uuid, &gatt_svr_char_aws_acc_name_uuid.u) == 0) {
        /// Write aws uuid to buffer
        rc = gatt_svr_chr_write(ctxt->om, 1, sizeof pdata.aws_uuid, pdata.aws_uuid, NULL);
        return rc;
    } else if(ble_uuid_cmp(uuid, &gatt_svr_char_aws_thing_name_uuid.u) == 0) {
        /// Write aws thing name to buffer
        rc = gatt_svr_chr_write(ctxt->om, 1, sizeof pdata.aws_thing, pdata.aws_thing, NULL);
        return rc;
    } else if(ble_uuid_cmp(uuid, &gatt_svr_char_prov_cpl_uuid.u) == 0) {
        rc = gatt_svr_chr_write(ctxt->om, 1, sizeof cpl, &cpl, NULL);

        if(cpl > 0) {
            // Save provisioning data and attempt wifi connection

            /// TODO: REMOVE PRINT
            printf("COMPLETE?: %d\n", cpl);
            printf("SSID: %s\n", pdata.ssid);
            printf("PWD: %s\n", pdata.pwd);
            printf("THING: %s\n", pdata.aws_thing);

            // Test wifi data if relevant fields are not empty
            if(pdata.ssid[0] != '\0' && pdata.aws_thing[0] != '\0') {
                // Test wifi credentials, a new task will be create for this as 
                // we want this callback function to return
                xTaskCreate(wifi_task, "wifi_task", 4096, &pdata, 10, NULL);
            } else {
                /// TODO: INFORM USER THAT SOME REQUIRED FIELD ARE EMPTY, 
                /// can be achieved using for example a ble notification
            }
        }

        return rc;
    } else {
        MODLOG_DFLT(INFO, "Unknown characteristic");
        assert(0);
        return BLE_ATT_ERR_UNLIKELY;
    }

    /*
     * Should not get here
     */
    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

int ble_prov_gatt_svr_init(void)
{
    int rc;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}