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
#include <string.h>

#define BAUD 4800
#define MSGLENGTH 70
#define PLENGTH 10

void init_serial(void);
void print(char * string);
void tx_str(char data);
char rx_byte(void);
char* rx_str(void);
int logdata(char * data, struct gps_loc* loc);

struct gps_loc {
	float time;
	char status;
	float lat;
	char ns;
	float lon;
	char ew;
	float sog;
	float cog;
	int date;
};

int main(int argc, char* argv[])
{
	char data[MSGLENGTH];
	struct gps_loc gps;
	
	tx_str("$PSRF103,4,0,1,0");		//set up GPS to send in RMC format every 1 second
	while(1) {
		data = rx_str();		//get some data
		logdata(data,&loc);		//log it
	}
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
	while(*string != '\0') {		//transmit the string byte by byte
		tx_byte(*string);
		string++;
	}
}

char rx_byte(void)
{
	while ( !(UCSRA & (1<<RXC)) );		// Wait for data to be received
	return UDR;				// Get and return received data from buffer
	
}

char* rx_str(void)
{
	char str[MSGLENGTH];
	int i = 0;
	
	do{					//receive the output byte by byte
		str[i] = rx_byte();
		i++;
	} while (str[i - 1] != '\0');
	
	return str;
}

int logdata(char * data, struct gps_loc* loc)
{
	int i = 0;
	int j;
	int field = 0;
	char temp[PLENGTH];
	
	i = 0;
	
	while (data[i] != '\0') {		//while still in the string
		while (data[i] != ',') {	//field by field in a comma separated file
			temp[j] = data[i];	//copy to temp
			i++;
			j++;
		}
		j = 0;
		switch (field) {
			case 0:			//error checking, return -1 if wrong data
				if (strcmp("$GPRMC",temp)) return -1;
				break;
			case 1:			//then fill in all the fields of the gps location
				loc.time = atof(temp);
				break;
			case 2:
				loc.status = *temp;
				break;
			case 3:
				loc.lat = atof(temp);
				break;
			case 4:
				loc.ns = *temp;
				break;
			case 5:
				loc.lon = atof(temp);
				break;
			case 6:
				loc.ew = *temp;
				break;
			case 7:
				loc.sog = atof(temp);
				break;
			case 8:
				loc.cog = atof(temp);
				break;
			case 9:
				loc.date = atoi(temp);
				break;
		}
		field++;
	}
	return 0;
}
