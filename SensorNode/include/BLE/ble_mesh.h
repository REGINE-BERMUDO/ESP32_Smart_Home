#ifndef BLE_MESH_H
#define BLE_MESH_H

// esp_err_t macros
#include <esp_err.h>

// Bluetooth Macros
#include <esp_bt.h>

// Bluedroid Host Stack
#include <esp_bt_main.h>

// ESP-IDF Logging functions
#include <esp_log.h>

// For Device metadata
#include <esp_bt_device.h>

// BLE Mesh definitions
#include <esp_ble_mesh_defs.h>

// Sets up BLE Bluedroid Host & Controller Stack
esp_err_t setupBLEHC(void);

// Get device UUID for Mesh Provisioning
void getDeviceUUID(uint8_t *device_uuid);

#endif