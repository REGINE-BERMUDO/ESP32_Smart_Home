#include "esp_log.h"
#include "nvs_flash.h"
#include "app_wifi.h"
#include "ble_mesh.h"

extern "C" void app_main(void)
{
    // Initialize flash storage required by the Wi-Fi driver.
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_LOGI("gateway", "Gateway node started");

    // Start the Bluetooth controller and Bluedroid host.
    ESP_ERROR_CHECK(setupBLEHC());
    ESP_LOGI("gateway", "Bluetooth started");
    
    // Start Wi-Fi; connection results arrive through our event handler.
    setupWiFi();
}