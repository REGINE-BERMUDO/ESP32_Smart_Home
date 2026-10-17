#include "relay.h"

void setupRelay(void) {
    gpio_config_t config ={
        .pin_bit_mask = (1ULL << RELAY_PIN),   
        .mode = GPIO_MODE_OUTPUT,           
        .pull_up_en = GPIO_PULLUP_DISABLE,   // Disable pull-up resistor
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Disable pull-down resistor
        .intr_type = GPIO_INTR_DISABLE       
    };
    
    gpio_config(&config); 
}


// Control the relay using its signal pin.
// Assumes HIGH turns it ON; hardware testing is still needed.
void setRelay(bool on)
{
    gpio_set_level(RELAY_PIN, on ? 1 : 0);
}