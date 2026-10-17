#include "ble_mesh.h"

// Copy bytes when preparing the device UUID.
#include <string.h>

// FreeRTOS task creation and delays.
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ESP32 restart function.
#include "esp_system.h"

static const char *TAG = "BLE Mesh";

esp_err_t setupBLEHC(void) {
    esp_err_t result;

#if CONFIG_IDF_TARGET_ESP32
    // Release Classic Bluetooth memory only on the classic ESP32.
    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
#endif

    // Configure Bluetooth Controller (Default Config mode)
    esp_bt_controller_config_t bluetooth_config = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    // Initializes Hardware Bluetooth Controller
    result = esp_bt_controller_init(&bluetooth_config);
    if(result != ESP_OK) {
        ESP_LOGE(TAG, "%s: Controller Initialization Failed!", __func__);
        return result;
    }

    // Powers on the Physical Radio in BLE Mode 
    result = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if(result != ESP_OK) {
        ESP_LOGE(TAG, "%s: Powering Hardware Controller Failed!", __func__);
        return result;
    }

    // Initialize Bluedroid Host Stack Configuration
    result = esp_bluedroid_init();
    if(result != ESP_OK) {
        ESP_LOGE(TAG, "%s: Bluedroid Initialization Failed!", __func__);
        return result;
    }

    // Activate Bluedroid Host Stack
    result = esp_bluedroid_enable();
    if(result != ESP_OK) {
        ESP_LOGE(TAG, "%s: Bluedroid Activation Failed!", __func__);
        return result;
    }

    return result;
}

void getDeviceUUID(uint8_t *device_uuid) {
    // Checks whether the pointer holds an address or is empty
    if(device_uuid == NULL) {
        ESP_LOGE(TAG, "%s: Invalid Device UUID.", __func__);
        return;
    }

    // Copies the Bluetooth Controller 16-byte UUID to the pointer address
    memcpy(device_uuid + 2, esp_bt_dev_get_address(), BD_ADDR_LEN);
}










static void rebootDevice(void *pvParameters) {
    // Waits for 5 seconds before rebooting the device
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Reboots the device
    esp_restart();
}

void provisioningHandler(esp_ble_mesh_prov_cb_event_t event, esp_ble_mesh_prov_cb_param_t *param) {
    switch(event) {
        // Provisioning Complete Event
        case ESP_BLE_MESH_NODE_PROV_COMPLETE_EVT:

            // Logs the device provisioning completion and its primary address
            ESP_LOGI(TAG, "provisioned. addr: 0x%04x", param -> node_prov_complete.addr); 
            break;

        // Provisioning Reset Event
        case ESP_BLE_MESH_NODE_PROV_RESET_EVT: 
            ESP_LOGI(TAG, "reset.");

        

            // Creates a FreeRTOS task to reboot the device after a delay
            xTaskCreate(rebootDevice, "Reboot Device", 2048, NULL, 1, NULL); 
            break;
        default:
            break;
    }   
}

void ble_configHandler(esp_ble_mesh_cfg_server_cb_event_t event, esp_ble_mesh_cfg_server_cb_param_t *param){
    // Handles BLE Mesh Configuration Server events
    if(event == ESP_BLE_MESH_CFG_SERVER_STATE_CHANGE_EVT) {
      
        // Access the received configuration message opcode 
        switch(param->ctx.recv_op){

        // Detect when an AppKey is added      
        case ESP_BLE_MESH_MODEL_OP_APP_KEY_ADD:
            ESP_LOGI(TAG, "config: app key added");
            break;
        default:
            break;
      }  
    }
}










// Created attention_callbacks_t Object and initialized current attention states
attention_callbacks_t attentionState = {NULL, NULL};

void health_EventHandler(esp_ble_mesh_health_server_cb_event_t event, esp_ble_mesh_health_server_cb_param_t *param) {
    // Logs current Health Server Event
    ESP_LOGI(TAG, "Health Server Event: %d", event);

    switch(event) {
    // If Health Server Event is ON
    case ESP_BLE_MESH_HEALTH_SERVER_ATTENTION_ON_EVT:

        // Checks if the function pointer attention_on exists or not
        if(attentionState.attention_on != NULL) {

            // Call the function attention_on()
            attentionState.attention_on();
        } 
        break;
    
    // If Health Server Event is OFF
    case ESP_BLE_MESH_HEALTH_SERVER_ATTENTION_OFF_EVT:

        // Checks if the function pointer attention_on exists or not
        if(attentionState.attention_off != NULL) {

            // Call the function attention_off()
            attentionState.attention_off();
        }
        break;
    
    default:
        break;
    }
}

void attention_Handler(attention_callbacks_t callbacks) {
    // Updates attentionState from an Initialized Node BLE function
    attentionState = callbacks;
}