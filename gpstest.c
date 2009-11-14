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
	
	char in[128];
	int i;
	
	while (1) {
	
		receive_str(in);
	
		if (gps_log_data(in , &gl1)) {
			lcd_printf("Error: Wrong\nGPS Data");
		} else {
			while (1) {
				receive_str(in);
				i = gps_log_data(in , &gl2);
				lcd_printf("lat: %d\nlon: %d", (int)gl2.lat, (int)gl2.lon);
				gps_calc_disp(gl1.lat , gl1.lon , gl2.lat , gl2.lon , &gd);
				//lcd_printf("IB: %d  FB: %d\nDisp: %d It: %d", (int)gd.initial_bearing , (int)gd.final_bearing , (int)gd.magnitude , (int)gd.iterations);
			}
		}
	
	}
	
	return 0;
}
