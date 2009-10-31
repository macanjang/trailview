#ifndef _GPSCOM_H
#define _GPSCOM_H

struct gps_loc {
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

int logdata(char * data, struct gps_loc* loc);

#endif
