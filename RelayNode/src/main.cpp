#include "relay.h"
#include "esp_log.h"

extern "C" void app_main(void)
{
    // Configure the relay output with its initial OFF level.
    setupRelay();

    // Print a startup message in the Serial Monitor.
    ESP_LOGI("relay", "Relay output initialized to OFF");
}