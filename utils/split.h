




#ifndef _SPLIT_H_
#define _SPLIT_H_



#if 1
#define VWIDTH					(320)
#define VHEIGHT					(240)
#elif 0
#define VWIDTH					(800)
#define VHEIGHT					(480)
#else
#define VWIDTH					(1024)
#define VHEIGHT					(800)
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


#ifndef EARTH_RADIUS

#define EARTH_RADIUS			(6378137.0)
#define LENGTH_SCALE			(1.0/(EARTH_RADIUS * (M_PI/180.0)))
#define LENGTH_1000M_LAT		(LENGTH_SCALE * 1000.0)
//#define LENGTH_1000M_LON		((LENGTH_SCALE * 1000.0) / cos(54.590378*(M_PI/180.0)))
//#define LENGTH_1000M_LON		((LENGTH_SCALE * 1000.0) / cosLatPreCalc)
#define LENGTH_1000M_LON		(0.0155037502718)


#if 1
// rough values used for [window] display calculations only
#define GPS_1000M_LAT			(0.008983152841f)
#define GPS_1000M_LON			(0.015511630655f)
#else
#define GPS_1000M_LAT			(LENGTH_1000M_LAT)
#define GPS_1000M_LON			(LENGTH_1000M_LON)
#endif

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



#define CALC_PITCH_1(w)			(((w)>>3)+(((w)&0x07)!=0))	// 1bit packed, calculate number of storage bytes per row given width (of glyph)
#define CALC_PITCH_16(w)		((w)*sizeof(uint16_t))		// 16bit, 8 bits per byte
#define COLOUR_24TO16(c)		((((c>>16)&0xF8)<<8) | (((c>>8)&0xFC)<<3) | ((c&0xF8)>>3))

// LFRM_BPP_16 - RGB 565
#define RGB_16_RED				(0xF800)
#define RGB_16_GREEN			(0x07E0)
#define RGB_16_BLUE				(0x001F)
#define RGB_16_WHITE			(RGB_16_RED|RGB_16_GREEN|RGB_16_BLUE)
#define RGB_16_BLACK			(0x0000)
#define RGB_16_MAGENTA			(RGB_16_RED|RGB_16_BLUE)
#define RGB_16_YELLOW			(RGB_16_RED|RGB_16_GREEN)
#define RGB_16_CYAN				(RGB_16_GREEN|RGB_16_BLUE)

#define COLOUR_RED				RGB_16_RED
#define COLOUR_GREEN			RGB_16_GREEN
#define COLOUR_BLUE				RGB_16_BLUE
#define COLOUR_WHITE			RGB_16_WHITE
#define COLOUR_BLACK			RGB_16_BLACK
#define COLOUR_MAGENTA			RGB_16_MAGENTA
#define COLOUR_YELLOW			RGB_16_YELLOW
#define COLOUR_CYAN				RGB_16_CYAN

#define COLOUR_CREAM			(COLOUR_24TO16(0xEEE7D0))
#define COLOUR_AQUA				(COLOUR_24TO16(0x00B7EB))
#define COLOUR_ORANGE			(COLOUR_24TO16(0xFF7F11))
#define COLOUR_BLUE_SEA_TINT	(COLOUR_24TO16(0x508DC5))		/* blue, but not too dark nor too bright. eg; Glass Lite:Volume */
#define COLOUR_GREEN_TINT		(COLOUR_24TO16(0x00FF1E))		/* softer green. used for highlighting */
#define COLOUR_PURPLE_GLOW		(COLOUR_24TO16(0xFF10CF))
#define COLOUR_GRAY				(COLOUR_24TO16(0x777777))
#define COLOUR_HOVER			(COLOUR_24TO16(0x28C672))
#define COLOUR_TASKBARFR		(COLOUR_24TO16(0x141414))
#define COLOUR_TASKBARBK		(COLOUR_24TO16(0xD4CAC8))
#define COLOUR_SOFTBLUE			(COLOUR_24TO16(0x7296D3))
#define COLOUR_LIGHTGREY		(COLOUR_24TO16(0x444444))
#define COLOUR_LIGHTBROWN		(COLOUR_24TO16(0xFFA014))
#define COLOUR_DARKBROWN		(COLOUR_24TO16(0xC73E10))
#define COLOUR_DARKERBROWN		(COLOUR_24TO16(0xA71E0B))






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
	float y;
	float x;
}vector2_t;

typedef struct {
	vector2_t v1;
	vector2_t v2;
}vector4_t;

typedef struct {
	uint32_t total;
	vector2_t *list;
}vectors_t;

typedef struct {
	vectorPt4_t region;
	double width;		// °
	double height;		// °
}mp_coverage_t;

typedef struct {
	uint32_t index;		// vector index in to polyline->vectors->list[]
	uint32_t id;
}vectorNode_t;

typedef struct {
	uint32_t total;
	vectorNode_t *list;
}nodes_t;

typedef struct {
	vectors_t *vectors;
	uint32_t type;
	nodes_t nodes;
	uint32_t isPolygon;
}polyline_t;

typedef struct {
	uint32_t total;		// amount of total used
	uint32_t size;		// total allocated
	uint32_t incBy;
	polyline_t **list;
	
	uint32_t vectorTotal;
}polylines_t;



#endif

