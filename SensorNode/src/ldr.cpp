#include "ldr.h"

static bool ldrState_Prev = false;

static void readLDR(void *pvParameters) {
    TickType_t taskWakeTimeReference = xTaskGetTickCount();

    while(true) {
        bool ldrState_Current = gpio_get_level(LDR_PIN);

        if(ldrState_Current != ldrState_Prev) {
            ldrState_Prev = ldrState_Current;
        }

        xTaskDelayUntil(&taskWakeTimeReference, pdMS_TO_TICKS(2000));
    }
}

void setupLDR(void) {
    gpio_config_t config = {
        .pin_bit_mask = (1ULL << LDR_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&config);

    xTaskCreate(readLDR, "LDR Module", 2048, NULL, 5, NULL);
}

bool sendLDR(void) {
    return ldrState_Prev;
}