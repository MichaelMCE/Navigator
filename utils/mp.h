
#ifndef _MP_H_
#define _MP_H_




#define RUNAS_BENCHMARK		(1)
#define VERTICAL_DISPLAY	(0)		// enable a vertical display ratio fudge (mobile phones)
#define MEMORY_DEBUGGING	(0)
#define MEMORYPROFILE		(0)		// 1: if memory is expensive. slower to repeat load under larger viewports
									// 0: where memory is cheap. faster tile reloads 

//#define SCENEZOOM			(250.0f)	// set initial viewport, in meters vertically 

									
#define TILE_UNLOAD_DELTA	(128)	// unload unused blocks after n frames, when a block has not been accessed for n frames, unload it 

#if (VERTICAL_DISPLAY)
#define COVERAGE_OVERSCAN	(1.4f)
#else
#define COVERAGE_OVERSCAN	(1.1f)
#endif



#define VSCALE				(1.0)

#if 0
#define VWIDTH				(1890)
#define VHEIGHT				(900)
#elif 0
#define VWIDTH				(1080/VSCALE)
#define VHEIGHT				(2400/VSCALE)
#elif 0
#define VWIDTH				(1080/VSCALE)
#define VHEIGHT				(1920/VSCALE)
#elif 1
#define VWIDTH				(int)(1800/VSCALE)
#define VHEIGHT				(800/VSCALE)
#elif 1
#define VWIDTH				(800)
#define VHEIGHT				(480)
#elif 1
#define VWIDTH				(480)
#define VHEIGHT				(320)
#elif 0
#define VWIDTH				(320)
#define VHEIGHT				(240)
#elif 0 
#define VWIDTH				(432)
#define VHEIGHT				(240)
#endif





enum _course {
	COURSE_N,
	COURSE_NE,
	COURSE_E,
	COURSE_SE,
	COURSE_S,
	COURSE_SW,
	COURSE_W,
	COURSE_NW
};




#define EARTH_RADIUS			(6378137.0)
#define LENGTH_SCALE			(1.0/(EARTH_RADIUS * (M_PI/180.0)))
#define LENGTH_1000M_LAT		(LENGTH_SCALE * 1000.0)
//#define LENGTH_1000M_LON		((LENGTH_SCALE * 1000.0) / cos(54.590378*(M_PI/180.0)))
//#define LENGTH_1000M_LON		((LENGTH_SCALE * 1000.0) / cosLatPreCalc)
#define LENGTH_1000M_LON		(0.0155037502718)


#if 1
// rough values used only for window display calculations
#define GPS_1000M_LAT			(0.008983152841)
#define GPS_1000M_LON			(0.015511630655)
#else
#define GPS_1000M_LAT			(LENGTH_1000M_LAT)
#define GPS_1000M_LON			(LENGTH_1000M_LON)
#endif



#define GPS_2000M_LAT			(GPS_1000M_LAT*2.0f)
#define GPS_2000M_LON			(GPS_1000M_LON*2.0f)
#define GPS_500M_LAT			(GPS_1000M_LAT/2.0f)
#define GPS_500M_LON			(GPS_1000M_LON/2.0f)
#define GPS_250M_LAT			(GPS_1000M_LAT/4.0f)
#define GPS_250M_LON			(GPS_1000M_LON/4.0f)
#define GPS_100M_LAT			(GPS_1000M_LAT/10.0f)
#define GPS_100M_LON			(GPS_1000M_LON/10.0f)
#define GPS_50M_LAT				(GPS_1000M_LAT/20.0f)
#define GPS_50M_LON				(GPS_1000M_LON/20.0f)
#define GPS_20M_LAT				(GPS_1000M_LAT/50.0f)
#define GPS_20M_LON				(GPS_1000M_LON/50.0f)
#define GPS_10M_LAT				(GPS_1000M_LAT/100.0f)
#define GPS_10M_LON				(GPS_1000M_LON/100.0f)

#define GPS_LENGTH_LAT			(GPS_500M_LAT)
#define GPS_LENGTH_LON			(GPS_500M_LON)

#define POI_LENGTH_LAT			(GPS_2000M_LAT)
#define POI_LENGTH_LON			(GPS_2000M_LON)





#include "logformat.h"
#include "map.h"
#include "poi.h"
#include "polyfile.h"



#endif

