#ifndef LDR_H
#define LDR_H

#define LDR_PIN GPIO_NUM_34

#include <driver/gpio.h>  

// Implementation for setting up LDR
void setupLDR(void);

// Implementation for reading LDR
bool readLDR(void);

#endif