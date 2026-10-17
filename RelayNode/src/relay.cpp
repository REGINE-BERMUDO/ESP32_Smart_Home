#include "relay.h"

void setupRelay(void) {
    gpio_config_t config ={
        .pin_bit_mask = (1ULL << RELAY_PIN),   
        .mode = GPIO_MODE_OUTPUT,           
        .pull_up_en = GPIO_PULLUP_DISABLE,   // Disable pull-up resistor
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // Disable pull-down resistor
        .intr_type = GPIO_INTR_DISABLE       
    };
    
    // Prepare HIGH (OFF) before enabling the output.
    gpio_set_level(RELAY_PIN, 1);
    
    gpio_config(&config); 
   
}


// Control channel 1: LOW turns it ON, HIGH turns it OFF.
void setRelay(bool on)
{
    gpio_set_level(RELAY_PIN, on ? 0 : 1);
}