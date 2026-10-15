#ifndef LED_H
#define LED_H

#include <driver/gpio.h>

#define LED_PIN GPIO_NUM_19

// Sets up LED Pin
void setupLED(void);

// Update LED state callback function for BLE Mesh
void updateLED(bool);

#endif