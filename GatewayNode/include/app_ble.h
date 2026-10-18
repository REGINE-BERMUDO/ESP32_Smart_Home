#ifndef APP_BLE_H
#define APP_BLE_H

#include "esp_err.h"

// Initialize the gateway's Mesh models and provisioning.
esp_err_t setupGatewayMesh(void);

// Send an ON/OFF request to RelayNode.
// Success means the request was accepted for sending,
// not that the physical relay has confirmed switching.
esp_err_t sendRelayCommand(bool on);

// Return a fresh RelayNode report, or false when its state is unavailable.
bool getRelayReportedState(bool *on);

#endif
