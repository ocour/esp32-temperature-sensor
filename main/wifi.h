#pragma once

#include "esp_err.h"
#include "sdkconfig.h"
#include "main.h"
#include "ble_prov_gatt.h"

#ifdef __cplusplus
extern "C" {
#endif

// Maximum amount of tries wifi will try to connect to AP
#define WIFI_MAX_RETRY  CONFIG_WIFI_MAXIMUM_RETRY

/* Strength of authmodes */
/* OPEN < WEP < WPA_PSK < OWE < WPA2_PSK = WPA_WPA2_PSK < WAPI_PSK < WPA2_ENTERPRISE < WPA3_PSK = WPA2_WPA3_PSK */
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

/**
 * Connect to wifi
 * @arg pdata: provisioning data
 * @return ESP_OK for success, ESP_FAIL for failure
*/
esp_err_t wifi_init_sta(uint8_t *ssid, uint8_t *pwd);

/**
 * Check wifi provisioning data
 * @arg pdata: Provisioning data
*/
void wifi_test_prov_data(struct prov_data *pdata);

void wifi_task(void* arg);

#ifdef __cplusplus
}
#endif