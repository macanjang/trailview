#ifndef _GPS_H
#define _GPS_H

#include <inttypes.h>

struct gps_location {
	double time;		//time of GPS data query hhmmss.sss
	char status;		//A=valid V=invalid
	double lat;		//lattitude ddmm.mmmm
	char ns;		//'N'=north 'S'=south
	double lon;		//longintude dddmm.mmmm
	char ew;		//'E'=east 'W'=west
	double sog;		//speed over ground knots
	double cog;		//course over ground degrees 
	int date;		//ddmmyy
};

struct gps_displacement {
	double magnitude;
	double initial_bearing;
	double final_bearing;
	char iterations;
};

/* calculates a NMEA checksum */
char gps_calcchecksum(const char * s);

/*This function takes a data string
 *parses it and places it into the 
 *provided gps_location structure
 */
int gps_log_data(char * data, struct gps_location* loc);

/*This function takes the latitudes and longitudes
 *of two GPS locations and returns the displacement
 *using the Vincenty formula, accurate to .5 mm
 */
int gps_calc_disp(double lat1, double lon1, double lat2, double lon2, struct gps_displacement * gd);

/*This function converts a lattitude (ddmm.mmmm)
 *or longitude dddmm.mmmm to decimal degrees
 */
double dm_to_dd(double dm);	//lat ddmm.mmmm and lon dddmm.mmmm to decimal degrees 

#endif
