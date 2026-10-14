#ifndef RELAY_H
#define RELAY_H

#include <driver/gpio.h>

#define RELAY_PIN GPIO_NUM_22

void setupRelay(void);

#endif // RELAY_H