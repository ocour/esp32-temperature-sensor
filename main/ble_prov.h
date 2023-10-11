#pragma once

#include "stdint.h"
#include "host/ble_hs.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PROV_SENSOR_SERVICE BLE_UUID128_INIT(0xa3, 0xc6, 0x8a, 0xf5, 0x05, 0x5c, 0x37, 0xa6, \
                     0xaf, 0x44, 0x9e, 0xe5, 0x63, 0x1f, 0xb2, 0xc4)

struct ble_hs_cfg;

void start_ble();
void stop_ble();

#ifdef __cplusplus
}
#endif