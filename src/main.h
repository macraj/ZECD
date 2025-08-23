#ifndef __MAIN_H__
#define __MAIN_H__  

#ifndef F_CPU
#define F_CPU 16000000UL // 16 MHz clock speed  
#endif

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

void setupPorts();
void setupTimer0();
void setupTimer1();
void setupExternalInterrupt();
void initUART();
void printResetSource();
void UART_send(char data);
void UART_sendString(const char* str);
void UART_sendNumber(uint8_t num);

#endif // __MAIN_H__