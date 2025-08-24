#ifndef __MAIN_H__
#define __MAIN_H__  

#ifndef F_CPU
#define F_CPU 16000000UL // 16 MHz clock speed  
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

void setupPorts();
void setupTimer0();
void setupTimer1();
void setupExternalInterrupt();
void initUART();
void printResetSource();
void UART_sendString(const char* str);
void UART_sendNumber(uint16_t num);
void UART_send(uint8_t *buf, uint16_t size);
void UART_sendFrequency(uint16_t freq_x10);

#endif // __MAIN_H__