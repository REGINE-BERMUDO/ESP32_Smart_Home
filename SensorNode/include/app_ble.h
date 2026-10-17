#ifndef APP_BLE_H
#define APP_BLE_H

// Bluetooth SIG Company Identifier for Espressif Systems
#define CID_ESP 0x02E5

#include "Common/BLE/ble_mesh.h"
#include <nvs_flash.h>

// Initialize BLE Mesh
void initializeBLEMesh(attention_callbacks_t cbs);

#endif // APP_BLE_H