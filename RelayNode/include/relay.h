#ifndef RELAY_H
#define RELAY_H

#include <driver/gpio.h>

#define RELAY_PIN GPIO_NUM_22

void setupRelay(void);

// Control the relay: true means ON, false means OFF.
void setRelay(bool on);

#endif // RELAY_H