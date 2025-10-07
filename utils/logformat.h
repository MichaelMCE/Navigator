
// A bike computer
// 
// Michael McElligott
// okio@users.sourceforge.net

//  Copyright (c) 2005-2015  Michael McElligott
// 
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU LIBRARY GENERAL PUBLIC LICENSE
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU LIBRARY GENERAL PUBLIC LICENSE for more details.
//
//	You should have received a copy of the GNU Library General Public
//	License along with this library; if not, write to the Free
//	Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.



#ifndef _LOGFORMAT_H_
#define _LOGFORMAT_H_


#include <inttypes.h>


#define LOG_FILENAME_MAXLENGTH		(16)


typedef struct {
	char filename[LOG_FILENAME_MAXLENGTH];
	char date[12];
	char time[12];
	char timeStart[16];
	char timeEnd[16];
	char totalSeconds[12];	// total moving time in seconds
	char speedMax[6];
	
#if 0
	uint32_t dateStart;		// date activity begun
	uint32_t dateEnd;		// date activity ended (need not be same date as above)
	uint32_t timeStart;		// actual time of activity start
	uint32_t timeEnd;		// actual time of activity end
	uint32_t timeMoving;	// total seconds moving

	uint32_t distance;		// meters
	
	uint16_t speedAve;		// km/h
	uint16_t speedMax;		// km/h
	
	int8_t tempMin;			// c
	int8_t tempAve;			// c
	int8_t tempMax;			// c
	int8_t altMin;			// meters
	
	uint16_t altMax;		// meters
	uint8_t cadenceAve;		// rpm
	uint8_t cadenceMax;		// rpm
	
	uint32_t filler[64];	// future use. fill with 0x00
#endif

}log_header_t;

typedef struct {
	double latitude;
	double longitude;
}gps_pos_t;

typedef struct {
	gps_pos_t pos;
	float altitude;		// could make this uint16_t
	uint32_t time;
}gps_datapt_t;

typedef struct {
	uint32_t total;
	gps_datapt_t *pt;
	log_header_t header;
}track_t;




#endif

