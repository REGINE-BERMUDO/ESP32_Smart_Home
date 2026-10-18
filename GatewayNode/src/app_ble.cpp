#include "app_ble.h"
#include "ble_mesh.h"
#include "lab_mesh_addresses.h"

#include "esp_ble_mesh_common_api.h"
#include "esp_ble_mesh_provisioning_api.h"
#include "esp_ble_mesh_generic_model_api.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Label messages from the gateway's Mesh module.
static const char *TAG = "gateway_mesh";

// Keep the device identifier available while Mesh is running.
static uint8_t deviceUUID[16] = {0};

// Reserve a publication buffer for the OnOff Client.
ESP_BLE_MESH_MODEL_PUB_DEFINE(onoffClientPub, 2 + 1, ROLE_NODE);

// Store the OnOff Client's settings.
static esp_ble_mesh_client_t onoffClient = {};

// RelayNode's current provisioned address.
static constexpr uint16_t relayAddress = LAB_MESH_RELAY_ADDRESS;

// Identify successive OnOff commands.
static uint8_t transactionID = 0;

static portMUX_TYPE relayStateLock = portMUX_INITIALIZER_UNLOCKED;
static bool reportedOn = false;
static int64_t reportedAt = 0;

bool getRelayReportedState(bool *on)
{
    if (on == nullptr) return false;
    const int64_t now = esp_timer_get_time();
    portENTER_CRITICAL(&relayStateLock);
    const bool fresh = reportedAt != 0 && now - reportedAt <= 6000000;
    *on = reportedOn;
    portEXIT_CRITICAL(&relayStateLock);
    return fresh;
}

static void relayStatusHandler(esp_ble_mesh_generic_client_cb_event_t event,
                               esp_ble_mesh_generic_client_cb_param_t *param)
{
    if (param == nullptr || param->params == nullptr || param->error_code != 0 ||
        param->params->ctx.addr != relayAddress) return;

    const bool queryReply = event == ESP_BLE_MESH_GENERIC_CLIENT_GET_STATE_EVT &&
        param->params->opcode == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET;
    const bool pushedReport = event == ESP_BLE_MESH_GENERIC_CLIENT_PUBLISH_EVT &&
        param->params->opcode == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS;
    if (!queryReply && !pushedReport) return;

    const bool on = param->status_cb.onoff_status.present_onoff != 0;
    const int64_t now = esp_timer_get_time();
    portENTER_CRITICAL(&relayStateLock);
    const bool changed = reportedAt == 0 || reportedOn != on;
    reportedOn = on;
    reportedAt = now;
    portEXIT_CRITICAL(&relayStateLock);
    // Distinguish immediate reports from recovery replies when measuring latency.
    if (changed || pushedReport)
    {
        ESP_LOGI(TAG, "Relay reported state: %s via %s (RSSI %d dBm)",
                 on ? "ON" : "OFF", pushedReport ? "push" : "query",
                 static_cast<int>(param->params->ctx.recv_rssi));
    }
}

static void pollRelayState(void *)
{
    while (true)
    {
        if (esp_ble_mesh_node_is_provisioned() && onoffClient.model != nullptr &&
            onoffClient.model->keys[0] != ESP_BLE_MESH_KEY_UNUSED)
        {
            esp_ble_mesh_client_common_param_t query = {};
            query.opcode = ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_GET;
            query.model = onoffClient.model;
            query.ctx.net_idx = 0;
            query.ctx.app_idx = onoffClient.model->keys[0];
            query.ctx.addr = relayAddress;
            query.ctx.send_ttl = 7;
            query.msg_timeout = 1500;
            esp_ble_mesh_generic_client_get_state_t state = {};
            const esp_err_t result = esp_ble_mesh_generic_client_get_state(&query, &state);
            if (result != ESP_OK) ESP_LOGD(TAG, "Relay query failed: %s", esp_err_to_name(result));
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

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

// Prepare the gateway identity and initialize its Mesh models.
esp_err_t setupGatewayMesh(void)
{
    // Bluetooth must already be started before reading its address.
    getDeviceUUID(deviceUUID);
    provisioning.uuid = deviceUUID;

    // Register the shared Mesh event handlers.
    ESP_ERROR_CHECK(esp_ble_mesh_register_prov_callback(
        provisioningHandler));
    ESP_ERROR_CHECK(esp_ble_mesh_register_config_server_callback(
        ble_configHandler));
    ESP_ERROR_CHECK(esp_ble_mesh_register_health_server_callback(
        health_EventHandler));
    ESP_ERROR_CHECK(esp_ble_mesh_register_generic_client_callback(relayStatusHandler));

        // Stop if Mesh initialization cannot be started.
    esp_err_t result = esp_ble_mesh_init(&provisioning, &composition);
    if (result != ESP_OK)
    {
        return result;
    }

    if (xTaskCreate(pollRelayState, "relay_status", 3072, nullptr, 5, nullptr) != pdPASS)
    {
        return ESP_ERR_NO_MEM;
    }

    // Keep saved membership after a restart.
    if (esp_ble_mesh_node_is_provisioned())
    {
        ESP_LOGI(TAG, "Restored gateway Mesh membership");
        return ESP_OK;
    }

    // Allow the phone to provision a new gateway.
    return esp_ble_mesh_node_prov_enable(
        static_cast<esp_ble_mesh_prov_bearer_t>(
            ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT));
}

// Send a command using the client's bound AppKey.
esp_err_t sendRelayCommand(bool on)
{
    if (!esp_ble_mesh_node_is_provisioned() ||
        onoffClient.model == nullptr)
    {
        return ESP_ERR_INVALID_STATE;
    }

    uint16_t appIndex = onoffClient.model->keys[0];
    if (appIndex == ESP_BLE_MESH_KEY_UNUSED)
    {
        return ESP_ERR_INVALID_STATE;
    }

    esp_ble_mesh_client_common_param_t command = {};
    command.opcode = ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK;
    command.model = onoffClient.model;
    // Lab setup uses the primary network, index 0.
    command.ctx.net_idx = 0;
    command.ctx.app_idx = appIndex;
    command.ctx.addr = relayAddress;
    command.ctx.send_ttl = 7;

    esp_ble_mesh_generic_client_set_state_t state = {};
    state.onoff_set.op_en = false;
    state.onoff_set.onoff = on ? 1 : 0;
    state.onoff_set.tid = transactionID++;

    return esp_ble_mesh_generic_client_set_state(&command, &state);
}
