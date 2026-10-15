#include "led.h"

void setupLED(void) {
    // LED Pin Configuration
    gpio_config_t config ={
        .pin_bit_mask = (1ULL << LED_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    // Initialize LED Pin Configuration
    gpio_config(&config);
}

void updateLED(bool state) {
    // Sets LED Pin current state
    gpio_set_level(LED_PIN, state);
}