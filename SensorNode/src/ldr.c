#include "ldr.h"
#include "app_ble.h"
#include <stdatomic.h>

// Keeps previous ldrState to compare with current ldrState
static atomic_bool ldrState_Prev = false;

static void readLDR(void *pvParameters)
{
    TickType_t taskWakeTimeReference = xTaskGetTickCount();
    bool hasPublished = false;
    bool lastPublishedState = false;
    bool candidate = gpio_get_level(LDR_PIN);
    unsigned matchingSamples = 0;
    bool retryWaiting = false;
    TickType_t lastAttempt = 0;

    while (true)
    {
        const bool reading = gpio_get_level(LDR_PIN);
        if (reading != candidate)
        {
            candidate = reading;
            matchingSamples = 1;
        }
        else if (matchingSamples < 4)
        {
            ++matchingSamples;
        }

        // Four readings 50 ms apart reject threshold chatter without a 2 s wait.
        if (matchingSamples == 4)
        {
            atomic_store(&ldrState_Prev, candidate);
            const TickType_t now = xTaskGetTickCount();
            if ((!hasPublished || candidate != lastPublishedState) &&
                (!retryWaiting || now - lastAttempt >= pdMS_TO_TICKS(500)))
            {
                lastAttempt = now;
                retryWaiting = appble_update_state(candidate) != ESP_OK;
                if (!retryWaiting)
                {
                    lastPublishedState = candidate;
                    hasPublished = true;
                    ESP_LOGI("ldr", "Stable light published: %s", candidate ? "dark / ON" : "bright / OFF");
                }
            }
        }

        xTaskDelayUntil(&taskWakeTimeReference, pdMS_TO_TICKS(50));
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
    ESP_ERROR_CHECK(gpio_config(&config));

    // Let FreeRTOS Task manage readLDR function
    if (xTaskCreate(readLDR, "LDR Module", 2048, NULL, 5, NULL) != pdPASS)
    {
        ESP_ERROR_CHECK(ESP_ERR_NO_MEM);
    }
}

bool sendLDR(void) {
    return atomic_load(&ldrState_Prev);
}
