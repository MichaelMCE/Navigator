

#ifndef _MAP_H_
#define _MAP_H_




#define DRAWLAYER_POLYGON			(1)
#define DRAWLAYER_POLYGON_OUTLINE	(2)
#define DRAWLAYER_PATH				(3)
#define DRAWLAYER_PATH_SINGLE		(4)
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

typedef struct {
	float y;
	float x;
}vector2_t;

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
	int16_t x;
	int16_t y;
}v16_t;

typedef struct {
	int total;
	v16_t *verts;
}plv_t;


#endif

