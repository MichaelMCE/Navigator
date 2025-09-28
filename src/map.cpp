

#include <Arduino.h>
#include "config.h"
#include "vfont/vfont.h"
#include "gps.h"
#include "scene.h"
#include "map.h"
#include "tiles.h"
#include "poi.h"




extern runState_t rstats;
application_t inst;




void map_init (vfont_t *vfont)
{
	memset(&inst, 0, sizeof(inst));
	
	inst.colourScheme = 1;
	inst.vfont = vfont;
	sceneSetZoom(&inst, SCENEZOOM);
	
	poi_t *poi = &inst.poi;
	poiInit(poi);
	
	sceneInit();
	sceneSetHeading(&inst, 0);
	
	map_setDetail(MAP_RENDER_POI, 1);
	map_setDetail(MAP_RENDER_COMPASS, 1);
	map_setDetail(MAP_RENDER_VIEWPORT, 1);
	map_setDetail(MAP_RENDER_TRACKPOINTS, 1);
	map_setDetail(MAP_RENDER_LOCGRAPTHIC, 1);
	map_setDetail(MAP_RENDER_OVERLAY, 0);
	
	map_setDetail(MAP_RENDER_SLEVELS, 1);
	map_setDetail(MAP_RENDER_SAVAIL, 1);
	map_setDetail(MAP_RENDER_SWORLD, 1);
	map_setDetail(MAP_RENDER_CONSOLE, 1);
}

static inline float calcDistance (const float x1, const float y1, const float x2, const float y2)
{
	const float x = x1 - x2;
	const float y = y1 - y2;
	return sqrtf((x * x) + (y * y));

	//return sqrtf((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

void map_render (trackRecord_t *trackRecord, const pos_rec_t *location, const float heading, const uint32_t flags)
{
	static int32_t loadCount = 0;
	//static uint32_t preLoc = 0;
	static vectorPt2_t preLoc;
	//static vectorPt2_t previousLoc;
	
	vectorPt2_t position;
	position.lon = location->longitude;
	position.lat = location->latitude;
	
	sceneSetLocation(&inst, &position);
	sceneSetHeading(&inst, heading);

	if (flags&MAP_RENDER_VIEWPORT){
		if (!loadCount--){
			//int32_t xlon, ylat;
			//sceneLocation2Tile(&inst.viewport.location, &xlon, &ylat);
			//uint32_t loc = ((xlon&0xFFFF)<<16) | (ylat&0xFFFF);
			
			//if (preLoc != loc){	// check distance is over, say, 50m
			float distance = calcDistance(position.lat, position.lon, preLoc.lat, preLoc.lon);
			if (distance >= 0.005f){
				preLoc = position;
				//preLoc = loc;
				//sceneFlushTiles(inst);
				tilesUnload(inst.renderPassCt);
				//tilesUnloadAll(inst);
				//poiCleanBlocks(&inst->poi);
			}

			sceneLoadTiles(&inst);
			loadCount = 8;
		}
	}

	if (rstats.rflags.map)
		if (flags&MAP_RENDER_VIEWPORT)	  sceneRenderViewport(&inst);
	if (rstats.rflags.trkPts)
		if (flags&MAP_RENDER_TRACKPOINTS) sceneRenderTrackPoints(&inst, trackRecord);
	if (rstats.rflags.locgraphic)
		if (flags&MAP_RENDER_LOCGRAPTHIC) sceneRenderLocGraphic(&inst);
	//if (rstats.rflags.)
	//	if (flags&MAP_RENDER_OVERLAY)	  sceneRenderOverlay(&inst);
	if (rstats.rflags.compass)
		if (flags&MAP_RENDER_COMPASS)	  sceneRenderCompass(&inst);
	if (rstats.rflags.poi)
		if (flags&MAP_RENDER_POI)		  sceneRenderPOI(&inst);
}

void map_setDetail (const uint32_t detail, uint32_t state)
{
	state &= 0x01;
	
	if (detail == MAP_RENDER_POI)
		rstats.rflags.poi = state;
	else if (detail == MAP_RENDER_COMPASS)
		rstats.rflags.compass = state;
	else if (detail == MAP_RENDER_TRACKPOINTS)
		rstats.rflags.trkPts = state;
	else if (detail == MAP_RENDER_SLEVELS)
		rstats.rflags.satlevels = state;
	else if (detail == MAP_RENDER_SAVAIL)
		rstats.rflags.satAvailability = state;
	else if (detail == MAP_RENDER_SWORLD)
		rstats.rflags.satWorld = state;
	else if (detail == MAP_RENDER_CONSOLE)
		rstats.rflags.console = state;
	else if (detail == MAP_RENDER_VIEWPORT)
		rstats.rflags.map = state;
	else if (detail == MAP_RENDER_LOCGRAPTHIC)
		rstats.rflags.locgraphic = state;
}


