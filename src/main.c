
#include "main.h"

// Pin definitions
#define PULSE_PIN PD2  // INT0 on PD2
#define LED_PIN PB5
#define FREQ_THRESHOLD 10 // Frequency threshold for power detection

typedef enum {  UNKNOWN, NO_POWER, POWER_OK, POWER_ERR, POWER_RDY } states;
states state = UNKNOWN;
enum { NONE, SHORT, LONG };
uint8_t outageDuration = NONE;

volatile uint16_t pulseCount = 0;
volatile uint8_t secondsCounter = 0;
volatile uint8_t measurementReady = 0;
// Variables for interrupt handling
volatile bool send_busy = false;
volatile uint8_t *send_p, *send_e;

int main(void) {
    // Initialize UART for serial communication
    initUART();
    // Initialize ports
    setupPorts();
    // Initialize Timer0 for pulse measurement
    setupTimer0();
    // Initialize Timer1 for 1-second intervals
    setupTimer1();
    // Initialize external interrupt for pulse detection
    setupExternalInterrupt();
    _delay_ms(100); // Wait for UART to stabilize
     // Print reset source  
    printResetSource();

    // Enable global interrupts
    sei();

    while (1) {
        // Main loop
        if (secondsCounter % 2 == 0) PINB |= (1 << PB5);    // Toggle LED every 2 seconds
        
        if (measurementReady) {
            measurementReady = 0; // Clear the flag

            // Calculate frequency in Hz with one decimal place
            uint16_t frequency_x10 = (pulseCount * 10); // Frequency in tenths of Hz

            // Send frequency via UART
            UART_sendFrequency(frequency_x10);

            // Reset pulse count for next measurement
            pulseCount = 0;
        } 
    }

    return 0;
}

// Initialize ports
void setupPorts() {
    // Set LED pin as output
    DDRB |= (1 << LED_PIN);
    
    // Set pulse pin as input
    DDRD &= ~(1 << PULSE_PIN);
}

void setupTimer0() {
  // Clear registers
  TCCR0A = 0;
  TCCR0B = 0;
  TCNT0 = 0;

  // 2000 Hz (16000000/((124+1)*64))
  OCR0A = 124;
  // CTC
  TCCR0A |= (1 << WGM01);
  // Prescaler 64
  // TCCR0B |= (1 << CS01) | (1 << CS00); // Start Timer0 later in INT0
  // Output Compare Match A Interrupt Enable
  TIMSK0 |= (1 << OCIE0A);
}

/* Timer1 setup for 1-second intervals (more precise for 16MHz) */
void setupTimer1() {
    // Set Timer1 to CTC mode with OCR1A as top
    TCCR1A = 0;
    TCCR1B = (1 << WGM12) | (1 << CS12) | (1 << CS10); // CTC mode, prescaler 1024
    
    // Set compare value for 1-second interval (16MHz clock)
    // 16000000 / 1024 = 15625 Hz timer frequency
    // 15625 - 1 = 15624 (1 second)
    OCR1A = 15624;
    
    // Enable Timer1 compare interrupt
    TIMSK1 = (1 << OCIE1A);
}

// External interrupt setup for INT0 (PD2)
void setupExternalInterrupt() {
    // Configure INT0 for falling edge trigger
    EICRA = (1 << ISC01);
    EIMSK = (1 << INT0);
    
    // Enable pull-up resistor on PD2
    PORTD |= (1 << PULSE_PIN);
}

// Initialize UART for 115200 baud with 16MHz clock
void initUART() {
    // Set baud rate to 115200 for 16MHz
    // Calculation: (16000000/(16*115200))-1 = 7.68 -> use 8
    UBRR0H = 0;
    UBRR0L = 16;
    
    // Enable double speed for more accurate baud rate
    UCSR0A = (1 << U2X0);
    
    // Enable transmitter
    UCSR0B = (1 << TXEN0);
    
    // Set frame format: 8 data bits, 1 stop bit, no parity
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}

