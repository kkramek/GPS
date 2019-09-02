/*----------------------------------------------------------------------*/
/* Foolproof FatFs sample project for AVR              (C)ChaN, 2014    */
/*----------------------------------------------------------------------*/


#include <avr/io.h>	/* Device specific declarations */
#include <util/delay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

#include "ff.h"		
#include "lcd.h"

#define UART_BAUD 9600
#define __UBRR (((F_CPU / (UART_BAUD * 16UL))) - 1)
#define GPS_BUFFOR_SIZE 80

char GPSBuffor[GPS_BUFFOR_SIZE] = {""};
char lat[15] = {""};
char lon[15] = {""};
char latDir[2] = {""};
char lonDir[2] = {""};
char speed[15] = {""};
float latf = 0;
float lonf = 0;
	
int32_t snr;

FATFS FatFs;		/* FatFs work area needed for each volume */
FIL Fil;			/* File object needed for each open file */

void USART_Init(void) 
{
	UBRR0H = (uint8_t) (__UBRR >> 8);
	UBRR0L = (uint8_t) (__UBRR);
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	UCSR0C = (1<<UCSZ00) | (1<<UCSZ01); // Set frame: 8data, 1 stop
}

char USART_Receive(void) 
{	
	while (!(UCSR0A & (1<<RXC0)));
	return UDR0;
}

void clearBuffor()
{
	int i, j;
	
	for(i = 0; i < GPS_BUFFOR_SIZE-1; ++i)
	{
		GPSBuffor[i] = "";
	}
	
	latf = 0;
	lonf = 0;
}

int getNmea()
{
	clearBuffor();
	char buffor = "";
	int nmeaReady = 0;
	int i = 0;
	
	while(1)
	{
		buffor = USART_Receive();
			
		//0x0D  cr
		//0x0A nl
		if(buffor == (char)0x0D || buffor == (char)0x0A || i > GPS_BUFFOR_SIZE - 1 ) {
			
			//0x24 = $
			//0x47 = G
			//0x50 = P
			if(GPSBuffor[0] == (char)0x24 && GPSBuffor[1] == (char)0x47 && GPSBuffor[2] == (char)0x50)
			{
				nmeaReady = 1;
				break;
			} else {
				clearBuffor();	
				break;
			}
		}
		
		//0x24 = $
		if(buffor == (char)0x24 && (GPSBuffor[0] == (char)0x24 || GPSBuffor[1] == (char)0x24)) {
			clearBuffor();	
			break;
		}
		
		//0x24 = $
		if(buffor == (char)0x24 || GPSBuffor[0] == (char)0x24) {
					
			GPSBuffor[i] = buffor;
			i = i+1;
		}
	}
	
	return nmeaReady;
}

void saveBufforToMmc()
{
	UINT bw1, bw2;
	f_mount(&FatFs, "", 0);												
	if (f_open(&Fil, "GPS.txt", FA_WRITE | FA_OPEN_APPEND) == FR_OK) {	
		f_write(&Fil, GPSBuffor, GPS_BUFFOR_SIZE, &bw1);							    
		f_write(&Fil, "\r\n", 2, &bw2);									
		f_close(&Fil);													
		if (bw1 == 80) {											    

		}
	}
}

void showOnDisplay()
{
	char latStr[30];
	char lonStr[30];
	char speedStr[30];
	
	latf = atof(lat);
	lonf = atof(lon);
	float speedkmh = atof(speed);
	int speedkmhint = speedkmh*3.6;	
	speedkmhint = (speedkmhint < 10) ? 0 : speedkmhint;
	
	if(latf>0 && lonf>0) {

		int latdegrees = latf / 100;
		latf = latf - (latdegrees*100);
		float latminutes = latf / 60;
		unsigned long latmin = (unsigned long)(latminutes * 100000);

		int londegrees = lonf / 100;
		lonf = lonf - (londegrees*100);
		float lonminutes = lonf / 60;
		unsigned long lonmin = (unsigned long)(lonminutes * 1000000);
		
		sprintf(latStr, "Lat: %d.%ld %s", latdegrees, latmin, latDir);
		sprintf(lonStr, "Lon: %d.%ld %s", londegrees, lonmin, lonDir);
		sprintf(speedStr, "Speed: %d", speedkmhint);
		
		lcd_clrscr();
		lcd_puts(latStr);
		lcd_gotoxy(0,2);
		lcd_puts(lonStr);
		lcd_gotoxy(0,4);
		lcd_puts(speedStr);

		_delay_ms(5000);
	}
}

void parseBuffor()
{
	char buffor[GPS_BUFFOR_SIZE];	
	strcpy(buffor, GPSBuffor);

	char* Message_ID = strtok(buffor,",");	
	
	if(strcmp(Message_ID, "$GPRMC") == 0) {
		// $GPRMC,081836,A,3751.65,S,14507.36,E,000.0,360.0,130998,011.3,E*62
		char* Time = strtok(NULL,",");
		char* Data_Valid = strtok(NULL,",");
		
		strcpy(lat, strtok(NULL,","));
		strcpy(latDir, strtok(NULL,","));
		strcpy(lon, strtok(NULL,","));
		strcpy(lonDir, strtok(NULL,","));
		strcpy(speed, strtok(NULL,","));
		char* COG = strtok(NULL,",");
		char* Date = strtok(NULL,",");
		char* Magnetic_Variation = strtok(NULL,",");
		char* M_E_W = strtok(NULL,",");
		char* Positioning_Mode = strtok(NULL,",");

		showOnDisplay();
	
	} 
		
}

int main(void)
{
		
	int flag = 0;

	USART_Init();
    lcd_init(LCD_DISP_ON);   
	
	lcd_clrscr();
	lcd_gotoxy(5,2);
	lcd_puts("Starting...");	
			
	while(1) 
	{
		flag = getNmea();  	
		
		if(flag == 1) {
			saveBufforToMmc();			
			parseBuffor();
		}
		clearBuffor();	
	}
	
	return 0;
}

