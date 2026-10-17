#ifndef LED_H
#define LED_H

// Rename GPIO_NUM_19 to LED_PIN
#define LED_PIN GPIO_NUM_19

// For GPIO Setup
#include <driver/gpio.h>

// Sets up LED Pin
void setupLED(void);

// Update LED state callback function for BLE Mesh
void updateLED(bool);

#endif