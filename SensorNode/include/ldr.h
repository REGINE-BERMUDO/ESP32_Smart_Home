#ifndef LDR_H
#define LDR_H

#include <driver/gpio.h>  

// Implementation for setting up LDR
void setupLDR(void);

// Implementation for reading LDR
bool readLDR(void);

#endif