


#include "commonGlue.h"




application_t inst;


FLASHMEM void map_init (vfont_t *vfont)
{
	memset(&inst, 0, sizeof(inst));

	inst.vfont = vfont;
	inst.cmdTaskRunMode = 1;
	inst.runLog.step = 4;
	inst.loadTiles = 1;

	sceneSetColourScheme(2);
	inst.scheme.pathThickness = 11;
	inst.scheme.spotRadius = 6;
	
	log_runReset();	
	poi_t *poi = &inst.poi;
	poiInit(poi);
	
	sceneInit(&inst);
	sceneSetHeading(&inst, 0);
	sceneSetZoom(&inst, SCENE_ZOOM);
	sceneResetViewport(&inst);
	
	map_setDetail(MAP_RENDER_POI, 1);
	map_setDetail(MAP_RENDER_COMPASS, 1);
	map_setDetail(MAP_RENDER_VIEWPORT, 1);
	map_setDetail(MAP_RENDER_TRACKPOINTS, 1);
	map_setDetail(MAP_RENDER_LOCGRAPTHIC, 1);
	map_setDetail(MAP_RENDER_OVERLAY, 1);
	
	map_setDetail(MAP_RENDER_SLEVELS, 1);
	map_setDetail(MAP_RENDER_SAVAIL, 1);
	map_setDetail(MAP_RENDER_SWORLD, 1);
	map_setDetail(MAP_RENDER_CONSOLE, 0);
}

void map_render (trackRecord_t *trackRecord, const pos_rec_t *location, const float heading, const uint32_t flags)
{
	vectorPt2_t position;
	position.lon = location->longitude;
	position.lat = location->latitude;
	
	vectorPt2_t preLoc = sceneGetLocation(&inst);
	sceneSetLocation(&inst, &position);
	sceneSetHeading(&inst, heading);

	if (flags&MAP_RENDER_VIEWPORT){
		inst.distance = sceneCaleDistanceVecPt2(&position, &preLoc);
		if (inst.distance > SCENE_TILE_DISTANCE){
			inst.loadTiles = SCENE_TILE_COUNT;
			preLoc = position;
		}
	}

	if (inst.rstats.rflags.map)
		if (flags&MAP_RENDER_VIEWPORT)	  sceneRenderViewport(&inst);
	if (inst.rstats.rflags.trkPts)
		if (flags&MAP_RENDER_TRACKPOINTS) sceneRenderTrackPoints(&inst, trackRecord);
	if (inst.rstats.rflags.locgraphic)
		if (flags&MAP_RENDER_LOCGRAPTHIC) sceneRenderLocGraphic(&inst);
	if (inst.rstats.rflags.overlay)
		if (flags&MAP_RENDER_OVERLAY)	  sceneRenderOverlay(&inst);
	if (inst.rstats.rflags.compass)
		if (flags&MAP_RENDER_COMPASS)	  sceneRenderCompass(&inst);
	if (inst.rstats.rflags.poi)
		if (flags&MAP_RENDER_POI)		  sceneRenderPOI(&inst);
}

void map_setDetail (const uint32_t detail, uint32_t state)
{
	state &= 0x01;
	
	if (detail == MAP_RENDER_POI)
		inst.rstats.rflags.poi = state;
	else if (detail == MAP_RENDER_COMPASS)
		inst.rstats.rflags.compass = state;
	else if (detail == MAP_RENDER_TRACKPOINTS)
		inst.rstats.rflags.trkPts = state;
	else if (detail == MAP_RENDER_SLEVELS)
		inst.rstats.rflags.satlevels = state;
	else if (detail == MAP_RENDER_SAVAIL)
		inst.rstats.rflags.satAvailability = state;
	else if (detail == MAP_RENDER_SWORLD)
		inst.rstats.rflags.satWorld = state;
	else if (detail == MAP_RENDER_CONSOLE)
		inst.rstats.rflags.console = state;
	else if (detail == MAP_RENDER_VIEWPORT)
		inst.rstats.rflags.map = state;
	else if (detail == MAP_RENDER_LOCGRAPTHIC)
		inst.rstats.rflags.locgraphic = state;
	else if (detail == MAP_RENDER_OVERLAY)
		inst.rstats.rflags.overlay = state;
	
}
