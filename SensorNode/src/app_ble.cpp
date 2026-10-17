#include "app_ble.h"
























static uint8_t dev_uuid[16] = {0xdd, 0xdd};

// Mesh Model Instance
static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(?),
    ESP_BLE_MESH_MODEL__GEN_ONOF_SRV(?),
    ESP_BLE_MESH_MODEL_HEALTH_SRV(?)
};

// BLE Mesh Element Description
static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(0, ?, ESP_BLE_MESH_MODEL_NONE)
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
    getDeviceUUID(?);

    // Store BLE Mesh Callbacks
    esp_ble_mesh_register_prov_callback(?);
    esp_ble_mesh_register_config_server_callback(?);
    esp_ble_mesh_register_generic_server_callback(?);
    esp_ble_mesh_register_health_server_callback(?);

    // Initialize BLE Mesh Provisioning Capabilities Module
    if(esp_ble_mesh_init(&provision, &composition) != ESP_OK) {
        ?
        return;
    }

    // Enable Provisioning Bearers (PB_ADV & PB_GATT)
    if(esp_ble_mesh_node_prov_enable(ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT) != ESP_OK) {
        ?
        return;
    }
}