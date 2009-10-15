//////////////////////////////////
//Zach Norris			//
//This file provides to code to	//
//initialize the GPS chip and	//
//to communicate with the GPS	//
//chip				//
//////////////////////////////////

#include <stdio.h>
#include <avr/io.h>
#include <avr/iom8.h>

#define BAUD 4800
#define MSGLENGTH 70

void init_serial(void);
void print(char * string);
void tx_str(char data);
char rx_byte(void);
char* rx_str(void);

int main(int argc, char* argv[])
{
	char data[MSGLENGTH];
	
	tx_str("$PSRF103,4,0,1,0");		//set up GPS to send in RMC format every 1 second
	data = 
}

void init_serial(void)
{
	UBRRH=0;
	UBRRL=12; 				// 4800 BAUD FOR 1MHZ SYSTEM CLOCK
	UCSRC= (1<<URSEL)|(1<<USBS)|(3<<UCSZ0) ;// 8 BIT NO PARITY 2 STOP
	UCSRB=(1<<RXEN)|(1<<TXEN)  ; 		//ENABLE TX AND RX ALSO 8 BIT
}

void tx_byte(char data)
{
	while((UCSRA&(1<<UDRE)) == 0);		//wait if byte is being transmitted
	UDR = data;				//send byte
}

void tx_str(char * string)
{
	while(*string != '\0') {
		tx_byte(*string);
		string++;
	}
}

char rx_byte(void)
{
	// Wait for data to be received 
	while ( !(UCSRA & (1<<RXC)) );
	// Get and return received data from buffer
	return UDR;
}

char* rx_str(void)
{
	char str[MSGLENGTH];
	int i = 0;
	
	do{
		str[i] = rx_byte();
		i++;
	} while(str[i - 1] != '\0');
}
