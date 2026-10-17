#include "relay.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern "C" void app_main(void)
{
    // Configure the relay output with its initial OFF level.
    setupRelay();

    // Print a startup message in the Serial Monitor.
    ESP_LOGI("relay", "Relay output initialized to OFF");

    // Wait two seconds before testing.
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Turn channel 1 ON for two seconds.
    setRelay(true);
    ESP_LOGI("relay", "Channel 1 commanded ON");
    vTaskDelay(pdMS_TO_TICKS(2000));

    // Turn channel 1 OFF and leave it OFF.
    setRelay(false);
    ESP_LOGI("relay", "Channel 1 commanded OFF");
}