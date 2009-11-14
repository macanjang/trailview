//Some test code for gps data transmission and communication

#include <stdio.h>
#include "gps.h"
#include "serialgps.h"
#include "lcd.h"

int main (int argc, char* argv[])
{
	struct gps_location gl1 , gl2;
	struct gps_displacement gd;
	init_serial();
	send_str("$PSRF103,00,00,00,00");
	char in[80];
	
	receive_str(in);
	if (gps_log_data(in , &gl1)) break;
	else {
		while (1) {
			receive_str(in);
			gps_log_data(in , &gl2);
			gps_calc_disp(gl1.lat , gl1.lon , gl2.lat , gl2.lon , &gd);
			lcd_printf("Disp: %d \n IB: %d  FB: %d", gd.magnitude , gd.initial_bearing , gd.final_bearing);
		}
	}
	printf("%s",in);
	return 0;
}
