#include "ldr.h"
#include "app_ble.h"

// Keeps previous ldrState to compare with current ldrState
static bool ldrState_Prev = false;

static void readLDR(void *pvParameters) {
    // Records time of task initialization as reference for xtaskDelayUntil()
    TickType_t taskWakeTimeReference = xTaskGetTickCount();

    while(true) {
        // Prevents unnecessary updates by checking if ldrState changed
        bool ldrState_Current = gpio_get_level(LDR_PIN);
        if(ldrState_Current != ldrState_Prev) {
            ldrState_Prev = ldrState_Current;
        }

        // Publish the latest reading every two seconds.
        appble_update_state(ldrState_Current);

        // Aligns task execution interval
        xTaskDelayUntil(&taskWakeTimeReference, pdMS_TO_TICKS(2000));
    }
}

void setupLDR(void) {
    // LDR Pin Configuration
    gpio_config_t config = {
        .pin_bit_mask = (1ULL << LDR_PIN),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_ENABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    // Initialize LDR Pin Configuration
    gpio_config(&config);

    // Let FreeRTOS Task manage readLDR function
    xTaskCreate(readLDR, "LDR Module", 2048, NULL, 5, NULL);
}

bool sendLDR(void) {
    return ldrState_Prev;
}