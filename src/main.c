
#include "main.h"


int main(void) {
    // Initialize ports
    DDRB = 0xFF; // Set PORTB as output
    PORTB = 0x00; // Initialize PORTB to 0

    // Enable global interrupts
    sei();

    while (1) {
        // Main loop
        PORTB ^= 0xFF; // Toggle all pins on PORTB
        _delay_ms(500); // Wait for 500 milliseconds
    }

    return 0;
}
