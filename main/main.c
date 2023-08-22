#include "nvs_flash.h"
#include "freertos/FreeRTOSConfig.h"
#include "nimble/nimble_port_freertos.h"
#include "main.h"
#include "ble_prov.h"
#include "wifi.h"
#include "my_nvs.h"
#include "mqtt.h"


void app_main(void)
{
    uint8_t ssid[WIFI_SSID_MAX_SIZE];
    uint8_t pwd[WIFI_PWD_MAX_SIZE];

    /* Initialize NVS — it is used to store PHY calibration data */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);


    ret = nvs_get_wifi_data(ssid, pwd);
    if(ret == ESP_FAIL) {
        printf("Error while getting wifi data from nvs!\n");
        return;
    } else if(ret == ESP_ERR_NVS_NOT_FOUND) {
        // Wifi has not been provisioned yet, do so by ble
        printf("Wifi has NOT been provisioned...\n");

        /* Start Ble */
        printf("Starting ble.\n");
        start_ble();


        return;
    }

    printf("Wifi has been provisioned.\n");

    ret = wifi_init_sta(ssid, pwd);
    if(ret != ESP_OK) {
        printf("Error while getting connecting to wifi!\n");

        /// TODO: CLEAR WIFI DATA FROM NVS?

        return;
    }

    printf("Wifi successfully connected.\n");

    // Check if thing has been registered.
    ret = mqtt_get_tls_certificates(NVS_KEY_SERVER_CERT, NVS_KEY_CON_CLIENT_CERT, NVS_KEY_CON_CLIENT_KEY);
    if(ret == ESP_ERR_NVS_NOT_FOUND) {
        // Thing has not been registered yet, do so.
        printf("Register thing.\n");
        mqtt_register_thing();
    } else if(ret == ESP_OK) {
        // Thing has been registered, start sending temperature data to aws.
        printf("Start sending temperature data.\n");
        mqtt_start_sending_data();
    } else {
        // An unexpected error occurred
        printf("An error occurred while getting tls certificates.\n");
        return;
    }

    /// TODO: CHANGE QOS 0 TO QOS 2 FOR FLEET PROVISINING?
}