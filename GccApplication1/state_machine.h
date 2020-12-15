#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_


#include <avr/io.h>         // AVR device-specific IO definitions
#include <avr/interrupt.h>  // Interrupts standard C library for AVR-GCC
#include "gpio.h"           // GPIO library for AVR-GCC
#include "timer.h"          // Timer library for AVR-GCC
#include "lcd.h"            // Peter Fleury's LCD library
#include <stdlib.h>         // C library. Needed for conversion function
#include "uart.h"           // Peter Fleury's UART library
#include "stdbool.h"
#include <util/delay.h>     // Functions for busy-wait delay loops
#include "state_machine.c"

void reset(void);

uint8_t getkey(void);

void get_code(uint8_t* code);

void state_machine(void);


#endif /* STATE_MACHINE_H_ */