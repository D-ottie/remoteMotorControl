/*
 * SPI.c
 *
 * Created: 8/3/2020 9:44:02 AM
 * Author : Orina Dorothy
 */ 

#define F_CPU 8000000UL					/* Define CPU Frequency e.g. here its 8MHz */
#include <avr/io.h>						/* Include AVR std. library file */
#include <util/delay.h>					/* Include Delay header file */
#include <stdio.h>
#include <string.h>
#include <avr/interrupt.h>
#include "LCD_16X2_H_file.h"			/* Include LCD header file */
#include "SPI_Master_H_file.h"			/* Include SPI master header file */

#define msg_len 8
#define snd_len 2
#define TRUE 1
#define FALSE 0

char read_msg[msg_len];
char send_msg[snd_len];
int keycount = -1;
int send_cmd = FALSE;
int get = 0;
volatile uint8_t mode = 0;


/* Keypad array holding the keys in a grid arrangement*/
unsigned char keypad[4][4] = {{'7','8','9','/'},
{'4','5','6','*'},
{'1','2','3','-'},
{'C','0','=','+'}};
	
/* Function that checks the key that has been pressed on the keypad*/
unsigned char check_Keypad(char input_val){
	int row = input_val/4;
	int col = input_val%4;
	if((input_val>= 0) & (input_val<16)) return (keypad[row][col]);
	else return 0;
}

/* Utility functions to clear the values in buffer variables */
void clear_read(char *buff){
	for (int i = 0; i < msg_len; i++){
		buff[i]= 0;
	}
}
void clear_send(char *buff){
	for (int i = 0; i < snd_len; i++){
		buff[i]= 0;
	}
}

/*ISR function: run whenever there is a new key press from the MMC74C922*/
ISR(INT0_vect){
	// Read the value from the keypad connected pins
	char value = PINB & (0x0F);
	unsigned char keycheck = check_Keypad(value);
	// If the key pressed is C, send a message to MCU2
	
	if (keycheck == '+'){
		mode = ~mode; // Toggle mode
		LCD_Clear();
		_delay_ms(50); // Software de-bouncing control delay
	}else if (keycheck == 'C'){
		send_cmd = ~send_cmd;
		keycount = 0;
	}else if (keycheck == '*'){
		PORTA |= (1<<PA7);
		_delay_ms(100);
		PORTA &= ~(1<<PA7);
	}else{
		if(keycount < 0){
			keycount++;
			}else if((keycount>= 0) && (keycount < snd_len)){
			send_msg[keycount] = keycheck;
			keycount++;
			if(keycount == snd_len) keycount = 0;
		}
	}
}
/* Setup function for the Keypad */
void keypad_Init(){
	DDRD |= (0<<PD2); /* PORTD as input */
	//PORTD |= (1<<PD2); /* Activate pull up resistor high */
	/* Interrupt setup */
	GICR = 1<<INT0; /* Enable INT0*/
	MCUCR = 1<<ISC01 | 1<<ISC00; /* Trigger INT0 on rising edge */
	sei(); /* Enable Global Interrupt */
}

/*Function to select the mode to display*/
//Passed value from main function is used as switch value
void mode_sel(int case_val){
	int buffer;
	int count;
	switch (case_val)
	{
		//Position display and sending of values
	case 0:
	//Display setup
		LCD_String_xy(0,0, "Mode:M-Pos");
		LCD_String_xy(1,0,"Pos:");
		LCD_String(send_msg); //Displays the pressed characters
		LCD_String_xy(1,11,"REV");
	//Checks if the send_cmd is toggled in order to send
		if (send_cmd)
		{
			buffer = atoi(send_msg);  //Conversion of message from string to integer
		//increase the converted value by one
		//Send both the converted value and increased value in order to recognize the mode sent to slave
			count = buffer+1; 
		//For loop for sending both values
			for (get = 0;get<2;get++)
			{
				if (get == 0)
				{
					SPI_Write(buffer);
				} if (get == 0)
				{
					SPI_Write(count);
				}
			}
			send_cmd = FALSE; //Return send command to initial state
			clear_send(send_msg); //clear message buffer
		}
		break;
	case 1:
	//Display setup
		LCD_String_xy(0,0, "Mode:M-Speed");
		LCD_String_xy(1,0,"Speed:");
		LCD_String(send_msg); //Displays the pressed characters
		LCD_String_xy(1,11,"RPM");
		if (send_cmd)
		{
			buffer = atoi(send_msg); //String to integer conversion
		//Send the converted value twice in order to recognize the mode sent to slave
			for (get = 0;get<2;get++)
			{
				SPI_Write(buffer);
			}
			send_cmd = FALSE; //Return send command to initial state
			clear_send(send_msg); //clear message buffer
		}
		break;
	}
}


int main(void)
{
	LCD_Init();
	SPI_Init();
	keypad_Init();
	SS_Enable;
	/*Emergency stop pin direction*/
	DDRA = 1<<PA7;
	while (1)
	{
		// Checks for the state of the mode status whether 0 or 1
		//Passes the value 0 or 1 to the mode selection function
		if (mode !=0) {		
			mode_sel(0); 
		}
		else{
			mode_sel(1);
		}			
	}
	return 0;
}
	

