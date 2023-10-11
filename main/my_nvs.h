#pragma once

#include <stdint.h>
#include "esp_err.h"
#include "nvs_flash.h"
#include "main.h"
#include "ble_prov_gatt.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NVS_NAMESPACE               "storage"
#define NVS_KEY_WIFI_SSID           "my_wifi_ssid"
#define NVS_KEY_WIFI_PWD            "my_wifi_pwd"
#define NVS_KEY_AWS_UUID            "aws_uuid"
#define NVS_KEY_AWS_THING_NAME      "aws_thing"

// AmazonCA1.pem
#define NVS_KEY_SERVER_CERT         "server_cert"

// Provisioning by claim client certificate and key, these are loaded to NVS before app's execution
#define NVS_KEY_CLAIM_CLIENT_CERT    "client_cert"
#define NVS_KEY_CLAIM_CLIENT_KEY     "client_key"

// Certificates that will be used after a successful RegisterThing MQTT API call
#define NVS_KEY_CON_CLIENT_CERT     "con_client_cert"
#define NVS_KEY_CON_CLIENT_KEY      "con_client_key"

/**
 * Gets wifi ssid and pwd that are stored in nvs 
 * @return  ESP_OK on success,
 *          ESP_ERR_NVS_NOT_FOUND when no value for either key (ssid and pwd not set)
 *          ESP_fail on failure
*/
esp_err_t nvs_get_wifi_data(uint8_t *ssid_output, uint8_t *pwd_output);


/// @brief Erases wifi data from nvs storage
/// @return ESP_OK on success, ESP_FAIL on failure
esp_err_t nvs_erase_wifi_data();

/**
 * Saves provisioning data to nvs 
 * @return  ESP_OK on success,
 *          ESP_fail on failure
*/
esp_err_t nvs_set_prov_data(struct prov_data *pdata);

/**
 * Gets tls certificates from stored in nvs
 * @param server_cert buffer capable holding a pem certificate
 * @param server_cert_nvs_key nvs key of @param server_cert
 * 
 * @param client_cert buffer capable holding a pem certificate
 * @param client_cert_nvs_key nvs key of @param client_cert
 * 
 * @param client_key buffer capable holding a pem certificate
 * @param client_key_nvs_key nvs key of @param client_key
 * 
 * @return  ESP_OK on successful retrieval of certificates
 *          ESP_ERR_NVS_NOT_FOUND when any nvs key has no value
 *          ESP_FAIL on failure
*/
esp_err_t nvs_get_tls_certs(
    char *server_cert, const char* server_cert_nvs_key,
    char *client_cert, const char* client_cert_nvs_key,
    char *client_key, const char* client_key_nvs_key);

/**
 * Sets tls cetificates to nvs storage
 * @param client_cert buffer containing the pem certificate
 * @param client_cert_nvs_key nvs key of @param client_cert
 * 
 * @param client_key buffer containing the pem certificate
 * @param client_key_nvs_key nvs key of @param client_key
 * 
 * @return  ESP_OK on success
 *          ESP_FAIL on failure
*/
esp_err_t nvs_set_tls_certs(
    const char *client_cert, const char* client_cert_nvs_key,
    const char *client_key, const char* client_key_nvs_key);

/**
 *  Get thingname from NVS storage
 * @return  ESP_OK on success,
 *          ESP_ERR_NVS_NOT_FOUND when no value for key
 *          ESP_fail on failure
*/
esp_err_t nvs_get_thing_name(char *thing_name);

/// Wrapper around nvs_open
esp_err_t nvs_open_and_print(nvs_handle_t *out_handle, const char *nvs_namespace, nvs_open_mode_t open_mode);

/// Wrapper around nvs_set_str
esp_err_t nvs_set_str_and_print(nvs_handle_t handle, const char *key, const char *in_value);

/// Wrapper around nvs_get_str
esp_err_t nvs_get_str_and_print(nvs_handle_t handle, const char *key, char *out_value, size_t *max_length);

/// Wrapper around nvs_set_blob
esp_err_t nvs_set_blob_and_print(nvs_handle_t handle, const char *key, const void *in_value, size_t length);

/// Wrapper around nvs_get_blob
esp_err_t nvs_get_blob_and_print(nvs_handle_t handle, const char *key, char *out_value, size_t *max_length);

#ifdef __cplusplus
}
#endif