//////////////////////////////////
//Zach Norris			//
//This file provides to code to	//
//initialize the GPS chip and	//
//to communicate with the GPS	//
//chip				//
//////////////////////////////////

#include <stdio.h>
#include <string.h>
#include "gps.h"
#include <math.h>
#include <stdlib.h>

#define PLENGTH 10
#define sq(a) ((a)*(a))

int gps_log_data(char * data, struct gps_location * loc)
{
	int i = 0;
	int j = 0;
	int field = 0;
	char temp[PLENGTH];
	
	while (data[i] != '\0') {		//while still in the string
		while (data[i] != ',') {	//field by field in a comma separated file
			temp[j] = data[i];	//copy to temp
			i++;
			j++;
		}
		j = 0;
		switch (field) {
			case 0:			//error checking, return -1 if wrong data
				if (strcmp("$GPRMC",temp)) return -1;
				break;
			case 1:			//then fill in all the fields of the gps location
				loc->time = atof(temp);
				break;
			case 2:
				loc->status = *temp;
				break;
			case 3:
				loc->lat = atof(temp);
				break;
			case 4:
				loc->ns = *temp;
				break;
			case 5:
				loc->lon = atof(temp);
				break;
			case 6:
				loc->ew = *temp;
				break;
			case 7:
				loc->sog = atof(temp);
				break;
			case 8:
				loc->cog = atof(temp);
				break;
			case 9:
				loc->date = atoi(temp);
				break;
		}
		field++;
	}
	return 0;
}

int gps_calc_disp(float lat1, float lon1, float lat2, float lon2, struct gps_displacement * gd)
{
	lat1 = dm_to_dd(lat1);
	lon1 = dm_to_dd(lon1);
	lat2 = dm_to_dd(lat2);
	lon2 = dm_to_dd(lon2);
	
	float a = 6378137;
	float b = 6356752.3142;
	float f = 1/298.257223563;		//flatening of geoid;
	float L = (lon2 - lon1) * M_PI / 180;	//difference in longitude in radians
	float s1 = sin(atan((1 - f) * tan(lat1 * M_PI / 180)));	//sin of reduced lat1
	float c1 = cos(atan((1 - f) * tan(lat1 * M_PI / 180)));	//cos of reduced lon1
	float s2 = sin(atan((1 - f) * tan(lat2 * M_PI / 180)));	//sin of reduced lat2
	float c2 = cos(atan((1 - f) * tan(lat2 * M_PI / 180)));	//cos of reduced lon2
	
	float lambda = L;
	float ltemp, sigma, ss /*sin(sigma)*/, cs /*cos(sigma)*/, sa /*sin(alpha), ca cos(alpha)*/, csqa /*cos^2(alpha(*/, c2sm /*cos(2sigma sub m)*/, C, usq /*u^2*/, A, B, ds /*delta(sigma)*/;
	int8_t lim = 0;
	
	do {
		ss = sqrt(sq(c2 * sin(lambda)) + sq(c1 * s2 - s1 * c2 * cos(lambda)));	//sin(sigma)
		cs = s1 * s2 + c1 * c2 * cos(lambda);			//cos(sigma)
		sigma = atan2(ss , cs);
		sa = c1 * c2 * sin(lambda) / ss;			//sin(alpha)
		csqa = 1 - sq(sa);					//cos_squared(alpha)
		c2sm = cs - 2 * s1 * s2 / csqa;				//cos(2sigma_m)
		C = f / 16 * csqa * (4 + f * (4 - 3 * csqa));		//intermediate value
		ltemp = lambda;						//store old value of lambda
		lambda = L + (1 - C) * f * sa * (sigma + C * ss * (c2sm + C * cs * (-1 + 2 * sq(c2sm))));	//get new value
	} while (abs(lambda - ltemp) > 1e-12 && lim++ < 50);		//check for accuracy or too many iterations
	
	if (lim == 50) return -1;
	
	usq = csqa * (sq(a) - sq(b)) / sq(b);				//u squared
	A = 1 + usq / 16384 * (4096 + usq * (-768 + usq * (320 - 175 * usq)));	//intermediate value
	B = usq / 1024 * (256 + usq * (-128 + usq * (74 - 47 * usq)));	//intermediate value
	ds = B * ss * (c2sm + B / 4 * (cs * (-1 + 2 * sq(c2sm)) - B / 6 * c2sm * (-3 + 4 * sq(ss)) * (-3 + 4 * sq(c2sm)))); //delta sigma
	
	gd->magnitude = b * A * (sigma - ds);			//displacement magnitude
	gd->initial_bearing = atan2(c2 * sin(lambda) , c1 * s2 - s1 * c2 * cos(lambda));	//displacement initial bearing	
	gd->final_bearing = atan2(c1 * sin(lambda) , -s1 * c2 + c1 * s2 * cos(lambda));	//displacement final bearing
	
	return 0;
}

float dm_to_dd(float dm)
{
	float dd = floor(dm/100);		//get rid of minutes
	float mm = (dm - dd * 100) / 60;	//get rid of degrees and convert to decimal deg
	return (mm + dd);			//add em up and return
}
