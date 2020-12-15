/**
* @mainpage
 * Keypad scanner using 4x3 keypad, LCD Hd44780 and a door relay
 *
 * ATmega328P (Arduino Uno), 16 MHz, AVR 8-bit Toolchain 3.6.2
 *
 * @author Pastusek Vaclav, Michal Krystof
 
 * @copyright (c) 2020-2021 Pastusek Vaclav, Michal Krystof
 * 
 */

/* Includes ----------------------------------------------------------*/
#include <avr/io.h>         // AVR device-specific IO definitions
#include <avr/interrupt.h>  // Interrupts standard C library for AVR-GCC
#include "gpio.h"           // GPIO library for AVR-GCC
#include "timer.h"          // Timer library for AVR-GCC
#include "lcd.h"            // Peter Fleury's LCD library
#include <stdlib.h>         // C library. Needed for conversion function
#include "uart.h"           // Peter Fleury's UART library
#include "stdbool.h"

#ifndef F_CPU
#define F_CPU 16000000
#endif

#include <util/delay.h>     // Functions for busy-wait delay loops

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

/* Defines -----------------------------------------------------------*/

/** 
* @brief List of states the machine will reach
*/
typedef enum{
	RESET,
	GET_CODE,
	CHECK_CODE,
	DOOR_OPEN,
	WRONG_CODE
} type_state;

type_state current_state = RESET;
uint32_t counter2 = 0;
uint32_t wrong_tries = 0;
/**
* @brief performs reset of the machine to the default state
* @return none
* @par Resets the display and prepares it for the GET_CODE state.
*/
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
	

	lcd_gotoxy(1, 0);
	lcd_puts("Password:____");
	//_delay_ms(2000);
}
/**
* @brief Captures the event of pressing a button on a keypad
* @return Numbers 0 to 12
* @par Enables high output value on three pins connected to columns one by one, checking each time all the pins connected to rows again one by one. 
If it detects output, it returns the unique number assigned to every combination of a row and a column pin. If it doesn't detect any connection, it returns 0.
*/
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
/**
* @brief Provides logic over the operations over writing and deleting individual characters of the code
* @return None
* @par Constantly runs the getkey() function, checking for input from the keypad. If there is any, operates with it - puts numbers in the code, if it is not full,
 deletes, if the '*'(backspace) character is used and sends the code to be checked if the '#'(enter) is used. Also limits the time to put in the code to 20s.
*/
void get_code(uint8_t* code)
{
	if(code[0] != 10)
	{		
		TIM2_overflow_16ms();
		TIM2_overflow_interrupt_enable();	
	}
	
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
	
	
	if(code[0] == 10)
	{
		TIM2_overflow_interrupt_disable();
		counter2 = 0;
	}	
	
	char password[] = "    ";
    lcd_gotoxy(10, 0);
    for(uint8_t i = 0; i < 4; i++){
        password[i] = (code[i] == 10)? '_': '*';
    }
    lcd_puts(password);   
}
/**
* @brief Checks, if the correct code has been entered
* @return true(1) or false (0)
*/
bool check_code(uint8_t* code)
{
	char welcome[] = "Welcome UserX";
	uint8_t correct_counter = 0;
	//codes are 4242, 0123, 9876
	uint8_t correct_password[3][4] = {{4,2,4,2},{11,1,2,3},{9,8,7,6}};
	for(uint8_t i = 0;i < 3; i++)
	{
		for(uint8_t j = 0; j < 4; j++)
			if(correct_password[i][j] == code[j])
				correct_counter++;
		if(correct_counter == 4)
		{
			lcd_gotoxy(1, 1);
			welcome[12] = (char) i + '0';
			lcd_puts(welcome);
			return 1;
		}
	}
	return 0;
}

/**
* @brief Switches the different states of the machine
* @return None
*/
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
			TIM2_overflow_interrupt_disable();
			TIM2_overflow_4ms();
			TIM2_overflow_interrupt_enable();
			break;
		case DOOR_OPEN:		
			break;
		case WRONG_CODE:		
			break;
		default:
			current_state = RESET;		
	}
}
/**
 * @brief Initializes the lcd and prepares the relay, runs state machine.
 * @return None
 */
int main(void)
{	
	DDRB|=(0x08);
	PORTB&=~(0x08);

	TIM0_overflow_16ms();
	TIM0_overflow_interrupt_enable();
	
	TIM1_overflow_262ms();
	TIM1_overflow_interrupt_enable();
	
	lcd_init(LCD_DISP_ON);
	
    // Initialize UART to asynchronous, 8N1, 9600
	uart_init(UART_BAUD_SELECT(9600, F_CPU));

    // Enables interrupts by setting the global interrupt mask
    sei();

    // Infinite loop
    while (1){}
    // Will never reach this
    return 0;
}

/* Interrupt service routines ----------------------------------------*/
/**
 * @brief Runs the state machine
 * @return None
 */
ISR(TIMER0_OVF_vect)
{
	state_machine();
}	
	
	


/* -------------------------------------------------------------------*/
/**
 * @brief Timer responsible for sending information to UART
 * @par Runs the whole time alongside the state machine, if the state of the machine is changed, timer registers that and sends the information to UART.
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
				wrong_tries = 0;
				uart_puts("DOOR_OPEN");
				break;
			case WRONG_CODE:
				wrong_tries++;
				uart_puts("WRONG_CODE");
				break;
			default:
				uart_puts("ERROR");
	   }
	   uart_puts("\n");
   }
   
   prev_state = current_state;
}

/**
* @brief Timer used for resetting after certain time
* @par Has 2 functions - when the code is being put in, limits the time for that to 20s, when the code is entered, waits for 5s before resetting the machine.
*/
ISR(TIMER2_OVF_vect)
{
	char str[] = "  ";
	if(current_state == DOOR_OPEN)
		PORTB|=(0x08);
	if(current_state == WRONG_CODE && counter2 == 250)
	{
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
			PORTB|=(0x04);
	}
	
	counter2++;
	if(counter2 == 1250)//1250*16ms=20s //1250*4ms = 5s
	{
		PORTB&=~(0x08);
		PORTB&=~(0x04);
		counter2 = 0;
		current_state = RESET;
		TIM2_overflow_interrupt_disable();
	}
	

}