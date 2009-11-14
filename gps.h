#ifndef _GPS_H
#define _GPS_H

#include <inttypes.h>

struct gps_location {
	float time;		//time of GPS data query hhmmss.sss
	char status;		//A=valid V=invalid
	float lat;		//lattitude ddmm.mmmm
	char ns;		//'N'=north 'S'=south
	float lon;		//longintude dddmm.mmmm
	char ew;		//'E'=east 'W'=west
	float sog;		//speed over ground knots
	float cog;		//course over ground degrees 
	int date;		//ddmmyy
};

struct gps_displacement {
	float magnitude;
	float initial_bearing;
	float final_bearing;
};

/* calculates a NMEA checksum */
char gps_calcchecksum(const char * s)

/*This function takes a data string
 *parses it and places it into the 
 *provided gps_location structure
 */
int gps_log_data(char * data, struct gps_location* loc);

/*This function takes the latitudes and longitudes
 *of two GPS locations and returns the displacement
 *using the Vincenty formula, accurate to .5 mm
 */
float gps_calc_disp(float lat1, float lon1, float lat2, float lon2);

/*This function converts a lattitude (ddmm.mmmm)
 *or longitude dddmm.mmmm to decimal degrees
 */
float dm_to_dd(float dm);	//lat ddmm.mmmm and lon dddmm.mmmm to decimal degrees 

#endif
