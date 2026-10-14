#ifndef LDR_H
#define LDR_H

#define LDR_PIN GPIO_NUM_23

#include <driver/gpio.h>
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