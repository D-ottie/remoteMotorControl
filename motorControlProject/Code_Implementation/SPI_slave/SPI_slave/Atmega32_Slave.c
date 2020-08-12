/*
 * SPI_slave.c
 *
 * Created: 8/3/2020 9:50:44 AM
 * Author : Orina Dorothy
 */ 

#define F_CPU 8000000UL					/* Define CPU Frequency e.g. here its 8MHz */
#include <avr/io.h>						/* Include AVR std. library file */
#include <util/delay.h>					/* Include Delay header file */
#include <stdio.h>
#include <string.h>						/* Include string header file */
#include <avr/interrupt.h>
#include "LCD_16X2_H_FILE.h"			/* Include LCD header file */
#include "SPI_Slave_H_file.h"			/* Include SPI slave header file */
#define snd_len 4
#define msg_len 8
#define cur_encode PORTD
volatile uint8_t start = 0;
volatile uint8_t stop = 0;
int count, cur_count, prev_count;
double RPM;
char buffer2[10];
char character;
char buffer[10];
char c = 0;

/* Utility functions to clear the values in buffer variables */
void clear_read(char *buff){
	for (int i = 0; i < msg_len; i++){
		buff[i]= 0;
	}
}

/*Interrupt setup function*/
void interrupt_setup(){
	DDRB = 0xFF; // Make PORTB as output Port
	DDRB &= ~(1<<PB2); // Make INT2 pin as Input
	DDRB |= (1<<PB3); // Make OC0 pin as Output
	DDRD &= ~((1<<PD2)|(1<<PD3)); // Make INT0 pin as Input
	GICR = (1<<INT2)|(1<<INT1)|(1<<INT0); // Enable INT0, INT2
	MCUCR = (1<<ISC00)|(1<<ISC10); // Trigger INT0,INT1 on Logic change trigger
	MCUCSR = (1<<ISC2);// Trigger INT2 on Rising Edge triggered
	sei(); // Enable Global Interrupt */
}
	/* Interrupt ISR functions */
ISR(INT0_vect){
		cur_encode = PIND & ((1<<PD2)|(1<<PD3));
		cur_encode = (cur_encode>>2);
		// From the encoder value chart, when Channel A changes logic states we look
		// at the value of the interrupt pins. If they are either 0b 11 or 0b 00,
		// the motor is moving CW and we decrease the count
		if(cur_encode == 0x03 || cur_encode == 0x00){
			count-=1;
		}
		// If they are either 0b 10 or 0b 01, the motor is moving CCW and we increase the count
		else if(cur_encode == 0x02 || cur_encode == 0x01){
			count+=1 ;
		}
	}
ISR(INT1_vect){
		cur_encode = PIND & ((1<<PD2)|(1<<PD3)); // Obtain the reading from the PIND2 and PIND3
		cur_encode = (cur_encode>>2);
		// From the encoder value chart, when Channel B changes logic states we look
		// at the value of the interrupt pins. If they are either 0b 01 or 0b 10,
		// the motor is moving CW and we decrease the count
		if(cur_encode == 0x01 || cur_encode == 0x02){
			count-=1;
		}
		// If they are either 0b 11 or 0b 00, the motor is moving CCW and we increase the count
		else if(cur_encode == 0x03 || cur_encode == 0x00){
			count+=1;
		}
	}
ISR(INT2_vect){
	/*Emergency stop interrupt*/
	LCD_Cmd(0x01);
	LCD_String_xy(0,3,"EMERGENCY!");
	LCD_String_xy(1,5,"STOP!");
	//Set motor speed to zero and initialize motor brake
	//Toggle stop status for all loops
	OCR0 = 0; 
	PORTB = 0x03;
	stop = ~stop;
	_delay_ms(50); // Software de-bouncing control delay
}

void pwm_setup(){
		/* GPIO setup */
		DDRB |= (1<<PB3); // Make OC0 pin as Output
		/* PWM setup */
		TCCR0 = (1<<WGM00)|(1<<WGM01)|(1<<COM01)|(1<<CS02); // Set Fast PWM with Fosc/256 Timer0 clock
	}
	
	/*Timer setup function*/
void timer_setup(){
		TIMSK |= (1<<TOIE1); // Activate the timer overflow interrupt
		TCCR1B = (1<<CS11)|(1<<CS10); // Set the timer prescalar to 64
		TCNT1 = 3036; // Load the countdown value for 500ms
	}
	
	/*Timer overflow ISR*/
