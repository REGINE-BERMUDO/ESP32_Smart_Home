#ifndef LED_H
#define LED_H

#include <stdbool.h>
#include "driver/gpio.h"
#include "sdkconfig.h"

// S3 uses GPIO4; classic ESP32 nodes use GPIO26.
#if CONFIG_IDF_TARGET_ESP32S3
#define LED_PIN GPIO_NUM_4
#else
#define LED_PIN GPIO_NUM_26
#endif

#ifdef __cplusplus
extern "C" {
#endif

void setupLED(void);
void updateLED(bool on);

#ifdef __cplusplus
}
#endif

#endif