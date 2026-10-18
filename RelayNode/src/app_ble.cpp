#include "app_ble.h"
#include "ble_mesh.h"
#include "relay.h"
#include "lab_mesh_addresses.h"
#include "esp_timer.h"
#include "state_reporter.h"

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

// Declare one Health test identifier, as in Espressif's example.
static uint8_t healthTestIDs[] = {0x00};

// Provide the test list required by the Health Server.
static esp_ble_mesh_health_srv_t healthServer = {
    .health_test = {
        .id_count = 1,
        .test_ids = healthTestIDs,
    },
};

// Reserve the client's publication buffer.
ESP_BLE_MESH_MODEL_PUB_DEFINE(onoffClientPub, 2 + 1, ROLE_NODE);

// Store the Generic OnOff Client's settings.
static esp_ble_mesh_client_t onoffClient = {};

// List the Mesh models provided by this node.
static esp_ble_mesh_model_t rootModels[] = {
    ESP_BLE_MESH_MODEL_CFG_SRV(&configServer),
    ESP_BLE_MESH_MODEL_GEN_ONOFF_SRV(&onoffPub, &onoffServer),
    ESP_BLE_MESH_MODEL_GEN_ONOFF_CLI(&onoffClientPub, &onoffClient),
    ESP_BLE_MESH_MODEL_HEALTH_SRV(&healthServer, &healthPub),
};

// Group our models into one addressable Mesh element.
static esp_ble_mesh_elem_t elements[] = {
    ESP_BLE_MESH_ELEMENT(
        0, rootModels, ESP_BLE_MESH_MODEL_NONE
    ),
};

// Describe the node's elements for Mesh initialization.
static esp_ble_mesh_comp_t composition = {
    .cid = 0x02E5,
    .element_count = sizeof(elements) / sizeof(elements[0]),
    .elements = elements,
};

// Keep provisioning settings available while Mesh is running.
// We will assign the device UUID during setup.
static esp_ble_mesh_prov_t provisioning = {};

static void reportRelayState(void)
{
    esp_ble_mesh_model_t *model = &rootModels[1];
    if (!esp_ble_mesh_node_is_provisioned() || model->keys[0] == ESP_BLE_MESH_KEY_UNUSED) return;

    esp_ble_mesh_msg_ctx_t destination = {};
    destination.net_idx = 0;
    destination.app_idx = model->keys[0];
    destination.addr = LAB_MESH_GATEWAY_ADDRESS;
    destination.send_ttl = 7;
    uint8_t state = getRelay() ? 1 : 0;
    const esp_err_t result = esp_ble_mesh_server_model_send_msg(model, &destination,
        ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS, sizeof(state), &state);
    if (result != ESP_OK) ESP_LOGW(TAG, "Relay report send failed: %s", esp_err_to_name(result));
}

// Apply incoming Mesh OnOff changes to relay channel 1.
static void relayServerHandler(
    esp_ble_mesh_generic_server_cb_event_t event,
    esp_ble_mesh_generic_server_cb_param_t *param)
{
    if (param == nullptr) return;
    if (event == ESP_BLE_MESH_GENERIC_SERVER_STATE_CHANGE_EVT)
    {
        // Handle both acknowledged and unacknowledged ON/OFF commands.
        if (param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET ||
            param->ctx.recv_op == ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK)
        {
            bool on = param->value.state_change.onoff_set.onoff != 0;

            setRelay(on);
            reportRelayState();
            requestStateReportRepeats();
            ESP_LOGI(TAG, "Mesh commanded relay: %s", on ? "ON" : "OFF");
        }
    }
}

// Receive OnOff Status messages published by the light sensor.
static void relayClientHandler(
    esp_ble_mesh_generic_client_cb_event_t event,
    esp_ble_mesh_generic_client_cb_param_t *param)
{
    // Only handle successfully received publications.
    if (param == nullptr || event != ESP_BLE_MESH_GENERIC_CLIENT_PUBLISH_EVT ||
        param->error_code != 0 || param->params == nullptr ||
        param->params->ctx.addr != LAB_MESH_SENSOR_ADDRESS)
    {
        return;
    }

    // Ignore messages that are not OnOff Status.
    if (param->params->opcode != ESP_BLE_MESH_MODEL_OP_GEN_ONOFF_STATUS)
    {
        return;
    }

    // Lab mapping: dark = ON, bright = OFF.
    bool on = param->status_cb.onoff_status.present_onoff != 0;

    // Publication retransmissions must not replay an old reading over a web command.
    static bool previousSensorState = false;
    static int64_t previousSensorAt = 0;
    const int64_t now = esp_timer_get_time();
    if (previousSensorAt != 0 && previousSensorState == on &&
        now - previousSensorAt < 1000000) return;
    previousSensorState = on;
    previousSensorAt = now;

    setRelay(on);

    // Keep the server's reported state consistent with the relay command.
    onoffServer.state.onoff = on;
    reportRelayState();
    requestStateReportRepeats();

    ESP_LOGI(TAG, "Published state commanded relay: %s", on ? "ON" : "OFF");
}

// Connect event handlers and initialize the Mesh stack.
esp_err_t setupRelayMesh(uint8_t *deviceUUID)
{
    // Reject a missing UUID buffer.
    if (deviceUUID == nullptr)
    {
        return ESP_ERR_INVALID_ARG;
    }

    // Use the UUID prepared in main.cpp.
    provisioning.uuid = deviceUUID;

    // Register the shared event handlers.
    ESP_ERROR_CHECK(esp_ble_mesh_register_prov_callback(
        provisioningHandler));
    ESP_ERROR_CHECK(esp_ble_mesh_register_config_server_callback(
        ble_configHandler));
    ESP_ERROR_CHECK(esp_ble_mesh_register_health_server_callback(
        health_EventHandler));

    // Send Generic Server events to our relay-control handler.
    ESP_ERROR_CHECK(esp_ble_mesh_register_generic_server_callback(
        relayServerHandler));

    // Send incoming sensor publications to our client handler.
    ESP_ERROR_CHECK(esp_ble_mesh_register_generic_client_callback(
        relayClientHandler));

    // Initialize Mesh before allowing provisioning.
    esp_err_t result = esp_ble_mesh_init(&provisioning, &composition);
    if (result != ESP_OK)
    {
        return result;
    }

    result = setupStateReporter(reportRelayState);
    if (result != ESP_OK) return result;

    // Keep restored membership without enabling provisioning again.
    if (esp_ble_mesh_node_is_provisioned())
    {
        ESP_LOGI(TAG, "Restored existing Mesh membership");
        return ESP_OK;
    }
    
    // Allow both provisioning methods using the required enum type.
    return esp_ble_mesh_node_prov_enable(
        static_cast<esp_ble_mesh_prov_bearer_t>(
            ESP_BLE_MESH_PROV_ADV | ESP_BLE_MESH_PROV_GATT));
}
