#include "BLE/ble_mesh.h"

void setupBLEHC(void) {
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
}