#include "BLE/ble_mesh.h"

static const char *TAG = "BLE Mesh";

esp_err_t setupBLEHC(void) {
    esp_err_t result;
    // Free up Classic Bluetooth memory
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    // Configure Bluetooth Controller (Default Config mode)
    esp_bt_controller_config_t bluetooth_config = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    // Initializes Hardware Bluetooth Controller
    result = esp_bt_controller_init(&bluetooth_config);
    if(result != ESP_OK) {
        ESP_LOGI(TAG, "%s: Controller Initialization Failed!", __func__);
        return result;
    }

    // Powers on the Physical Radio in BLE Mode 
    result = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if(result != ESP_OK) {
        ESP_LOGI(TAG, "%s: Powering Hardware Controller Failed!", __func__);
        return result;
    }

    // Initialize Bluedroid Host Stack Configuration
    result = esp_bluedroid_init();
    if(result != ESP_OK) {
        ESP_LOGI(TAG, "%s: Bluedroid Initialization Failed!", __func__);
        return result;
    }

    // Activate Bluedroid Host Stack
    result = esp_bluedroid_enable();
    if(result != ESP_OK) {
        ESP_LOGI(TAG, "%s: Bluedroid Activation Failed!", __func__);
        return result;
    }

    return result;
}