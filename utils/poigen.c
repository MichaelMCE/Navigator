




#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mp.h"
#include "poi.h"




#include "libmylcd/fileal.h"


#define MAP_SOURCE				((uint8_t*)"gmapsupp.mp")
#define POI_SRC_SOURCE			((uint8_t*)"poi0.txt")
#define POI_DES_PATH			((uint8_t*)"m:\\polys\\poi")


// MAP_SOURCE coverage, gmapsupp.mp (155 MB)
static const mp_coverage_t coverage = {
		{{55.25573, -8.188934}, {54.022522, -5.413513}},
		2.775421/*2.77542*/,	// ° width
		1.233208/*1.23322*/		// ° height
};





static inline int isVectorInRegion (const vectorPt2_t *vec, const vectorPt4_t *win)
{
	return  (vec->lat <= win->v1.lat && vec->lat >= win->v2.lat) &&
			(vec->lon >= win->v1.lon && vec->lon <= win->v2.lon);
}

static inline void block2Location (const int32_t x_lon, const int32_t y_lat, vectorPt2_t *loc)
{
	loc->lat = fabsf(coverage.region.v1.lat) - (y_lat * GPS_LENGTH_LAT);
	loc->lon = (x_lon * GPS_LENGTH_LON) - fabsf(coverage.region.v1.lon);
}

static inline void makeBlockWindow (const int32_t x_lon, const int32_t y_lat, vectorPt4_t *out)
{
	vectorPt2_t loc;
	block2Location(x_lon, y_lat, &loc);
	
	out->v1.lon = loc.lon;						// top left
	out->v1.lat = loc.lat;
	out->v2.lon = loc.lon + POI_LENGTH_LON;		// bottom right
	out->v2.lat = loc.lat - POI_LENGTH_LAT;
}

static inline int isLinePOI (const uint8_t *string) 
{
	return !strncmp((char*)string, "[POI]", 5);
}

static inline int isLinePOLYGON (const uint8_t *string) 
{
	return !strncmp((char*)string, "[POLYGON]", 9);
}

static inline int isLineLABEL (const uint8_t *string) 
{
	return !strncmp((char*)string, "Label=", 6);
}

static inline int isLineType (const uint8_t *string) 
{
	return !strncmp((char*)string, "Type=", 5);
}

static inline int isLineData (const uint8_t *string) 
{
	return !strncmp((char*)string, "Data", 4);
}

static inline int isLineEND (const uint8_t *string) 
{
	return !strncmp((char*)string, "[END]", 5);
}

static inline int isLineDATA (const uint8_t *string, const int level) 
{
	if (!strncmp((char*)string, "Data", 4)){
		if (/*!level ||*/ atoi((char*)&string[4]) == level)
			return 1;
		return -1;
	}
	return 0;
}

static inline void poiDump (const uint8_t *path, const int zoomLevel)
{
	uint32_t lines[128] = {0};
	int lineIdx = 0;
	int hasLabel = 0;
	
	
	TASCIILINE *al = readFileA((char*)path);
	if (!al) return;
	
	int isPOI = 0;
	
	for (int i = 0; i < al->tlines; i++){
		uint8_t *line = al->line[i];
		
		if (!isPOI && isLinePOI(line)){					// start of POI block
			isPOI = 1;
			lineIdx = 0;
			hasLabel = 0;

		}else if (!isPOI && isLinePOLYGON(line)){		// start of block
			isPOI = 1;
			lineIdx = 0;
			hasLabel = 0;
			
		}else if (isPOI && isLineEND(line)){			// end of POI block
			isPOI = 0;
			if (!hasLabel) continue;

			for (int j = 0; j < lineIdx; j++)
				printf("%s\n", al->line[lines[j]]);
				
			printf("\n");
		}else if (isPOI){								// POI block
			if (isLineDATA(line, zoomLevel) == -1){		// it's a Data field but not the zoom level we're interested in
				isPOI = 0;
			}else{
				if (isLineLABEL(line)) hasLabel = 1;
				lines[lineIdx++] = i;
			}
		}
	}

	freeASCIILINE(al);
}

static inline int readVector (uint8_t *string, vector2_t *vec)
{
	int tmp;
	return sscanf((char*)string, "Data%i=(%f,%f)", &tmp, &vec->y, &vec->x) == 3;
}

static inline uint32_t readType (uint8_t *string)
{
	uint32_t type = 0;
	if (sscanf((char*)string, "Type=%i", &type) == 1)
		return type;
	return 0;
}


