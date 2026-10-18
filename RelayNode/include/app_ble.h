#ifndef APP_BLE_H
#define APP_BLE_H

#include <stdint.h>
#include "esp_err.h"

// Initialize the relay node's Mesh models and provisioning.
esp_err_t setupRelayMesh(uint8_t *deviceUUID);

#endif