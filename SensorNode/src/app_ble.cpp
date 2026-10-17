#include "app_ble.h"

static const char *TAG = "SensorNode";






















static uint8_t dev_uuid[16] = {0xdd, 0xdd};

// Configuration Server Model Setup
static esp_ble_mesh_cfg_srv_t config_server = {
    .relay = ESP_BLE_MESH_RELAY_DISABLED,
    .beacon = ESP_BLE_MESH_BEACON_ENABLED,
    .friend_state = ESP_BLE_MESH_FRIEND_NOT_SUPPORTED,
    .gatt_proxy = ESP_BLE_MESH_GATT_PROXY_ENABLED,
    .default_ttl = 7,
    .net_transmit = ESP_BLE_MESH_TRANSMIT(2, 20),
    .relay_retransmit = ESP_BLE_MESH_TRANSMIT(2, 20)
};









// Define Model Publication context
ESP_BLE_MESH_MODEL_PUB_DEFINE(onoff_pub_0, 2 + 3, ROLE_NODE);

// User Data of Generic OnOff Server Model
static esp_ble_mesh_gen_onoff_srv_t onoff_server_0 = {
    .rsp_ctrl.get_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
    .rsp_ctrl.set_auto_rsp = ESP_BLE_MESH_SERVER_RSP_BY_APP,
    .rsp_ctrl.status_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP
};






// Mesh Model Instance
static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
    ESP_BLE_MESH_MODEL_GEN_ONOFF_SRV(&onoff_pub_0, &onoff_server_0),
    ESP_BLE_MESH_MODEL_HEALTH_SRV(?)
};

// BLE Mesh Element Description
static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, root_models, ESP_BLE_MESH_MODEL_NONE)
};

static esp_ble_mesh_prov_t provision = {
    .uuid = dev_uuid,
    .output_size = 0,
    .output_actions = 0
};

static esp_ble_mesh_comp_t composition = {
    .cid = CID_ESP,
    .elements = elements,
    .element_count = ARRAY_SIZE(elements)
};










void initializeBLEMesh(attention_callbacks_t cbs) {
    // Set Initial attention LED State
    attention_Handler(cbs);

    // Initialize Non-Volatile Flash with the Default NVS Flash Partition
    esp_err_t nvs_result = nvs_flash_init();
    if(nvs_result == ESP_ERR_NVS_NO_FREE_PAGES || nvs_result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // When If-Statement detects nvs is failing, it erases flash and initializes again
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // Sets up BLE Bluedroid Host & Controller Stack
    if(setupBLEHC() != ESP_OK) {
        return;
    }

    // Get device UUID for Mesh Provisioning
    getDeviceUUID(dev_uuid);

    // Store BLE Mesh Callbacks
    esp_ble_mesh_register_prov_callback(provisioningHandler);
    esp_ble_mesh_register_config_server_callback(ble_configHandler);
    esp_ble_mesh_register_generic_server_callback(?);
    esp_ble_mesh_register_health_server_callback(health_EventHandler);

    // Initialize BLE Mesh Provisioning Capabilities Module
    if(esp_ble_mesh_init(&provision, &composition) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to Initialize Mesh Module");
        return;
    }

    // Enable Provisioning Bearers (PB_ADV & PB_GATT)
    if(esp_ble_mesh_node_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to Enable node Provisioning Bearers");
        return;
    }
}