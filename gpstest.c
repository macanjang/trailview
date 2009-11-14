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
	
	
	lcd_printf("st %d\nmt",4);
	/*
	int i = 0, l = 0;
	while (1) {
		lcd_wdata(receive_char());
		if (i++ >= 16) lcd_go_line_clear(l=!l), i=0;
	}
	*/
	send_gps("$PSRF103,00,00,00,00\r\n");	//$PSRF103,<msg>,<mode>,<rate>,<cksumEn>*CKSUM<CR><LF>
	while(1);
	
	char in[80];
	
	receive_str(in);
	if (gps_log_data(in , &gl1)) {
		lcd_printf("Error 2 many\n");
		lcd_printf(in);
	} else {
		while (1) {
			receive_str(in);
			gps_log_data(in , &gl2);
			gps_calc_disp(gl1.lat , gl1.lon , gl2.lat , gl2.lon , &gd);
			lcd_printf("Disp: %d \n IB: %d  FB: %d", gd.magnitude , gd.initial_bearing , gd.final_bearing);
		}
	}
	return 0;
}
