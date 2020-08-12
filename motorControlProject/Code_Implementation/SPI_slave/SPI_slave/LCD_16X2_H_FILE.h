/*
 * LCD_16X2_H_FILE.h
 *
 * Created: 8/3/2020 4:23:07 PM
 *  Author: Orina Dorothy
 */ 


#ifndef LCD_16X2_H_FILE_H_
#define LCD_16X2_H_FILE_H_

#define F_CPU 8000000UL					/* Define CPU Frequency e.g. here its 8MHz */
#include <avr/io.h>						/* Include AVR std. library file */
#include <util/delay.h>					/* Include Delay header file */


#define LCD_Dir DDRC
#define LCD_Port PORTC
#define RS PC0
#define EN PC1

/*Function Declarations*/
void LCD_Cmd(unsigned char cmd);
void LCD_Char(unsigned char char_data);
void LCD_Init(void);
void LCD_Clear(void);
void LCD_String(char *str);
void LCD_String_xy(char row, char pos, char *str);



#endif /* LCD_16X2_H_FILE_H_ */