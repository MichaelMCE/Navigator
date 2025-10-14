




#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mp.h"
#include "polyfile.h"
#include "logformat.h"



// source location
#define POLY_SRC_PATH			((uint8_t*)"m:\\polys\\500")
#define SRC_PATH				POLY_SRC_PATH

// destination location
#define DES_PATH				((uint8_t*)"m:\\polys\\500_32")






// MAP_SOURCE coverage (gmapsupp.mp)

#if 1
// original			155meg
static const mp_coverage_t coverage = {
		{{55.25573, -8.188934}, {54.022522, -5.413513}},
		2.775421/*2.77542*/,	// ° width
		1.233208/*1.23322*/		// ° height
};
#else
//planet_-8.403,53.98_-4.268,55.434-garmin-osm
// 315mb
static const mp_coverage_t coverage = {
		{{55.434, -8.403}, {53.98, -4.268}},
		4.135,		// ° width
		1.454		// ° height
};
#endif






uint64_t lof (FILE *stream)
{
	fpos_t pos;
	fgetpos(stream, &pos);
	fseek(stream, 0, SEEK_END);
	uint64_t fl = ftell(stream);
	fsetpos(stream,&pos);
	return fl;
}


size_t polypack (const int fileX, const int fileY)
{

	poly_pack_header_t header;
	memset(&header, 0, sizeof(header));
	
	char file_src[64];
	char file_des[64];
	
	const int w = PACK_ACROSS;
	const int h = PACK_DOWN;


	void *buffer = calloc(256, 1024);		// should be enough for any file
	if (!buffer) return 0;
	
	snprintf(file_des, sizeof(file_des), "%s\\%03i_%03i.pk32", DES_PATH, fileY, fileX);
	FILE *fp_des = fopen(file_des, "w+b");
	if (!fp_des){
		free(buffer);
		return 0;
	}

	size_t offset = sizeof(header);
	
	for (int  y = 0; y < h; y++){
		for (int x = 0; x < w; x++){
			snprintf(file_src, sizeof(file_src), "%s\\%03i_%03i.poly", SRC_PATH, fileY+y, fileX+x);
			
			FILE *fp_src = fopen(file_src, "r+b");
			if (!fp_src) continue;
			
			header.file[y][x].length = lof(fp_src);
			if (header.file[y][x].length){
				header.file[y][x].offset = offset;

				fseek(fp_des, offset, SEEK_SET);
				fread(buffer, header.file[y][x].length, 1, fp_src);
				fwrite(buffer, header.file[y][x].length, 1, fp_des);

				offset += header.file[y][x].length;
			}
			fclose(fp_src);
		}
	}
	
	fseek(fp_des, 0, SEEK_SET);
	fwrite(&header, sizeof(header), 1, fp_des);
	
	fclose(fp_des);
	free(buffer);

	return offset;
}



int main (const int argc, const char *argv[])
{
	
	const int blockTotalLon = ceilf(coverage.width / GPS_LENGTH_LON);
	const int blockTotalLat = ceilf(coverage.height / GPS_LENGTH_LAT);
		
	for (int y = 0; y < blockTotalLat; y += PACK_ACROSS){
		for (int x = 0; x < blockTotalLon; x += PACK_DOWN)
			polypack(x, y);
	}
	
	return 1;
}
