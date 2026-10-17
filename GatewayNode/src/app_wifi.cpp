#include "app_wifi.h"
#include "secrets.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_log.h"

// Label messages printed by this module.
static const char *TAG = "gateway_wifi";

// React to Wi-Fi startup, disconnection, and IP assignment.
static void wifiEventHandler(
    void *arg,
    esp_event_base_t eventBase,
    int32_t eventID,
    void *eventData)
{
    if (eventBase == WIFI_EVENT && eventID == WIFI_EVENT_STA_START)
    {
        // Begin connecting when the Wi-Fi driver starts.
        ESP_ERROR_CHECK(esp_wifi_connect());
    }
    else if (eventBase == WIFI_EVENT &&
             eventID == WIFI_EVENT_STA_DISCONNECTED)
    {
        // Try connecting again if the connection is lost.
        ESP_LOGI(TAG, "Wi-Fi disconnected; retrying");
        ESP_ERROR_CHECK(esp_wifi_connect());
    }
    else if (eventBase == IP_EVENT && eventID == IP_EVENT_STA_GOT_IP)
    {
        // Display the address we will later open in a browser.
        ip_event_got_ip_t *event =
            static_cast<ip_event_got_ip_t *>(eventData);

        ESP_LOGI(TAG, "Gateway IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

// Prepare networking and register our connection-event handler.
void setupWiFi(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create the station interface: the ESP32 joins an existing router.
    esp_netif_create_default_wifi_sta();

    // Initialize the Wi-Fi driver with default settings.
    wifi_init_config_t config = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&config));

    // Receive Wi-Fi events and successful IP assignment events.
    ESP_ERROR_CHECK(esp_event_handler_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, wifiEventHandler, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, wifiEventHandler, nullptr));


    // Start with empty Wi-Fi settings.
    wifi_config_t wifiConfig = {};

    // Check that the credentials fit the driver's buffers.
    static_assert(sizeof(WIFI_SSID) - 1 <= sizeof(wifiConfig.sta.ssid),
                  "Wi-Fi name is too long");
    static_assert(sizeof(WIFI_PASSWORD) - 1 <= sizeof(wifiConfig.sta.password),
                  "Wi-Fi password is too long");

    // Copy the credentials from our local secrets.h file.
    memcpy(wifiConfig.sta.ssid, WIFI_SSID, sizeof(WIFI_SSID) - 1);
    memcpy(wifiConfig.sta.password, WIFI_PASSWORD, sizeof(WIFI_PASSWORD) - 1);

    // Join an existing network and start the Wi-Fi driver.
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifiConfig));
    ESP_ERROR_CHECK(esp_wifi_start());
    
}