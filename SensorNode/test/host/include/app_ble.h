#pragma once
#include <stdbool.h>
#include <assert.h>
#define ESP_OK 0
#define ESP_ERR_NO_MEM 1
#define ESP_ERROR_CHECK(result) assert((result) == ESP_OK)
#define ESP_LOGI(...) ((void)0)
int appble_update_state(bool state);
