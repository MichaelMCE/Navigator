

#ifndef _map_h_
#define _map_h_



#include "gps.h"



#define MAP_RENDER_VIEWPORT		0x001
#define MAP_RENDER_TRACKPOINTS	0x002
#define MAP_RENDER_LOCGRAPTHIC	0x004
#define MAP_RENDER_OVERLAY		0x008
#define MAP_RENDER_COMPASS		0x010
#define MAP_RENDER_POI			0x020
#define MAP_RENDER_SLEVELS		0x040
#define MAP_RENDER_SAVAIL		0x080
#define MAP_RENDER_SWORLD		0x100
#define MAP_RENDER_CONSOLE		0x200



typedef struct {
	struct {
		int strings;
		float map;
		float trkpts;
		float poi;
	}rtime;

	int trkptsTotal;
	int trkptsToWrite;
	
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



void map_init (vfont_t *vfont);
void map_render (trackRecord_t *trackRecord, const pos_rec_t *location, const float heading, const uint32_t flags);

void map_setDetail (const uint32_t detail, uint32_t state);

#endif

