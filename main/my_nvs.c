#include <string.h>
#include "nvs_flash.h"
#include "my_nvs.h"
#include "mqtt.h"
#include "ble_prov_gatt.h"

esp_err_t nvs_get_wifi_data(uint8_t *ssid_output, uint8_t *pwd_output)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    size_t output_len;

    err = nvs_open_and_print(&nvs_handle, NVS_NAMESPACE, NVS_READWRITE);
    if(err != ESP_OK) {
        return err;
    }

    output_len = WIFI_SSID_MAX_SIZE;
    err = nvs_get_str_and_print(nvs_handle, NVS_KEY_WIFI_SSID, (char *)ssid_output, &output_len);
    if(err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    output_len = WIFI_PWD_MAX_SIZE;
    err = nvs_get_str_and_print(nvs_handle, NVS_KEY_WIFI_PWD, (char *)pwd_output, &output_len);
    if(err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    // CLOSE NVS HANDLE
    nvs_close(nvs_handle);
    return ESP_OK;
}

esp_err_t nvs_erase_wifi_data()
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open_and_print(&nvs_handle, NVS_NAMESPACE, NVS_READWRITE);
    if(err != ESP_OK) {
        return err;
    }

    err = nvs_erase_key(nvs_handle, NVS_KEY_WIFI_SSID);
    if(err != ESP_OK) {
        printf("Error (%s) erasing %s key!\n", esp_err_to_name(err), NVS_KEY_WIFI_SSID);
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_erase_key(nvs_handle, NVS_KEY_WIFI_PWD);
    if(err != ESP_OK) {
        printf("Error (%s) erasing %s key!\n", esp_err_to_name(err), NVS_KEY_WIFI_PWD);
        nvs_close(nvs_handle);
        return err;
    }

    // CLOSE NVS HANDLE
    nvs_close(nvs_handle);
    return ESP_OK;
}

esp_err_t nvs_set_prov_data(struct prov_data *pdata)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;

    err = nvs_open_and_print(&nvs_handle, NVS_NAMESPACE, NVS_READWRITE);
    if(err != ESP_OK) {
        return err;
    }

    err = nvs_set_str_and_print(nvs_handle, NVS_KEY_WIFI_SSID, (char *)(pdata->ssid));
    if(err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_set_str_and_print(nvs_handle, NVS_KEY_WIFI_PWD, (char *)(pdata->pwd));
    if(err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_set_str_and_print(nvs_handle, NVS_KEY_AWS_UUID, (char *)(pdata->aws_uuid));
    if(err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_set_str_and_print(nvs_handle, NVS_KEY_AWS_THING_NAME, (char *)(pdata->aws_thing));
    if(err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    // CLOSE NVS HANDLE
    nvs_close(nvs_handle);
    return ESP_OK;
}

esp_err_t nvs_get_tls_certs(
    char *server_cert, const char* server_cert_nvs_key,
    char *client_cert, const char* client_cert_nvs_key,
    char *client_key, const char* client_key_nvs_key
)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    size_t output_len;

    err = nvs_open_and_print(&nvs_handle, NVS_NAMESPACE, NVS_READWRITE);
    if(err != ESP_OK) {
        return err;
    }

    output_len = SERVER_CERT_MAX_SIZE;
    err = nvs_get_blob_and_print(nvs_handle, server_cert_nvs_key, server_cert, &output_len);
    if(err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    } 

    output_len = CLIENT_CERT_MAX_SIZE;
    err = nvs_get_blob_and_print(nvs_handle, client_cert_nvs_key, client_cert, &output_len);
    if(err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    output_len = CLIENT_KEY_SIZE;
    err = nvs_get_blob_and_print(nvs_handle, client_key_nvs_key, client_key, &output_len);
    if(err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    // SUCCESS!
    // Close handle
    nvs_close(nvs_handle);
    return ESP_OK;
}

esp_err_t nvs_set_tls_certs(
    const char *client_cert, const char* client_cert_nvs_key,
    const char *client_key, const char* client_key_nvs_key
)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    size_t input_len;

    err = nvs_open_and_print(&nvs_handle, NVS_NAMESPACE, NVS_READWRITE);
    if(err != ESP_OK) {
        return err;
    }

    input_len = strlen((char*)client_cert);
    err = nvs_set_blob_and_print(nvs_handle, client_cert_nvs_key, client_cert, input_len);
    if(err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    input_len = strlen((char*)client_key);
    err = nvs_set_blob_and_print(nvs_handle, client_key_nvs_key, client_key, input_len);
    if(err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    // SUCCESS!
    // Close handle
    nvs_close(nvs_handle);
    return ESP_OK;
}

esp_err_t nvs_get_thing_name(char *thing_name)
{
    nvs_handle_t nvs_handle;
    esp_err_t err;
    size_t output_len;

    err = nvs_open_and_print(&nvs_handle, NVS_NAMESPACE, NVS_READWRITE);
    if(err != ESP_OK) {
        return err;
    }

    output_len = AWS_THING_NAME_MAX_SIZE;
    err = nvs_get_str_and_print(nvs_handle, NVS_KEY_AWS_THING_NAME, thing_name, &output_len);
    if(err != ESP_OK) {
        nvs_close(nvs_handle);
        return err;
    }

    // CLOSE NVS HANDLE
    nvs_close(nvs_handle);
    return ESP_OK;
}

esp_err_t nvs_open_and_print(nvs_handle_t *out_handle, const char *nvs_namespace, nvs_open_mode_t open_mode)
{
    esp_err_t err;

    printf("Opening Non-Volatile Storage (NVS) handle %s... ", nvs_namespace);

    err = nvs_open(nvs_namespace, open_mode, out_handle);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle %s!\n", esp_err_to_name(err), nvs_namespace);
    } else {
        printf("Done\n");
    }

    return err;
}

esp_err_t nvs_set_str_and_print(nvs_handle_t handle, const char *key, const char *in_value)
{
    esp_err_t err;

    printf("Setting %s... ", key);

    err = nvs_set_str(handle, key, in_value);
    if(err != ESP_OK) {
        printf("Error (%s) setting %s!\n", esp_err_to_name(err), key);
    } else {
        printf("Done\n");
    }

    return err;
}

esp_err_t nvs_get_str_and_print(nvs_handle_t handle, const char *key, char *out_value, size_t *max_length)
{
    esp_err_t err;

    printf("Getting %s... ", key);

    err = nvs_get_str(handle, key, out_value, max_length);
    if (err != ESP_OK) {
        printf("Error (%s) getting %s!\n", esp_err_to_name(err), key);
    } else {
        printf("Done\n");
    }

    return err;
}

esp_err_t nvs_set_blob_and_print(nvs_handle_t handle, const char *key, const void *in_value, size_t length)
{
    esp_err_t err;

    printf("Setting %s... ", key);

    err = nvs_set_blob(handle, key, in_value, length);
    if(err != ESP_OK) {
        printf("Error (%s) setting %s!\n", esp_err_to_name(err), key);
    } else {
        printf("Done\n");
    }

    return err;
}

esp_err_t nvs_get_blob_and_print(nvs_handle_t handle, const char *key, char *out_value, size_t *max_length)
{
    esp_err_t err;

    printf("Getting %s... ", key);

    err = nvs_get_blob(handle, key, out_value, max_length);
    if (err != ESP_OK) {
        printf("Error (%s) getting %s!\n", esp_err_to_name(err), key);
    } else {
        printf("Done\n");
    }

    return err;
}