#ifndef RELAY_H
#define RELAY_H

#include <driver/gpio.h>

// GPIO27 controls relay channel 1 through IN1.
#define RELAY_PIN GPIO_NUM_27

void setupRelay(void);

// Control the relay: true means ON, false means OFF.
void setRelay(bool on);

#endif // RELAY_H