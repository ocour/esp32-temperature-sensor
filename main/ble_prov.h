#ifndef H_BLE_PROV_
#define H_BLE_PROV_

#include "stdint.h"

// BLE
#include "host/ble_hs.h"

#define TAG             "BLE_PROV"
#define DEVICE_NAME     "DEVICE_NAME"

#ifdef __cplusplus
extern "C" {
#endif

struct ble_hs_cfg;

void start_ble();
void stop_ble();

#ifdef __cplusplus
}
#endif

#endif