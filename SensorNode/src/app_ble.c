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
    .rsp_ctrl.set_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP,
    .rsp_ctrl.status_auto_rsp = ESP_BLE_MESH_SERVER_AUTO_RSP
};

static uint8_t test_ids[] = {0, 1, 2};

ESP_BLE_MESH_MODEL_PUB_DEFINE(health_pub_0, 10, ROLE_NODE);
static esp_ble_mesh_health_srv_t health_server = {
    .health_test = {
        .id_count = ARRAY_SIZE(test_ids),
        .test_ids = test_ids,
        .company_id = CID_ESP
    }
};










// Mesh Model Instance
static esp_ble_mesh_model_t root_models[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&config_server),
    ESP_BLE_MESH_MODEL_GEN_ONOFF_SRV(&onoff_pub_0, &onoff_server_0),
    ESP_BLE_MESH_MODEL_HEALTH_SRV(&health_server, &health_pub_0)
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


esp_err_t appble_update_state(bool onoff)
{
    esp_ble_mesh_model_t *model = &root_models[1];
    esp_ble_mesh_gen_onoff_srv_t *srv = &onoff_server_0;

    srv->state.onoff = onoff;

    // Retry later until Mesh publication is configured.
    if (!esp_ble_mesh_node_is_provisioned() ||
        model->pub == NULL ||
        model->pub->publish_addr == ESP_BLE_MESH_ADDR_UNASSIGNED)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t result = esp_ble_mesh_model_publish(
        model,
        ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS,
        sizeof(srv->state.onoff),
        &srv->state.onoff,
        ROLE_NODE);

    if (result != ESP_OK)
    {
        ESP_LOGW(TAG, "Sensor publication failed: %s",
                 esp_err_to_name(result));
    }

    return result;
}






void initializeBLEMesh(attention_callbacks_t cbs) {
    // Set Initial attention LED State
    attention_Handler(cbs);

   esp_err_t nvs_result = nvs_flash_init();
    if (nvs_result == ESP_ERR_NVS_NO_FREE_PAGES ||
     nvs_result == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
    ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_result);

    ESP_ERROR_CHECK(setupBLEHC());

    // Get device UUID for Mesh Provisioning
    getDeviceUUID(dev_uuid);

   // Stop startup if a Mesh event handler cannot be registered.
    ESP_ERROR_CHECK(esp_ble_mesh_register_prov_callback(
        provisioningHandler));
    ESP_ERROR_CHECK(esp_ble_mesh_register_config_server_callback(
    ble_configHandler));
    ESP_ERROR_CHECK(esp_ble_mesh_register_health_server_callback(
        health_EventHandler));

    ESP_ERROR_CHECK(esp_ble_mesh_init(&provision, &composition));

    // Use saved membership instead of enabling provisioning again.
    if (esp_ble_mesh_node_is_provisioned())
    {
        ESP_LOGI(TAG, "Restored SensorNode Mesh membership");
        return;
    }

    ESP_ERROR_CHECK(esp_ble_mesh_node_prov_enable(
    ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT));
}