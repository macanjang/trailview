//Some test code for gps data transmission and communication

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "gps.h"
#include "serialgps.h"
#include "lcd.h"
#include "fat32.h"
#include "sdcard.h"
#include "camera.h"

const char loading_map[] = {'-', '\\', '|', '/'};
void init_logtoggle(void);
#define CHECK_LOGTOGGLE() (PIND&0x80)

int main (int argc, char* argv[])
{
	struct gps_location gl1 , gl2;
	struct gps_displacement gd;
	struct fatwrite_t fout;
	char logging_state = 0;
	char flag_reset = 0;

	char in[128];
	int i;
	char c = 0;
	
	/* initialization sequences */
	gps_init_serial();
	lcd_init();
	camera_init();
	lcd_printf("sd card:\nconnecting");
	c = mmc_init();
	if (c) {
		lcd_printf("sd card: error\n");
		while (1) ;
	}
	init_partition(0);
	gps_disablesignals();
	init_logtoggle();

	/* main loop */

	// wait until receiving valid locations
	do {
		receive_str(in);
		gps_log_data(in , &gl1);
		lcd_printf("GPS Fixing %c\n", loading_map[(c++)&0x3]);
	} while (gl1.status != 'A');

	// got fix
	lcd_printf("Acquired Fix");

	/* main loop */
	while (1) {
		// read in gps data
		receive_str(in);
		if (flag_reset) {
			gps_log_data(in, &gl1);
			flag_reset = 0;
		}
		i = gps_log_data(in , &gl2);
	
		// check for fix
		if (gl2.status != 'A') {
			lcd_printf("Lost GPS Fix %c\n", loading_map[(c++)&0x3]);
			continue;
		}
	
		// compute displacement and display
		gps_calc_disp(&gl1, &gl2, &gd);
		lcd_printf("I: %d\xb2 F: %d\xb2\nMg: %dm Sp: %d",
			(int)gd.initial_bearing,
			(int)gd.final_bearing,
			(int)gd.magnitude,
			(int)(1.15*gl2.sog + 0.5));
			
		// log data
		if (logging_state) {
			if (!CHECK_LOGTOGGLE()) {
				// end log
				logging_state = 0;
				log_end(&fout);
				lcd_printf("log: finished\n");
			}
			// add to log
			log_add(&fout, &gl2, &gd);
		} else if (CHECK_LOGTOGGLE()) {
			// start logging
			lcd_printf("log: starting\n");
			logging_state = 1;
			flag_reset = 1;
			log_start(&fout);
		}
	}

	return 0;
}

/* initializes the log-toggle switch */
void init_logtoggle(void)
{
	DDRD &= ~0x80;
}

