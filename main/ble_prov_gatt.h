#pragma once

#include "host/ble_gatt.h"

#ifdef __cplusplus
extern "C" {
#endif

/// According to cisco 32 is the wifi ssid's max length
/// https://www.cisco.com/assets/sol/sb/WAP321_Emulators/WAP321_Emulator_v1.0.0.3/help/Wireless05.html
#define WIFI_SSID_MAX_SIZE          32
/// max password lenght is 63
#define WIFI_PWD_MAX_SIZE           64
/// AWS cognito UUID(sub) mine was 32, but will set to 64 to be safe
#define AWS_UUID_MAX_SIZE           64
/// AWS Thing name
#define AWS_THING_NAME_MAX_SIZE     64

// Struct to hold data received from provisioning
struct prov_data {
    uint8_t ssid[WIFI_SSID_MAX_SIZE];
    uint8_t pwd[WIFI_PWD_MAX_SIZE];
    uint8_t aws_uuid[AWS_UUID_MAX_SIZE];
    uint8_t aws_thing[AWS_THING_NAME_MAX_SIZE];
};

/* Callback function to show which services/characteristics/descriptors get registered */
// TODO:
void ble_prov_gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);

/* Initialises gatt services */
int ble_prov_gatt_svr_init(void);

#ifdef __cplusplus
}
#endif