#ifndef _GPSCOM_H
#define _GPSCOM_H

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

int logdata(char * data, struct gps_loc* loc);
uint32_t gps_calc_disp(struct gps_location * gps0, struct gps_location * gps1);
float dm_to_dd(float dm);	//lat ddmm.mmmm and lon dddmm.mmmm to decimal degrees 

#endif
