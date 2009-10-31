//////////////////////////////////
//Zach Norris			//
//This file provides to code to	//
//initialize the GPS chip and	//
//to communicate with the GPS	//
//chip				//
//////////////////////////////////

#include <stdio.h>
#include <avr/io.h>
#include <avr/iom8.h>
#include <string.h>
#include "gpscom.h"
#include <inttypes.h>

#define PLENGTH 10

int gps_log_data(char * data, struct gps_location* loc)
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
				loc.time = atof(temp);
				break;
			case 2:
				loc.status = *temp;
				break;
			case 3:
				loc.lat = atof(temp);
				break;
			case 4:
				loc.ns = *temp;
				break;
			case 5:
				loc.lon = atof(temp);
				break;
			case 6:
				loc.ew = *temp;
				break;
			case 7:
				loc.sog = atof(temp);
				break;
			case 8:
				loc.cog = atof(temp);
				break;
			case 9:
				loc.date = atoi(temp);
				break;
		}
		field++;
	}
	return 0;
}

uint32_t gps_calc_disp(struct gps_location gps0, struct gps_location gps1)
{
	float lat0 = dm_to_dd(gps0.lat);
	float lon0 = dm_to_dd(gps0.lon);
	float lat1 = dm_to_dd(gps1.lat);
	float lon1 = dm_to_dd(gps1.lon);
	
	
}

float dm_to_dd(float dm)
{
	float dd = floor(dm/100);		//get rid of minutes
	float mm = (dm - dd * 100) / 60;	//get rid of degrees and convert to decimal deg
	return (mm + dd);			//add em up and return
}
