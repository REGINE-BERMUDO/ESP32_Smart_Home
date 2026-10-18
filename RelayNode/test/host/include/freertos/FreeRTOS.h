#pragma once
#include <cstdint>
using TickType_t = unsigned;
using TaskHandle_t = void *;
constexpr int pdTRUE = 1, pdPASS = 1;
constexpr unsigned portMAX_DELAY = ~0U;
#define pdMS_TO_TICKS(ms) (ms)
int xTaskCreate(void (*)(void *), const char *, unsigned, void *, unsigned, TaskHandle_t *);
unsigned ulTaskNotifyTake(int, TickType_t);
void xTaskNotifyGive(TaskHandle_t);
