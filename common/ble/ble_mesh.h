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

// BLE Mesh Networking API
#include <esp_ble_mesh_networking_api.h>

// BLE Mesh Configuration Model API
#include <esp_ble_mesh_config_model_api.h>

// BLE Mesh Health Model API
#include <esp_ble_mesh_health_model_api.h>

// Sets up BLE Bluedroid Host & Controller Stack
esp_err_t setupBLEHC(void);

// Get device UUID for Mesh Provisioning
void getDeviceUUID(uint8_t *device_uuid);

// Check Device Provisioning State
void provisioningHandler(esp_ble_mesh_prov_cb_event_t event, esp_ble_mesh_prov_cb_param_t *param);

// Allows a device to be configured for the Mesh network
void ble_configHandler(esp_ble_mesh_cfg_server_cb_event_t event, esp_ble_mesh_cfg_server_cb_param_t *param);

// Handles Incoming Health Updates/Events
void health_EventHandler(esp_ble_mesh_health_server_cb_event_t event, esp_ble_mesh_health_server_cb_param_t *param);

// Function Pointers and Aliased for simplicity
typedef void (*attention_on_Function)(void);
typedef void (*attention_off_Function)(void);

// attention_callbacks_t blueprint that holds function addresses
typedef struct {
    attention_on_Function attention_on;
    attention_off_Function attention_off;
} attention_callbacks_t;

// Called from Nodes BLE Initialization
void attention_Handler(attention_callbacks_t callbacks);

#endif