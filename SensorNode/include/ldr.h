#ifndef LDR_H
#define LDR_H

// Rename GPIO_NUM_23 to LDR_PIN
#define LDR_PIN GPIO_NUM_23

// For GPIO Setup
#include <driver/gpio.h>

// FreeRTOS dependencies
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// Implementation for setting up LDR
void setupLDR(void);

/* Implementation for reading LDR
static void readLDR(void *pvParameters);
*/

// LDR Service for sending its current state
bool sendLDR(void);

#endif