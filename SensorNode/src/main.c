#include "app_ble.h"
#include "ldr.h"
#include "led.h"

static void  attn_on(void) {
    updateLED(true);
}

static void attn_off(void) {
    updateLED(false);
}

void app_main() {
    setupLDR();   
    setupLED();

    attention_callbacks_t cbs = {attn_on, attn_off};
    initializeBLEMesh(cbs);
}