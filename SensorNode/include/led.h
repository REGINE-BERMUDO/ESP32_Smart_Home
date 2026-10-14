#ifndef LED_H
#define LED_H

#include <driver/gpio.h>
#define LED_PIN GPIO_NUM_19


void setupLED(void);
void updateLED(bool);

#endif