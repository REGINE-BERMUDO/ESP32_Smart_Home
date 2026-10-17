#include "app_ble.h"
#include "ble_mesh.h"
#include "relay.h"

#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_generic_model_api.h"

// Label messages printed by this module.
static const char *TAG = "relay_mesh";

// Starting settings for the Mesh Configuration Server.
static esp_ble_mesh_cfg_srv_t configServer = {
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay = ESP_BLE_MESH_RELAY_DISABLED,
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_ENABLED,
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
    .default_ttl = 7,
};


 // Reserve space for an OnOff Status message:
    // 2 bytes for the message type, up to 3 bytes for its data.
    ESP_BLE_MESH_MODEL_PUB_DEFINE(onoffPub, 2 + 3, ROLE_NODE);

    // Let ESP-IDF automatically answer OnOff Get and Set messages.
    static esp_ble_mesh_gen_onoff_srv_t onoffServer = {
        .rsp_ctrl = {
            .get_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
            .set_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
    },
};

// Reserve a Health Status message buffer with no fault entries.
ESP_BLE_MESH_HEALTH_PUB_DEFINE(healthPub, 0, ROLE_NODE);

// Start with zero-initialized Health Server settings.
static esp_ble_mesh_health_srv_t healthServer = {};

// List the Mesh models provided by this node.
static esp_ble_mesh_model_t rootModels[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&configServer),
    ESP_BLE_MESH_MODEL_GEN_ONOFF_SRV(&onoffPub, &onoffServer),

    ESP_BLE_MESH_MODEL_HEALTH_SRV(&healthServer, &healthPub),
};

