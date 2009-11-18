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
#include "lcd.h"
#include "fat32.h"

#define PLENGTH 16
#define ITERATIONS 64
#define sq(a) ((a)*(a))

char gps_calcchecksum(const char * s)
{
	s++;
	char c = *s++;

	while (*s != '*' && *s != '\0')
		c ^= *s++;
	
	return c;
}

int gps_log_data(char * data, struct gps_location * loc)
{
	int i = 0;
	int j = 0;
	int field = 0;
	char temp[PLENGTH];
	char done_flag = 0;
	
	while (!done_flag) {			//while still in the string
		while (data[i] != ',') {	//field by field in a comma separated file
			temp[j] = data[i];	//copy to temp
			i++;
			j++;
		}
		temp[j] = '\0';
		j = 0;
		i++;
		
		switch (field) {
			case 0:			//error checking, return -1 if wrong data
				//if (strncmp("$GPRMC", temp, 6)) {lcd_print(" k"); return -1;}
				if (temp[1] != 'G') return -1;
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
			default:
				done_flag = 1;
				break;
		}
		field++;
	}
	return 0;
}

int gps_calc_disp(struct gps_location * gl1, struct gps_location * gl2, struct gps_displacement * gd)
{
	double lat1 = dm_to_dd(gl1->lat, gl1->ns);
	double lon1 = dm_to_dd(gl1->lon, gl1->ew);
	double lat2 = dm_to_dd(gl2->lat, gl2->ns);
	double lon2 = dm_to_dd(gl2->lon, gl2->ew);
	
	double a = 6378137;
	double b = 6356752.3142;
	double f = 1/298.257223563;		//flatening of geoid;
	double L = (lon2 - lon1) * M_PI / 180;	//difference in longitude in radians
	double s1 = sin(atan((1 - f) * tan(lat1 * M_PI / 180)));	//sin of reduced lat1
	double c1 = cos(atan((1 - f) * tan(lat1 * M_PI / 180)));	//cos of reduced lon1
	double s2 = sin(atan((1 - f) * tan(lat2 * M_PI / 180)));	//sin of reduced lat2
	double c2 = cos(atan((1 - f) * tan(lat2 * M_PI / 180)));	//cos of reduced lon2
	
	double lambda = L;
	double ltemp, sigma, ss /*sin(sigma)*/, cs /*cos(sigma)*/, sa /*sin(alpha), ca cos(alpha)*/, csqa /*cos^2(alpha(*/, c2sm /*cos(2sigma sub m)*/, C, usq /*u^2*/, A, B, ds /*delta(sigma)*/;
	int8_t lim = 0;
	
	do {	
		lim++;
		ss = sqrt(sq(c2 * sin(lambda)) + sq(c1 * s2 - s1 * c2 * cos(lambda)));	//sin(sigma)
		cs = s1 * s2 + c1 * c2 * cos(lambda);			//cos(sigma)
		sigma = atan2(ss , cs);
		sa = c1 * c2 * sin(lambda) / ss;			//sin(alpha)
		csqa = 1 - sq(sa);					//cos_squared(alpha)
		c2sm = cs - 2 * s1 * s2 / csqa;				//cos(2sigma_m)
		C = f / 16 * csqa * (4 + f * (4 - 3 * csqa));		//intermediate value
		ltemp = lambda;						//store old value of lambda
		lambda = L + (1 - C) * f * sa * (sigma + C * ss * (c2sm + C * cs * (-1 + 2 * sq(c2sm))));	//get new value
		//lcd_printf("ltemp: %d\nlambda: %d",(int)ltemp,(int)lambda);
		//printf("ltemp: %f\nlambda: %f\niterations: %d\n",ltemp,lambda,lim);
	} while ((((lambda - ltemp) > 1e-12) || ((ltemp - lambda) > 1e-12)) && lim < ITERATIONS);	//check for accuracy or too many iterations

	usq = csqa * (sq(a) - sq(b)) / sq(b);				//u squared
	A = 1 + usq / 16384 * (4096 + usq * (-768 + usq * (320 - 175 * usq)));	//intermediate value
	B = usq / 1024 * (256 + usq * (-128 + usq * (74 - 47 * usq)));	//intermediate value
	ds = B * ss * (c2sm + B / 4 * (cs * (-1 + 2 * sq(c2sm)) - B / 6 * c2sm * (-3 + 4 * sq(ss)) * (-3 + 4 * sq(c2sm)))); //delta sigma
	
	gd->magnitude = b * A * (sigma - ds);				//displacement magnitude
	gd->initial_bearing = atan2(c2 * sin(lambda) , c1 * s2 - s1 * c2 * cos(lambda)) * 180. / M_PI;	//displacement initial bearing	
	gd->final_bearing = atan2(c1 * sin(lambda) , -s1 * c2 + c1 * s2 * cos(lambda)) * 180. / M_PI;		//displacement final bearing
	gd->iterations = lim;
	
	// make the bearings always positive
	if (gd->initial_bearing < 0) gd->initial_bearing += 360.;
	if (gd->final_bearing < 0) gd->final_bearing += 360.;
	
	return 0;
}

double dm_to_dd(double dm, char nsew)
{
	double dd = floor(dm/100);		//get rid of minutes
	double mm = (dm - dd * 100) / 60;	//get rid of degrees and convert to decimal deg
	//printf("dm: %f dd: %f mm: %f mm + dd: %f\n",dm,dd,mm,mm+dd);
	return (mm + dd) * ((nsew == 'W' || nsew == 'S') ? -1 : 1);			//add em up and return
}

const char map_start[] = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<kml xmlns=\"http://earth.google.com/kml/2.0\">\n";
const char map_end[] = "</kml>";
const char map_pointstart[] = "<Placemark>\n<description>";
const char map_pointmiddle[] = "</description><name>";
const char map_pointname[] = "</name>\n<Point>\n<coordinates>";
const char map_pointend[] = "</coordinates>\n</Point>\n</Placemark>\n\n";

void ftostr(double f, char * buf, int n)
{
	
}

void log_start(const char * name, struct fatwrite_t * fwrite)
{
	del(name);
	touch(name);
	write_start(name, fwrite);

	write_add(fwrite, map_start, sizeof(map_start)-1);
}

void log_end(struct fatwrite_t * fwrite)
{
	write_add(fwrite, map_end, sizeof(map_end)-1);

	write_end(fwrite);
}

void log_add(struct fatwrite_t * fwrite, struct gps_location * gl)
{
	char buf[128];
	
	write_add(fwrite, map_pointstart, sizeof(map_pointstart)-1);
	
	// add speed
	snprintf(buf, 128, "Speed: %.2fmph", gl->sog*1.5);	
	write_add(fwrite, buf, strlen(buf));
	write_add(fwrite, map_pointmiddle, sizeof(map_pointmiddle)-1);
	
	// add time
	snprintf(buf, 128, "%u/%u/%u %u:%u:%u",
			(gl->date/100)%100, gl->date/10000, gl->date%100,
			((int)gl->time)/10000, (((int)gl->time)/100)%100, ((int)gl->time)%100);	
	write_add(fwrite, buf, strlen(buf));
	write_add(fwrite, map_pointname, sizeof(map_pointname)-1);

	// add coordinates
	snprintf(buf, 64, "%.7f,%.7f", dm_to_dd(gl->lon, gl->ew), dm_to_dd(gl->lat, gl->ns));
	write_add(fwrite, buf, strlen(buf));
	
	write_add(fwrite, map_pointend, sizeof(map_pointend)-1);
}

