

#ifndef _scene_H_
#define _scene_H_


#include "gps.h"
#include "record.h"


#define MEMORY_DEBUGGING	(0)
#define MEMORYPROFILE		(1)			// 1: if memory is expensive. slower to repeat load under larger viewports
										// 0: where memory is cheap. faster tile reloads 

#define SCENEZOOM			(200.0f)	// set initial viewport, in meters vertically 
									
#define TILE_UNLOAD_DELTA	(40)		// unload unused blocks after n frames, when a block has not been accessed for n frames, unload it 

#if (VERTICAL_DISPLAY)
#define COVERAGE_OVERSCAN	(1.4f)
#else
#define COVERAGE_OVERSCAN	(1.0f)
#endif

						
									
#define DRAWLAYER_POLYGON			(1)
#define DRAWLAYER_POLYGON_OUTLINE	(2)
#define DRAWLAYER_PATH				(3)
#define DRAWLAYER_PATH_LINE			(4)
#define DRAWLAYER_TILE_BOUNDRY		(5)



#define aspectCorrection  (1.0 / ((double)VWIDTH/(double)VHEIGHT))
#define aspectOffset (double)(((VHEIGHT*0.5) * (1.0 - aspectCorrection)) * ((double)VWIDTH/(double)VHEIGHT))


typedef struct {
	uint16_t x;
	uint16_t y;
}point16_t;

typedef struct {
	float lat;
	float lon;
}vectorPt2_t;

typedef struct {
	vectorPt2_t v1;
	vectorPt2_t v2;
}vectorPt4_t;

typedef struct {
	vectorPt4_t region;
	float width;
	float height;
	uint32_t tilesAcross;
	uint32_t tilesDown;
}mp_coverage_t;

typedef struct __attribute__((packed)){
	vector2_t *list;
	uint8_t total;
}vectors_t;

typedef struct __attribute__((packed)){
	uint32_t type;
	vectors_t vectors;
}polyline_t;

typedef struct {
	uint32_t size;			// length of list
	uint32_t total;
	uint32_t lastRendered;
	polyline_t *list;
}block_t;

typedef struct {
	int total;
	v16_t *verts;
}plv_t;

typedef struct {
	block_t ***block;				// space for [PACK_ACROSS * PACK_DOWN];
}tile8_t;

#include "poi.h"

typedef struct {
	struct {
		int strings;
		float map;
		float trkpts;
		float poi;
		float display;
	}rtime;

	int trkptsTotal;
	int trkptsToWrite;
	
	uint64_t nothingCount;
	uint64_t nothingCountSecond;
	
	struct {
		uint32_t poi:1;
		uint32_t compass:1;
		uint32_t satlevels:1;
		uint32_t satAvailability:1;
		uint32_t locgraphic:1;
		uint32_t satWorld:1;
		uint32_t console:1;
		uint32_t trkPts:1;
		uint32_t map:1;
		uint32_t stub:23;
	}rflags;		// render flags
}runState_t;

typedef struct {
	vfont_t *vfont;
	poi_t poi;
	mp_coverage_t coverage;
	
	struct {
		vectorPt2_t location;
		vectorPt4_t window;
		float zoom;
		float dx;
		float dy;
		float dw;
		float dh;
		float heading;
	}viewport;

	uint32_t colourScheme;
	uint32_t renderPassCt;
	uint32_t renderFlags;
	uint32_t cmdTaskRunMode;
	uint32_t heartbeatPulse;
	
	runState_t rstats;
	
	struct {
		uint32_t enabled:1;
		uint32_t pause:1;
		uint32_t step:4;
		uint32_t idx:24;
		uint32_t stub:2;
	}runLog;
}application_t;

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


void sceneInit ();
void sceneClose ();
void sceneLoadTiles (application_t *inst);
float sceneGetZoom (application_t *inst);
void sceneSetZoom (application_t *inst, const float zoomMeters);
void sceneLocation2Tile (const vectorPt2_t *loc, int32_t *x_lon, int32_t *y_lat);
vectorPt2_t sceneGetLocation (application_t *inst);
void sceneMakeGPSWindow (const vectorPt2_t *center, const double spanMeters, vectorPt4_t *out);
void sceneSetLocation (application_t *inst, const vectorPt2_t *loc);
void sceneSetHeading (application_t *inst, const float heading);
void sceneRenderCompass (application_t *inst);
void sceneRenderViewport (application_t *inst);
void sceneRenderOverlay (application_t *inst);
void sceneResetViewport (application_t *inst);
void sceneRenderTrackPoints (application_t *inst, trackRecord_t *trackRecord);
void sceneRenderLocGraphic (application_t *inst);
void sceneRenderPOI (application_t *inst);

void sceneSetColourScheme (const int colourScheme);

#endif

