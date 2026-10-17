#ifndef APP_BLE_H
#define APP_BLE_H

// Bluetooth SIG Company Identifier for Espressif Systems
#define CID_ESP 0x02E5

#include "ble_mesh.h"
#include <nvs_flash.h>

// Initialize BLE Mesh
void initializeBLEMesh(attention_callbacks_t cbs);

// Return ESP_OK when the sensor publication is accepted for sending.
esp_err_t appble_update_state(bool onoff);

#endif // APP_BLE_H