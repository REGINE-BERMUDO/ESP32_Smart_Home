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

// Sets up BLE Bluedroid Host & Controller Stack
esp_err_t setupBLEHC(void);

#endif