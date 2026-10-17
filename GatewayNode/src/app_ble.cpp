#include "app_ble.h"
#include "ble_mesh.h"

#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_generic_model_api.h"

// Label messages from the gateway's Mesh module.
static const char *TAG = "gateway_mesh";

// Keep the device identifier available while Mesh is running.
static uint8_t deviceUUID[16] = {0};

// Reserve a publication buffer for the OnOff Client.
ESP_BLE_MESH_MODEL_PUB_DEFINE(onoffClientPub, 2 + 1, ROLE_NODE);

// Store the OnOff Client's settings.
static esp_ble_mesh_client_t onoffClient = {};

// Initial settings for configuring this Mesh node.
static esp_ble_mesh_cfg_srv_t configServer = {
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay = ESP_BLE_MESH_RELAY_DISABLED,
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_ENABLED,
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
    .default_ttl = 7,
};

// Reserve space for Health Status messages.
ESP_BLE_MESH_HEALTH_PUB_DEFINE(healthPub, 0, ROLE_NODE);

// Supply the test identifier required by the Health Server.
static uint8_t healthTestIDs[] = {0x00};

static esp_ble_mesh_health_srv_t healthServer = {
    .health_test = {
        .id_count = 1,
        .test_ids = healthTestIDs,
    },
};

// List the gateway's three Mesh models.
static esp_ble_mesh_model_t rootModels[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&configServer),
    ESP_BLE_MESH_MODEL_GEN_ONOFF_CLI(&onoffClientPub, &onoffClient),
    ESP_BLE_MESH_MODEL_HEALTH_SRV(&healthServer, &healthPub),
};

// Group the models into one addressable element.
static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, rootModels, ESP_BLE_MESH_MODEL_NONE),
};

// Describe the node's elements.
static esp_ble_mesh_comp_t composition = {
    .cid = 0x02E5,
    .element_count = sizeof(elements) / sizeof(elements[0]),
    .elements = elements,
};

// Hold the provisioning settings; assign the UUID during setup.
static esp_ble_mesh_prov_t provisioning = {};