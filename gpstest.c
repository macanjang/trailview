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
	
	// disable unwanted GPS signals (set rate to 0)
	// $PSRF103,<msg>,<mode>,<rate>,<cksumEn>*CKSUM<CR><LF>
	send_gps("$PSRF103,00,00,00,01*");
	send_gps("$PSRF103,01,00,00,01*");
	send_gps("$PSRF103,02,00,00,01*");
	send_gps("$PSRF103,03,00,00,01*");
	send_gps("$PSRF103,04,00,01,01*");
	send_gps("$PSRF103,05,00,00,01*");
	
	char in[128];
	int i;
	char c = 0;
	char loading_map[] = {'-', '\\', '|', '/'};
	
	while (1) {

		// wait until valid location
		do {
			receive_str(in);
			if (gps_log_data(in , &gl1))
				lcd_printf("Bad GPS Data");
			else lcd_printf("GPS Fixing %c", loading_map[(c++)&0x3]);
		} while (gl1.status != 'A');
	
		// got fix
		lcd_printf("Acquired Fix");

		// compute displacement
		while (1) {
			receive_str(in);
			i = gps_log_data(in , &gl2);
		
			// check for fix
			if (gl2.status != 'A') break;
		
			// compute and display
			gps_calc_disp(gl1.lat , gl1.lon , gl2.lat , gl2.lon , &gd);
			lcd_printf("IB: %d\xb2 FB: %d\xb2\nMg: %dm Sp: %d",
				(int)gd.initial_bearing,
				(int)gd.final_bearing,
				(int)gd.magnitude,
				(int)(1.15*gl2.sog));
		}

	}
	
	return 0;
}