ISR(TIMER1_OVF_vect){
		cur_count = count;
		RPM = (cur_count-prev_count)*120/(96); // Calculate the RPMs
		prev_count = cur_count;
		TCNT1 = 3036;
	}

/*Motor speed setup function*/
void speed_ctrl(char motor_speed){
	char buffer1[4];
	int std_speed;
	//standard speed setup of maximum 70RPM = 255
	std_speed = motor_speed*3.65; //Convert input value to a fraction of 255	
	PORTB = 0x01;
	//If condition to ensure the motor RPM and OCR0 is above 0 and below maximum
	if (std_speed > 255)
	{
		std_speed = 255;
		motor_speed = 70;
	} else if (std_speed<10)
	{
		std_speed = 10;
		motor_speed = 10;
		}else{
		std_speed = std_speed;
		motor_speed = motor_speed;
	}
	OCR0 = std_speed; //map received values to OCR0
	
	//display the RPM value
	sprintf(buffer,"%d",motor_speed);
	LCD_String_xy(0,10,buffer);
	//Track the RPM and display
	start = ~start;
	//Loop to monitor speed status or motor
	while (start)
	{
		if (stop != 0) break; //Toggle status change causes loop to exit
		LCD_Cmd(0xC6);
		dtostrf(RPM,2,2,buffer1); //convert double to string
		LCD_String(buffer1);
		LCD_String(" ");
		_delay_ms(500);
	}
}

/*Setup function for number of revolutions*/
void pos_ctrl(char rev){
	char buffer1[10];
	int n_Rev;
	OCR0 = 218; // OCRO value for 60 revolutions per second i.e. 1 revolution per second
	//minimum value set is 5 and maximum is 40 revolutions
	if (rev<5)
	{
		n_Rev = 5;
	}else if (rev > 40)
	{
		n_Rev = 40;
		}else{
		n_Rev = rev;
	}
	//display the number of revolutions to be done
	sprintf(buffer,"%d",n_Rev); //integer to string conversion
	LCD_String_xy(0,7,buffer);
	//while loop condition for 1 second per loop
	PORTB = 0x01;
	while(c != n_Rev){
		if (stop != 0) break; //Toggle status change causes loop to exit
		c++;
		sprintf(buffer1, "%d",c); //convert nth_loop to string
		LCD_Cmd(0xc6); //LCD Position setup
		LCD_String(buffer1); //display the nth revolution
		_delay_ms(1000); // Delay loop for 1 second
	}
	PORTB = 0x03; // Initiate motor brakes after LOOP is done
}

/*Select loop Mode i.e. speed/position*/
void Mode_Set(char comp1, char comp2){ 
	char mode_val[4];
	int mode_sel;
	mode_sel = comp2%comp1; //Modulus returns either 0 or 1 to determine mode
	switch(mode_sel) //returned value passed to switch case
	{
		case 0: // Switch case for speed control selection
		//LCD setup function for speed control
			LCD_String_xy(0,0,"M-SPEED->");
			sprintf(mode_val, "%d",comp1);
			LCD_String_xy(0,13,"RPM");
		//value passed to control setup function
			LCD_String_xy(1,0,"Cur:");
			LCD_String_xy(1,12,"RPM");
			speed_ctrl(comp1);
			break;
		case 1: //Switch case for position control setup
		//LCD setup function for position control
			LCD_String_xy(0,0,"M-POS->");
			sprintf(mode_val, "%d",comp1);
			LCD_String_xy(0,10,"REV");
		//value passed to control setup function
			LCD_String_xy(1,0,"Cur:");
			LCD_String_xy(1,10,"REV");
			pos_ctrl(comp1);
			break;
	}
}

/*Setup function for received data*/
void received(){
	char message1, message2;
	int k = 0;
	//Loop to read the data sent in form of characters and assigns to new char declarations
	do 
	{
		character = SPI_Receive();
		if(k==0)
		{
			message1 = character;
		}
		else
		{
			message2 = character;
		}
		k++;
	} while (k <2); // Loop stop condition
	//Received characters passed to mode setup function
	Mode_Set(message1,message2);
}

int main(void)
{
	LCD_Init();
	SPI_Init();
	pwm_setup();
	interrupt_setup();
	timer_setup();
	
	while (1)
	{	
		received();
	}
}