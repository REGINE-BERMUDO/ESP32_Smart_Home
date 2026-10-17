#include "relay.h"
#include "esp_log.h"

extern "C" void app_main(void)
{
    // Initialize the relay output to OFF.
    setupRelay();

    // Report startup without automatically switching the relay.
    ESP_LOGI("relay", "Relay output initialized to OFF");
}