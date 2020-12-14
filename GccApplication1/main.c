/***********************************************************************
 * 
 * Analog-to-digital conversion with displaying result on LCD and 
 * transmitting via UART.
 * ATmega328P (Arduino Uno), 16 MHz, AVR 8-bit Toolchain 3.6.2
 *
 * Copyright (c) 2020-2021 Pastusek Vaclav, Michal Krystof
 * 
 **********************************************************************/

/* Defines -----------------------------------------------------------*/
/*#define COL_0	PD2
#define COL_1	PD1
#define COL_2	PD0
//#define AUDIO	PC0
#define RELAY	PC1
#define ROW_0	PC2
#define ROW_1	PC3
#define ROW_2	PC4
#define ROW_3	PC5*/


/* Includes ----------------------------------------------------------*/
#include <avr/io.h>         // AVR device-specific IO definitions
#include <avr/interrupt.h>  // Interrupts standard C library for AVR-GCC
#include "gpio.h"           // GPIO library for AVR-GCC
#include "timer.h"          // Timer library for AVR-GCC
#include "lcd.h"            // Peter Fleury's LCD library
#include <stdlib.h>         // C library. Needed for conversion function
#include "uart.h"           // Peter Fleury's UART library
#include "stdbool.h"

//#include "keypadscaner.h"				// KeyPad scaner
#ifndef F_CPU
#define F_CPU 16000000
#endif

#include <util/delay.h>     // Functions for busy-wait delay loops

/* Variables ---------------------------------------------------------*/
// Custom character definition using https://omerk.github.io/lcdchargen/
uint8_t customChar[8] = {
	0b00100,
	0b01110,
	0b11111,
	0b11111,
	0b11111,
	0b00100,
	0b01110,
	0b11111
};


/* States */
typedef enum{
	RESET,
	GET_CODE,
	CHECK_CODE,
	DOOR_OPEN,
	WRONG_CODE
} type_state;

type_state current_state = RESET;
char wrong_tries = 0;



uint8_t getkey()
{
	uint8_t row, col;
	DDRC&=~(0x7F);
	PORTC|=0x0F;
	for(col=0;col<3;col++)
	{
		DDRC|=(0x10<<col);
		if(!GPIO_read(&PINC, 0))
			return(0);
		for(row=0;row<4;row++)
			if(!GPIO_read(&PINC, row))
				return(row*3+col);
		DDRC&=~(0x10<<col);
	}
	return 12;//Indicate No key pressed
}


/* Function definitions ----------------------------------------------*/
/**
 * Main function where the program execution begins. Use Timer/Counter1
 * and start ADC conversion four times per second. Send value to LCD
 * and UART.
 */
int main(void)
{	
	// Initialize LCD display
	lcd_init(LCD_DISP_ON);
	// Set pointer to beginning of CGRAM memory
	lcd_command(1 << LCD_CGRAM);
	for (uint8_t i = 0; i < 8; i++) //0,1,2,3 ,4,5,6,7
	{
		// Store all new chars to memory line by line
		lcd_data(customChar[i]);
	}
	/*for (uint8_t j = 7; j >= 0; j--) //7,6,5,4, 3,2,1,0
	{
		// Store all new chars to memory line by line
		lcd_data(customChar[j]);
	}*/
	// Set DDRAM address
	lcd_command(1 << LCD_DDRAM);
	
	// Display custom characters
	lcd_putc(0);
	lcd_gotoxy(15, 0);
	lcd_putc(0);
	
    lcd_gotoxy(1, 0); 
	lcd_puts("Password:____");


    // Configure 16-bit Timer/Counter1 to start ADC conversion
    // Enable interrupt and set the overflow prescaler to 262 ms
	TIM1_overflow_262ms();
	TIM1_overflow_interrupt_enable();
	
	TIM2_overflow_4ms();
	TIM2_overflow_interrupt_enable();
	
	
	
    // Initialize UART to asynchronous, 8N1, 9600
	uart_init(UART_BAUD_SELECT(9600, F_CPU));

    // Enables interrupts by setting the global interrupt mask
    sei();

    // Infinite loop
    while (1)
    {
        /* Empty loop. All subsequent operations are performed exclusively 
         * inside interrupt service routines ISRs */
    }

    // Will never reach this
    return 0;
}

/* Interrupt service routines ----------------------------------------*/
/**
 * ISR starts when Timer/Counter1 overflows. Use single conversion mode
 * and start conversion four times per second.
 */
ISR(TIMER1_OVF_vect)
{
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	
	static uint16_t counter = 0;
	
	counter++;
	lcd_gotoxy(14, 1);
	if(counter == 4){ //4/4=1s
		//TIM1_overflow_interrupt_disable();
		//GPIO_toggle(&PORTC, RELAY);
		counter = 0;
		//char sss = "RESET";
	
			// Send to uart in decimal
			uart_puts("Actual state:");
			
			char cislo[4] = "   ";
			uint8_t number = getkey();
			itoa(number, cislo, 10);
			uart_puts(cislo);
			uart_puts("\n");
			//key=getkey();
			//itoa(key, str, 10);
			//lcd_puts(str);
	}
	


}

/* -------------------------------------------------------------------*/
/**
 * ISR starts when ADC completes the conversion. Display value on LCD
 * and send it to UART.
 */
ISR(TIMER2_OVF_vect)
{
   
   static uint16_t counter = 0;
   
   counter++;
   lcd_gotoxy(14, 2);
   if(counter == 2500){ //4ms
	   TIM1_overflow_interrupt_enable();
	   lcd_putc('Z');
   }
   
   
   
   /** // WRITE YOUR CODE HERE
	uint16_t value;
	char lcd_string[5];
	char parity = 0;
	value = ADC;
	bool b[8]; //bits

	for (int j = 0;  j < 8;  j++)
		b[j] = 0 != (value & (1 << j));
		
	// Print parity bit
	parity = b[0]^b[1]^b[2]^b[3]^b[4]^b[5]^b[6]^b[7];
	lcd_gotoxy(14, 1);
	itoa(parity, lcd_string, 10);
	lcd_puts(lcd_string);
	
	// Print on LCD in decimal
	itoa(value, lcd_string, 10);
	lcd_gotoxy(8, 0);
	lcd_puts("    ");
	lcd_gotoxy(8, 0);
	lcd_puts(lcd_string);
	
	if(value < 700)
	{
		// Send to uart in decimal
		uart_puts("ADC value in decimal: ");
		uart_puts(lcd_string);
		uart_puts("\n");
	}
	itoa(value, lcd_string, 16);
	lcd_gotoxy(13, 0);
	lcd_puts("    ");
	lcd_gotoxy(13, 0);
	lcd_puts(lcd_string);
	
	
	// Print what is pressed
	lcd_gotoxy(8, 1);
	lcd_puts("      ");
	if(value >= 1023-8)
	{
		lcd_gotoxy(8, 1);
		lcd_puts("None  ");
	}
	else if(value >= 651-8)
	{
		lcd_gotoxy(8, 1);
		lcd_puts("Select");
	}
	else if(value >= 403-8)
	{
		lcd_gotoxy(8, 1);
		lcd_puts("Left  ");
	}
	else if(value >= 246-8)
	{
		lcd_gotoxy(8, 1);
		lcd_puts("Down  ");
	}
	else if(value >= 101-8)
	{
		lcd_gotoxy(8, 1);
		lcd_puts("Up");
	}
	else
	{
		lcd_gotoxy(8, 1);
		lcd_puts("Right ");
	}
*/	
}