void printResetSource() {
    uint8_t resetSource = MCUSR;
    
    UART_sendString("Reset source: ");
    
    if (resetSource & (1 << PORF)) {
        UART_sendString("Power-On Reset");
    }
    else if (resetSource & (1 << EXTRF)) {
        UART_sendString("External Reset");
    }
    else if (resetSource & (1 << BORF)) {
        UART_sendString("Brown-Out Reset");
    }
    else if (resetSource & (1 << WDRF)) {
        UART_sendString("Watchdog Reset");
    }
    else {
        UART_sendString("Unknown");
    }
    
    UART_sendString("\r\n");
    
    // Clear reset flags
    MCUSR = 0;
}

void UART_sendNumber(uint16_t num) {
    char buffer[10]; // Maximum 5 digits for uint16_t + null terminator
    char *p = &buffer[sizeof(buffer) - 1]; // Pointer to the end of the buffer
    *p = '\0'; // Null-terminate the string

    do {
        *(--p) = (num % 10) + '0'; // Convert digit to character
        num /= 10;
    } while (num != 0);

    // Send the string
    UART_send((uint8_t*)p, strlen(p));
}

void UART_sendString(const char* str) {
    UART_send((uint8_t*)str, strlen(str));
}

/* Multi-byte send function */
void UART_send(uint8_t *buf, uint16_t size){
    while (send_busy);
    send_busy = true;
    send_p = buf;
    send_e = buf + size;
    UCSR0B |= (1 << UDRIE0);
}

void UART_sendFrequency(uint16_t freq_x10) {
    char buffer[20]; // Bufor na cały komunikat
    char *p = buffer; // Wskaźnik na początek bufora
    
    // Step 1: Copy "Frequency: "
    const char *prefix = "Frequency: ";
    while (*prefix) {
        *p++ = *prefix++;
    }
    
    // Step 2: Convert integer part to string
    uint16_t integerPart = freq_x10 / 10;
    char temp_num_buf[6]; // Bufor tymczasowy
    char *temp_p = &temp_num_buf[sizeof(temp_num_buf) - 1];
    *temp_p = '\0';
    
    do {
        *(--temp_p) = (integerPart % 10) + '0';
        integerPart /= 10;
    } while (integerPart != 0);

    while (*temp_p) {
        *p++ = *temp_p++;
    }
    
    // Step 3: Add decimal point and fractional part
    *p++ = '.';
    uint8_t fractionalPart = freq_x10 % 10;
    *p++ = fractionalPart + '0';
    
    // Step 4: Add " Hz\r\n"
    const char *suffix = " Hz\r\n";
    while (*suffix) {
        *p++ = *suffix++;
    }
    
    // Send the complete message
    UART_send((uint8_t*)buffer, p - buffer);
}

/* Timer1 interrupt service routine */
ISR(TIMER1_COMPA_vect) {
  secondsCounter++; // Increment seconds counter
  // Set flag to indicate new measurement is ready
  measurementReady = 1;
}

/* External interrupt service routine for INT0 */
ISR(INT0_vect) {
  // Start Timer0 on falling edge
  TCNT0 = 0; // Reset Timer0 counter
  TCCR0B |= (1 << CS01) | (1 << CS00); // Start Timer0 with prescaler 64
  EIMSK &= ~(1 << INT0); // Disable INT0 interrupt
}

/* Timer0 interrupt service routine */
// Stop Timer0 when it reaches the compare value
ISR(TIMER0_COMPA_vect) {
  TCCR0B = 0; // Stop Timer0
  // Check if pin is still low after delay
  if (!(PIND & (1 << PULSE_PIN))) pulseCount++; // Valid pulse detected
  EIMSK = (1 << INT0); // Re-enable INT0 interrupt  
} 
 
// USART Data Register Empty interrupt for ATmega328P
ISR(USART_UDRE_vect){
    UDR0 = *send_p++;
    if (send_p == send_e){
        UCSR0B &= ~(1 << UDRIE0);
        send_busy = false;
    }
}