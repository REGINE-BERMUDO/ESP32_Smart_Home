#include "ldr.h"

void setupLDR(void) {
    gpio_config_t config = {
        .pin_bit_mask = (1ULL << GPIO_NUM_34), // Assuming LDR is connected to GPIO34
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&config);
}

bool readLDR(void) {
    bool value = gpio_get_level(GPIO_NUM_34);
    return value;
}