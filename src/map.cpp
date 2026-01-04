


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
	map_setDetail(MAP_RENDER_COMPASS, 0);
	map_setDetail(MAP_RENDER_VIEWPORT, 1);
	map_setDetail(MAP_RENDER_TRACKPOINTS, 1);
	map_setDetail(MAP_RENDER_LOCGRAPTHIC, 1);
	map_setDetail(MAP_RENDER_MEASURE, 1);		// ruler
	
	map_setDetail(MAP_RENDER_SLEVELS, 0);
	map_setDetail(MAP_RENDER_SAVAIL, 1);
	map_setDetail(MAP_RENDER_SWORLD, 1);
	map_setDetail(MAP_RENDER_CONSOLE, 0);
	
	map_setDetail(MAP_RENDER_STRINGS, 1);
}

void map_render (trackRecord_t *trackRecord, const pos_rec_t *location, const float heading, const uint32_t flags)
{
	vectorPt2_t position;
	position.lon = location->longitude;
	position.lat = location->latitude;
	
	sceneSetLocation(&inst, &position);
	sceneSetHeading(&inst, heading);

	if (flags&MAP_RENDER_VIEWPORT){
		inst.distance = sceneCaleDistanceVecPt2(&position, &inst.tileLoadLoc);
		if (inst.distance > SCENE_TILE_DISTANCE){
			inst.loadTiles = SCENE_TILE_COUNT;
			inst.tileLoadLoc = position;
		}
	}

	if (inst.rstats.rflags.map)
		if (flags&MAP_RENDER_VIEWPORT)	  sceneRenderViewport(&inst);
	if (inst.rstats.rflags.trkPts)
		if (flags&MAP_RENDER_TRACKPOINTS) sceneRenderTrackPoints(&inst, trackRecord);
	if (inst.rstats.rflags.locgraphic)
		if (flags&MAP_RENDER_LOCGRAPTHIC) sceneRenderLocGraphic(&inst);
	if (inst.rstats.rflags.measure)
		if (flags&MAP_RENDER_MEASURE)	  sceneRenderMeasure(&inst);
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
		
	else if (detail == MAP_RENDER_MEASURE)
		inst.rstats.rflags.measure = state;
		
	else if (detail == MAP_RENDER_STRINGS)
		inst.rstats.rflags.strings = state;

	else if (detail == MAP_RENDER_AREAS)
		inst.rstats.rflags.mapFilled = state;
		
	else if (detail == MAP_RENDER_AREAS_OUTLINE)
		inst.rstats.rflags.mapOutline = state;
		
	else if (detail == MAP_RENDER_PATHS)
		inst.rstats.rflags.pathFilled = state;
		
	else if (detail == MAP_RENDER_PATHS_LINE)
		inst.rstats.rflags.pathLine = state;
		
	else if (detail == MAP_RENDER_TITLE_BOUNDARY)
		inst.rstats.rflags.tileOutline = state;

	
	/*inst->rstats.rflags.trackSpot = 0;
	inst->rstats.rflags.trackPath = 1;
	inst->rstats.rflags.trackLine = 0;*/
}

uint32_t map_getDetail (const uint32_t detail)
{
	if (detail == MAP_RENDER_POI)
		return inst.rstats.rflags.poi;
		
	else if (detail == MAP_RENDER_COMPASS)
		return inst.rstats.rflags.compass;
		
	else if (detail == MAP_RENDER_TRACKPOINTS)
		return inst.rstats.rflags.trkPts;
		
	else if (detail == MAP_RENDER_SLEVELS)
		return inst.rstats.rflags.satlevels;
		
	else if (detail == MAP_RENDER_SAVAIL)
		return inst.rstats.rflags.satAvailability;
		
	else if (detail == MAP_RENDER_SWORLD)
		return inst.rstats.rflags.satWorld;
		
	else if (detail == MAP_RENDER_CONSOLE)
		return inst.rstats.rflags.console;
		
	else if (detail == MAP_RENDER_VIEWPORT)
		return inst.rstats.rflags.map;
		
	else if (detail == MAP_RENDER_LOCGRAPTHIC)
		return inst.rstats.rflags.locgraphic;
		
	else if (detail == MAP_RENDER_MEASURE)
		return inst.rstats.rflags.measure;
		
	else if (detail == MAP_RENDER_STRINGS)
		return inst.rstats.rflags.strings;

	else if (detail == MAP_RENDER_AREAS)
		return inst.rstats.rflags.mapFilled;
		
	else if (detail == MAP_RENDER_AREAS_OUTLINE)
		return inst.rstats.rflags.mapOutline;
		
	else if (detail == MAP_RENDER_PATHS)
		return inst.rstats.rflags.pathFilled;
		
	else if (detail == MAP_RENDER_PATHS_LINE)
		return inst.rstats.rflags.pathLine;
		
	else if (detail == MAP_RENDER_TITLE_BOUNDARY)
		return inst.rstats.rflags.tileOutline;

	return 0;
	
	/*inst->rstats.rflags.trackSpot = 0;
	inst->rstats.rflags.trackPath = 1;
	inst->rstats.rflags.trackLine = 0;*/
}
