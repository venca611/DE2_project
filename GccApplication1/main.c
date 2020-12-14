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
#define AUDIO	PB2
#define RELAY	PB3



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
uint8_t customChar[8*2] = {
	0b00100,
	0b01110,
	0b11111,
	0b11111,
	0b11111,
	0b00100,
	0b01110,
	0b00000,
	
	
	
	0b01110,
	0b00100,
	0b11111,
	0b11111,
	0b11111,
	0b01110,
	0b00100,
	0b00000
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

void reset(void)
{
	
	
	// Set pointer to beginning of CGRAM memory
	lcd_command(1 << LCD_CGRAM);
	for (uint8_t i = 0; i < 8*2; i++) //0,1,2,3 ,4,5,6,7
	{
		// Store all new chars to memory line by line
		lcd_data(customChar[i]);
	}
	// Set DDRAM address
	lcd_command(1 << LCD_DDRAM);

	// Initialize LCD display
	lcd_clrscr();
	// Display custom characters
	lcd_putc(0);
	lcd_gotoxy(15, 0);
	lcd_putc(0);
	lcd_gotoxy(0, 1);
	lcd_putc(1);
	lcd_gotoxy(15, 1);
	lcd_putc(1);
	
	//lcd_gotoxy(1, 1);
	//lcd_puts("Welcome Bachhh");
	

	lcd_gotoxy(1, 0);
	lcd_puts("Password:____");
	//_delay_ms(2000);
}

uint8_t getkey()
{
	uint8_t row, col;
	DDRC&=~(0x7F);
	PORTC|=0x0F;
	for(col=0;col<3;col++)
	{
		DDRC|=(0x10<<col);
		if(!GPIO_read(&PINC, 0))
			return(-1);
		for(row=0;row<4;row++)
			if(!GPIO_read(&PINC, row))
				return(row*3+col+1);
		DDRC&=~(0x10<<col);
	}
	return 0;//Indicate No key pressed
}

void get_code(uint8_t* code)
{
	TIM2_overflow_16ms()
	TIM2_overflow_interrupt_enable();
	uint8_t key = getkey();
	if (key!=0)
	{
		switch(key)
		{
			case 12:
				TIM2_overflow_interrupt_disable();
				current_state = CHECK_CODE; //ma se provest kontrola hesla a pripadne dalsi zmeny
				break;
			case 10:
				for(uint8_t i=3;i>=0;i--)
					if(code[i]!=10)
					{
						code[i]=10;
						break;
					}
				_delay_ms(250);
					break;
			default:
				for(uint8_t j=0;j<4;j++)
					if(code[j]==10)
					{
						code[j]=key;
						break;
					}
				_delay_ms(250);
		} //pokud nedochazi ke kontrole hesla, je treba vlozit malou pauzu (cca 0,5s), aby nedochazelo k duplikaci stisknuteho tlacitka
	}
char password[] = "    ";
    lcd_gotoxy(10, 0);
    for(uint8_t i = 0; i < 4; i++){
        password[i] = (code[i] == 10)? '_': '*';
    }
    lcd_puts(password);   
}

bool check_code(uint8_t* code)
{
	//codes are 4242, 0123, 9876
	if((code[0]==4)&&(code[1]==2)&&(code[2]==4)&&(code[3]==2))
	{
		lcd_gotoxy(1, 1);
		lcd_puts("Welcome User1");
		return 1;
	}
	else if((code[0]==11)&&(code[1]==1)&&(code[2]==2)&&(code[3]==3))
	{
		lcd_gotoxy(1, 1);
		lcd_puts("Welcome User2");
		return 1;
	}
	else if((code[0]==9)&&(code[1]==8)&&(code[2]==7)&&(code[3]==4))
	{
		lcd_gotoxy(1, 1);
		lcd_puts("Welcome User3");
		return 1;
	}
	else return 0;
	
	
}

//funkce a procedury
void state_machine(void)
{
	static uint8_t code[4]={10,10,10,10};
	switch (current_state)
	{
		case RESET:
			reset();
			for(uint8_t i = 0; i < 4; i++)
				code[i]=10;
			current_state = GET_CODE;
			break;
		case GET_CODE:
			get_code(code);
			break;
		case CHECK_CODE:
			current_state = check_code(code)?DOOR_OPEN:WRONG_CODE;
			break;
		case DOOR_OPEN:		
			break;
		case WRONG_CODE:		
			break;
		default:
			current_state = RESET;
			
	}
}



/* Function definitions ----------------------------------------------*/
/**
 * Main function where the program execution begins. Use Timer/Counter1
 * and start ADC conversion four times per second. Send value to LCD
 * and UART.
 */
int main(void)
{	
	
	DDRB|=(0x0C);
	PORTB&=~(0x0C);

    // Configure 16-bit Timer/Counter1 to start ADC conversion
    // Enable interrupt and set the overflow prescaler to 16 ms
	//TIM0_overflow_16us();
	TIM0_overflow_16ms();
	TIM0_overflow_interrupt_enable();
	
	TIM1_overflow_262ms();
	//TIM1_overflow_4ms();
	TIM1_overflow_interrupt_enable();
	
	
	
	lcd_init(LCD_DISP_ON);
	
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
ISR(TIMER0_OVF_vect)
{
	state_machine();
}	
	
	


/* -------------------------------------------------------------------*/
/**
 * UART
 */
ISR(TIMER1_OVF_vect)
{
	static type_state prev_state = RESET;
	uart_puts("");
	if(current_state != prev_state)
   {
	   // Send to uart
	   uart_puts("Current state: ");
	   switch(current_state)
	   {
			case RESET:
				uart_puts("RESET");
				break;
			case GET_CODE:
				uart_puts("GET_CODE");
				break;
			case CHECK_CODE:
				uart_puts("CHECK_CODE");
				break;
			case DOOR_OPEN:
				uart_puts("DOOR_OPEN");
				break;
			case WRONG_CODE:
				uart_puts("WRONG_CODE");
				break;
			default:
				uart_puts("ERROR");
	   }
	   uart_puts("\n");
   }
   
   prev_state = current_state;
}

// 4ms / 16ms counter
ISR(TIMER2_OVF_vect)
{
	char str[] = "  ";
	static uint8_t wrong_tries = 0;
	static uint32_t count = 0;
	if(count == 0){
		if(current_state == DOOR_OPEN)
			GPIO_toggle(&PORTB, RELAY);
		else if(current_state == WRONG_CODE)
		{
					wrong_tries++;
					lcd_clrscr();
					lcd_putc(0);
					lcd_gotoxy(15, 0);
					lcd_putc(0);
					lcd_gotoxy(0, 1);
					lcd_putc(1);
					lcd_gotoxy(15, 1);
					lcd_putc(1);
					lcd_gotoxy(1, 0);
					lcd_puts("ACCESS DENIED");
					lcd_gotoxy(1, 1);
					lcd_puts("WRONG TRIES:");
					lcd_gotoxy(13, 1);
					itoa(wrong_tries, str, 10);
					lcd_puts(str);
					if(wrong_tries == 100)
						wrong_tries = 0;
					if(wrong_tries > 4)
						GPIO_toggle(&PORTB, AUDIO);
		}
	}
	
	if(count == 1250)//1250*16ms=20s //1250*4ms = 5s
	{
		current_state = RESET;
		if(current_state == DOOR_OPEN)
			GPIO_toggle(&PORTB, RELAY);
		else if(current_state == WRONG_CODE && wrong_tries > 4)
			GPIO_toggle(&PORTB, AUDIO);		
		TIM2_overflow_interrupt_disable();
	}
	count++;

}