#pragma once

#include "stdint.h"
#include "host/ble_hs.h"
#include "main.h"

#ifdef __cplusplus
extern "C" {
#endif

/// TODO: MAKE DEVICE NAME CONFIG BASED
#define DEVICE_NAME     "ESP32"

struct ble_hs_cfg;

void start_ble();
void stop_ble();

#ifdef __cplusplus
}
#endif