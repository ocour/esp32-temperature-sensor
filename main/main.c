#include "main.h"
#include "ble_prov.h"

#include "nvs_flash.h"
#include "freertos/FreeRTOSConfig.h"
#include "nimble/nimble_port_freertos.h"

// DEBUG MEM LEAK
#include <esp_heap_trace.h>
#define NUM_RECORDS 100
static heap_trace_record_t trace_record[NUM_RECORDS];
//

void app_main(void)
{
    /* Initialize NVS — it is used to store PHY calibration data */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // // DEBUG MEM LEAK
    // ESP_ERROR_CHECK(heap_trace_init_standalone(trace_record, NUM_RECORDS));
    // ESP_ERROR_CHECK(heap_trace_start(HEAP_TRACE_LEAKS));
    // //

    /* Start Ble */
    start_ble();

    // vTaskDelay(pdMS_TO_TICKS(500));

    // /* Stop Ble */
    // stop_ble();

    // // DEBUG MEM LEAK
    // ESP_ERROR_CHECK(heap_trace_stop());
    // heap_trace_dump();
    // //
}