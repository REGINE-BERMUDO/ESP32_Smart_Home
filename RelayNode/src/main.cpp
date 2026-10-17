#include "relay.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "ble_mesh.h"

extern "C" void app_main(void)
{
    // Initialize the relay output to OFF.
    setupRelay();

    // Initialize flash storage before starting Bluetooth.
    // Stop and report an error if initialization fails.
    ESP_ERROR_CHECK(nvs_flash_init());

    // Start the Bluetooth controller and Bluedroid software stack.
    ESP_ERROR_CHECK(setupBLEHC());

    // Report successful Bluetooth startup.
    ESP_LOGI("relay", "Bluetooth started");

    // Report startup without automatically switching the relay.
    ESP_LOGI("relay", "Relay output initialized to OFF");

    // Show the last commanded state: 0 = OFF, 1 = ON.
    ESP_LOGI("relay", "Commanded state: %d", getRelay());
}