// https://wiki.openstreetmap.org/wiki/OSM_Map_On_Garmin/POI_Types
// https://www.cferrero.net/maps/maps_index.html
static inline int isTypeWanted (const uint32_t garminType)
{
	switch (garminType){
		
	//case 0x0013:		// buildings
	case 0x0050:		// forest
	case 0x0052:		// forest park
	case 0x003C:		// LOUGHs
		
	case 0x0100:
	case 0x0200:
	case 0x0300:
	case 0x0400:
	case 0x0500:
	case 0x0600:
	case 0x0700:
	case 0x0800:
	case 0x0900:
	case 0x0A00:
	case 0x0B00:
	case 0x0C00:
	case 0x0D00:
	case 0x0E00:
	case 0x0F00:
	case 0x1000:
	case 0x1100:

	case 0x1E00:
	case 0x1F00:
	
	case 0x2c07: 	// zoo's
	
	case 0x3001:	// police station
	case 0x3002:	// hospital
	case 0x3003:	// city hall
	case 0x3004:	// court house
	case 0x3006:	// border crossing
	case 0x3008:	// fire station
	
	case 0x5900:
	case 0x5901:
	case 0x5902:
	case 0x5903:
	case 0x5904:
	case 0x5905:
	
	case 0x6604:
	case 0x6612:
	
	case 0x660A:
	
	case 0x6618:
		return 1;
	}
	 
	if (garminType > 0x6600 && garminType <= 0x7400)
		return 1;
	 
	return 0;
}

static int poiWriteRegion (TASCIILINE *al, FILE *file, const vectorPt4_t *region)
{    
	vectorPt2_t vec;
	int poiStart = 0;
	uint16_t type = 0;
	uint8_t *label = NULL;
	int labelLen = 0;
	int generated = 0;
	
	for (int i = 0; i < al->tlines; i++){
		uint8_t *line = al->line[i];
		
		if (!poiStart &&  isLineType(line)){
			type = readType(line);
			poiStart = isTypeWanted(type);
			
		}else if (poiStart && isLineLABEL(line)){
			label = line + strlen("Label=");
			labelLen = strlen((char*)label);
			if (labelLen < 2) poiStart = 0;

		}else if (poiStart && isLineData(line)){
			poiStart = 0;

			if (readVector((uint8_t*)line, (vector2_t*)&vec)){
				if (isVectorInRegion(&vec, region)){
					generated = 1;
										
					poi_obj_t poi;
					poi.vec = vec;
					poi.type = type;
					poi.len = labelLen;
					
					fwrite(&poi, sizeof(poi), 1, file);
					fwrite(label, labelLen, 1, file);
				}
			}
		}
	}
	
	return generated;
}

static inline int poiSplit (TASCIILINE *al, const uint8_t *dir, const int32_t x_lon, const int32_t y_lat)
{
	vectorPt4_t region;
	makeBlockWindow(x_lon, y_lat, &region);
	
	//printf("%lf %lf, %lf %lf\n", region.v1.lat, region.v1.lon, region.v2.lat, region.v2.lon);
	
	char filename[64];
	snprintf(filename, sizeof(filename), "%s\\%03i_%03i.poi", dir, y_lat, x_lon);
	
	uint32_t poiCount = 0;
	
	FILE *file = fopen(filename, "w+b");
	if (file){
		poiCount = poiWriteRegion(al, file, &region);
		fclose(file);
	}
	
	//printf("poiCount %i\n", poiCount);
	
	return poiCount;
}


static inline void poiGenBlocks (uint8_t *mpFile, const mp_coverage_t *coverage, uint8_t *destDir)
{
	TASCIILINE *al = readFileA((char*)mpFile);
	if (!al) return;
	
	const int blockTotalLon = ceil(coverage->width / GPS_LENGTH_LON);
	const int blockTotalLat = ceil(coverage->height / GPS_LENGTH_LAT);
	printf("Block area: %ix%i\n", blockTotalLon, blockTotalLat);
	printf("Block total: %i\n", blockTotalLat*blockTotalLon);
	printf("scanning..\n");

	int ct = 0;

	for (int y = 0; y < blockTotalLat; y += 4){
		for (int x = 0; x < blockTotalLon; x += 4){
			ct += poiSplit(al, destDir, x, y);
		}
	}

	freeASCIILINE(al);

	printf("POI files generated: %i\n", ct);
}

int main (const int argc, const char *argv[])
{
	printf("Map source: %s\n", (char*)MAP_SOURCE);	
	printf("Coverage: NW: %f %f\n", coverage.region.v1.lat, coverage.region.v1.lon);
	printf("Coverage: SE: %f %f\n", coverage.region.v2.lat, coverage.region.v2.lon);
	printf("Coverage: %f by %f\n", coverage.width, coverage.height);
	printf("\n");


#if 0
	// poi_mp2txt
	
	if (argc > 1){
		printf("Generating POI for zoom level %i\n\n", atoi(argv[1]));
		poiDump(MAP_SOURCE, atoi(argv[1]));
	}else{
		printf("Provide a zoom level. 0 = highest detail, 3 = lowest.");
	}
	
#else
	// poi_txt2blks

	uint8_t *src = POI_SRC_SOURCE;
	if (argc > 1) src = (uint8_t*)argv[1];
	
	printf("Source: %s\n", (char*)src);
	printf("Destination: %s\\\n", POI_DES_PATH); 

	poiGenBlocks(src, &coverage, POI_DES_PATH);
	
#endif
	return 1;
}
