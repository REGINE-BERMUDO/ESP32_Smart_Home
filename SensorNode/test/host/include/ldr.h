#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef uint32_t TickType_t;
typedef struct {
    uint64_t pin_bit_mask;
    int mode, pull_up_en, pull_down_en, intr_type;
} gpio_config_t;
#define LDR_PIN 27
#define GPIO_MODE_INPUT 1
#define GPIO_PULLUP_DISABLE 0
#define GPIO_PULLDOWN_ENABLE 1
#define GPIO_INTR_DISABLE 0
#define pdPASS 1
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
int gpio_get_level(int pin);
int gpio_config(const gpio_config_t *config);
TickType_t xTaskGetTickCount(void);
void xTaskDelayUntil(TickType_t *reference, TickType_t delay);
int xTaskCreate(void (*task)(void *), const char *, unsigned, void *, unsigned, void *);
