/*----------------------------------------------------------------------*/
/* Foolproof FatFs sample project for AVR              (C)ChaN, 2014    */
/*----------------------------------------------------------------------*/


#include <avr/io.h>	/* Device specific declarations */
#include <util/delay.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "minmea.h"
#include "ff.h"		
#include "lcd.h"

#define UART_BAUD 9600
#define __UBRR (((F_CPU / (UART_BAUD * 16UL))) - 1)
#define GPS_BUFFOR_SIZE 80

char GPSBuffor[GPS_BUFFOR_SIZE] = {""};

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
	int i;
	
	for(i = 0; i < GPS_BUFFOR_SIZE - 1; i++)
	{
		GPSBuffor[i] = "";
	}
	
}

void getNmea()
{
	clearBuffor();
	char buffor = "";
	int i = 0;
	
	while(1)
	{
		buffor = USART_Receive();
		
		//0x24 = $
		if((buffor == (char)0x24 && GPSBuffor[0] == (char)0x24) || i >= GPS_BUFFOR_SIZE - 1 ) {
			clearBuffor();
			i = 0;		
			break;
		}
		
		if(buffor == (char)0x24 || GPSBuffor[0] == (char)0x24) {
			
			GPSBuffor[i] = buffor;
			i = i+1;
		}
		
		//0x0D  cr
		//0x0A nl
		if(buffor == (char)0x0D || buffor == (char)0x0A) {
			
			//0x24 = $
			//0x47 = G
			//0x50 = P
			if(GPSBuffor[0] == (char)0x24 && GPSBuffor[1] == (char)0x47 && GPSBuffor[2] == (char)0x50)
			{
				break;
			} else {
				clearBuffor();
				i = 0;
			}
		}		
	}
		
	return;
}

void saveBufforToMmc()
{
	UINT bw1, bw2;
	f_mount(&FatFs, "", 0);												/* Give a work area to the default drive */
	if (f_open(&Fil, "GPS.txt", FA_WRITE | FA_OPEN_APPEND) == FR_OK) {	/* Otwiera plik do zapisu, dopisuje na koncu pliku */
		f_write(&Fil, GPSBuffor, GPS_BUFFOR_SIZE, &bw1);							    /* Zapisuje znaki do pliku */
		f_write(&Fil, "\r\n", GPS_BUFFOR_SIZE, &bw2);									/* Znaki powrotu karetki i nowej linii */
		f_close(&Fil);													/* Zamyka plik */
		if (bw1 == 80) {											    /* Lights green LED if data written well */
			//PORTC |= 0x02;												/* Set PB4 high */
			//DDRB |= 0x10; PORTB |= 0x10;	/* Set PB4 high */
		}
	}
}

void showBufforOnDisplay()
{
	char* lat;
	char* lon;
	char* snr;
		
	switch (minmea_sentence_id(GPSBuffor, false)) {
		case MINMEA_SENTENCE_RMC: {
			struct minmea_sentence_rmc frame;
			if (minmea_parse_rmc(&frame, GPSBuffor)) {
				sprintf(lat, "%d", frame.latitude.value);	
				sprintf(lon, "%d", frame.longitude.value);	
					
			}
			else {
						
			}
		} break;
				
		case MINMEA_SENTENCE_GLL: {
			struct minmea_sentence_gll frame;
			if (minmea_parse_gll(&frame, GPSBuffor)) {
				sprintf(lat, "%d", frame.latitude.value);
				sprintf(lon, "%d", frame.longitude.value);
						
			}
			else {
						
			}
		} break;

		case MINMEA_SENTENCE_GSV: {
			struct minmea_sentence_gsv frame;
			if (minmea_parse_gsv(&frame, GPSBuffor)) {
				sprintf(snr, "%d", frame.sats->snr);
			}
			else {

			}
		} break;
				
		case MINMEA_INVALID: {
					
		} break;
		//
		default: {
					
		} break;
	}
	  lcd_clrscr();
	  lcd_puts("Lat: "); 
	  lcd_gotoxy(5,0); 
	  lcd_puts(lat); 
	  lcd_gotoxy(0,2);          // set cursor to first column at line 3
	  lcd_puts("Lon: "); 
	  lcd_gotoxy(5,0); 
	  lcd_puts(lon); 
}

int main(void)
{
		
	int flag = 0;

	USART_Init();
    lcd_init(LCD_DISP_ON);    // init lcd and turn on
			
	while(1) 
	{

		getNmea();
		
		if(flag == 0) {
			saveBufforToMmc();
			clearBuffor();
			flag = 1;
			
		} else {
			//showBufforOnDisplay();
			flag = 0;
		}
		
	}
	
	return 0;
}

