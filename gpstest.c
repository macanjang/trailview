//Some test code for gps data transmission and communication

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "gps.h"
#include "serialgps.h"
#include "lcd.h"
#include "fat32.h"
#include "sdcard.h"

#define LOGNAME "log.kml"

int main (int argc, char* argv[])
{
	struct gps_location gl1 , gl2;
	struct gps_displacement gd;
	struct fatwrite_t fout;
	
	gps_init_serial();
	lcd_init();
	char rt = mmc_init();
	if (rt) {
		lcd_printf("sd card error\n");
		while (1) ;
	}
	
	init_partition(0);
	
	// disable unwanted GPS signals (set rate to 0)
	// $PSRF103,<msg>,<mode>,<rate>,<cksumEn>*CKSUM<CR><LF>
	send_gps("$PSRF103,00,00,00,01*");
	send_gps("$PSRF103,01,00,00,01*");
	send_gps("$PSRF103,02,00,00,01*");
	send_gps("$PSRF103,03,00,00,01*");
	send_gps("$PSRF103,04,00,01,01*");
	send_gps("$PSRF103,05,00,00,01*");
	
	// init write
	log_start(LOGNAME, &fout);

	char in[128];
	int i, j = 0;
	char c = 0;
	char loading_map[] = {'-', '\\', '|', '/'};
	
	while (1) {

		// wait until valid location
		do {
			receive_str(in);
			gps_log_data(in , &gl1);
			lcd_printf("GPS Fixing %c\n", loading_map[(c++)&0x3]);
		} while (gl1.status != 'A');
	
		// got fix
		lcd_printf("Acquired Fix");

		// compute displacement
		while (1) {
			receive_str(in);
			i = gps_log_data(in , &gl2);
		
			// check for fix
			if (gl2.status != 'A') {
				lcd_printf("Lost GPS Fix %c\n", loading_map[(c++)&0x3]);
				continue;
			}
		
			// compute and display
			gps_calc_disp(gl1.lat , gl1.lon , gl2.lat , gl2.lon , &gd);
			lcd_printf("I: %d\xb2 F: %d\xb2\nMg: %dm Sp: %d",
				(int)gd.initial_bearing,
				(int)gd.final_bearing,
				(int)gd.magnitude,
				(int)(1.15*gl2.sog));
				
			// log first 50 points
			log_add(&fout, &gl2);
			if (j++ >= 40) {
				log_end(&fout);
				lcd_printf("done logging\n");
				while (1) ;
			}
		}

	}
	
	return 0;
}
