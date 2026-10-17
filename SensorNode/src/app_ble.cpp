#include "app_ble.h"

void initializeBLEMesh(attention_callbacks_t cbs) {
    // Set Initial attention LED State
    attention_Handler(cbs);
}