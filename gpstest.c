//Some test code for gps data transmission and communication

#include <stdio.h>
#include "gps.h"
#include "serialgps.h"
#include "lcd.h"

int main (int argc, char* argv[])
{
	struct gps_location gl1 , gl2;
	struct gps_displacement gd;
	
	gps_init_serial();
	lcd_init();
	
	/*
	int i = 0, l = 0;
	while (1) {
		lcd_wdata(receive_char());
		if (i++ >= 16) lcd_go_line_clear(l=!l), i=0;
	}
	*/
	send_gps("$PSRF103,00,00,00,01*");	//$PSRF103,<msg>,<mode>,<rate>,<cksumEn>*CKSUM<CR><LF>
	send_gps("$PSRF103,01,00,00,01*");
	send_gps("$PSRF103,02,00,00,01*");
	send_gps("$PSRF103,03,00,00,01*");
	send_gps("$PSRF103,04,00,01,01*");
	send_gps("$PSRF103,05,00,00,01*");
	
	char in[80];
	int i;
	
	receive_str(in);
	if (gps_log_data(in , &gl1)) {
		lcd_printf("Error: Wrong\nGPS Data");
	} else {
		while (1) {
			lcd_printf("while loop");
			receive_str(in);
			lcd_printf("rx string");
			i = gps_log_data(in , &gl2);
			lcd_printf("gpslog: %d",i);
			gps_calc_disp(gl1.lat , gl1.lon , gl2.lat , gl2.lon , &gd);
			lcd_printf("Disp: %d \n IB: %d  FB: %d", gd.magnitude , gd.initial_bearing , gd.final_bearing);
		}
	}
	return 0;
}
