
#ifndef _POI_H_
#define _POI_H_



#if 0
#define POI_MAX_STORAGE			(1)		// total combined [string] storage space (lengths' including nulls)
#define POI_MAX_BLOCKS			(1)		// max blocks per instance
#define POI_MAX_STRING			(1)		// max strings per block
#define POI_MAX_STRINGS			(1)		// total number of storable strings
#else
#define POI_MAX_STORAGE			(4*1024)	// total combined [string] storage space (lengths' including nulls)
#define POI_MAX_BLOCKS			(50)	// max blocks per instance
#define POI_MAX_STRING			(20)	// max strings per block
#define POI_MAX_STRINGS			(256)	// total number of storable strings
#endif


typedef struct {
	vectorPt2_t vec;
	uint16_t type;		// type of POI object
	uint16_t len;		// length of following string excluding null
}poi_obj_t;

typedef struct __attribute__((packed)){
	vectorPt2_t vec;
	uint16_t type;
	uint16_t strIdx;				// string offset in to poi_t:strOffsets[], from where the string's offset is located for ::storage
}poi_string_t;

typedef struct __attribute__((packed)){
	uint16_t x;
	uint16_t y;
	
	poi_string_t poi[POI_MAX_STRING];
}poi_file_t;

typedef struct __attribute__((packed)){
	struct {
		uint16_t end;							// end of the storage 'stack' (always begins at 0)
		uint16_t offsets[POI_MAX_STRINGS];		// all string positions within storage are held here, then index'd via poi_string_t:strIdx
		uint8_t storage[POI_MAX_STORAGE];
	}string;
	poi_file_t blocks[POI_MAX_BLOCKS];
}poi_t;



void poiCleanBlocks (poi_t *poi);

const uint8_t *poiGetString (poi_t *poi, const uint16_t strIdx);
int poiBlockAddObj (poi_t *poi, poi_file_t *blk, poi_obj_t *obj, void *data);
int poiIsBlockLoaded (poi_t *poi, const uint16_t x, const uint16_t y);
poi_file_t *poiGetNewBlock (poi_t *poi, const int32_t flush);
uint16_t poiCountStringSlot (poi_t *poi);

void poiInit (poi_t *poi);



#endif

