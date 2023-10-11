#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_system.h"
#include "wifi.h"
#include "ble_prov.h"
#include "my_nvs.h"

#include "ble_prov_gatt.h"

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// Keeps track of retries
static int s_retry_num = 0;

// Function declarations
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < WIFI_MAX_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            // Failed to connect to wifi
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        // Successfully connected to wifi
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_init_sta(uint8_t *ssid, uint8_t *pwd)
{
    s_wifi_event_group = xEventGroupCreate();

    // Initialize the underlying TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create default event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };

    // Set wifi config ssid and password
    strncpy((char *)wifi_config.sta.ssid, (char *)ssid, sizeof wifi_config.sta.ssid);
    strncpy((char *)wifi_config.sta.password, (char *)pwd, sizeof wifi_config.sta.password);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap");
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect");
        return ESP_FAIL;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        return ESP_FAIL;
    }
}

void wifi_test_prov_data(struct prov_data *pdata)
{
    esp_err_t ret = wifi_init_sta(pdata->ssid, pdata->pwd);
    if(ret == ESP_OK) {
        // SUCCESS; Save pdata to nvs and reboot
        ESP_LOGI(TAG, "Connection to wifi succeeded.");

        // SAVE PROV DATA TO NVS
        ESP_LOGI(TAG, "Saving of prov data...");
        ret = nvs_set_prov_data(pdata);
        if(ret != ESP_OK) {
            printf("Error (%s) while setting wifi data!\n", esp_err_to_name(ret));
            return;
        }
        ESP_LOGI(TAG, "Saving of prov data succeeded.");
        ESP_LOGI(TAG, "Restarting...");

        esp_restart();
    } else {
        // FAILED; Reboot
        ESP_LOGI(TAG, "Connection to wifi failed.");
        ESP_LOGI(TAG, "Restarting...");
        esp_restart();
    }
}

void wifi_task(void* arg)
{
    ESP_LOGI(TAG, "Wifi task created!");
    // Get pdata from passes parameter
    struct prov_data *pdata = (struct prov_data *)arg;

    // wait for 1 second
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    wifi_test_prov_data(pdata);
    // Delete this task after, not really needed as above function will 
    // always (almost) end in reboot
    ESP_LOGI(TAG, "Error occurred while testing wifi data.");
    vTaskDelete(NULL);
}