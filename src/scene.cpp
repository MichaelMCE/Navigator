
#include <Arduino.h>
#include "config.h"
#include "displays.h"
#include "vfont/vfont.h"
#include "gps.h"
#include "fileio.h"
#include "polyfile.h"
#include "scene.h"
#include "map.h"
#include "tiles.h"
#include "poi.h"



extern mp_coverage_t coverage;
extern application_t inst;
extern uint8_t renderBuffer[VWIDTH*VHEIGHT];




void location2Block (const vectorPt2_t *loc, int32_t *x_lon, int32_t *y_lat)
{
	*y_lat = (fabsf(coverage.region.v1.lat) - fabsf(loc->lat)) / GPS_LENGTH_LAT;
	*x_lon = (fabsf(coverage.region.v1.lon) - fabsf(loc->lon)) / GPS_LENGTH_LON;
}

static inline void block2Location (const int32_t x_lon, const int32_t y_lat, vectorPt2_t *loc)
{
	loc->lat = fabsf(coverage.region.v1.lat) - (y_lat * GPS_LENGTH_LAT);
	loc->lon = (x_lon * GPS_LENGTH_LON) - fabsf(coverage.region.v1.lon);
}

static inline void rotateZ (const float a, const float x, const float y, float *xr, float *yr)
{
	*xr = x * cosf(a) - y * sinf(a);
	*yr = x * sinf(a) + y * cosf(a);
}

#if 0
static inline float getCourse (const vectorPt2_t *vec1, const vectorPt2_t *vec2)
{
	float dlon = DEG2RAD(vec2->lon - vec1->lon);
	float lat1 = DEG2RAD(vec1->lat);
	float lat2 = DEG2RAD(vec2->lat);
	float a1 = sinf(dlon) * cosf(lat2);
	float a2 = sinf(lat1) * cosf(lat2) * cosf(dlon);
	a2 = cosf(lat1) * sinf(lat2) - a2;
	a2 = atan2f(a1, a2);

	if (a2 < 0.0f) a2 += TWO_PI;
	return RAD2DEG(a2);
}
#endif

static inline uint16_t polylineToColour (const uint8_t type)
{
	if (inst.colourScheme){
		if (type <= 0x16) return COLOUR_PAL_WHITE;			// all paths
		if (type == 0x27) return COLOUR_PAL_LIGHTGREY;		// airport runway
		return COLOUR_PAL_WHITE;
	
	}else{
		switch (type){
		case 0x01: return COLOUR_PAL_DARKERBROWN;
		case 0x02: return COLOUR_PAL_DARKBROWN;
		case 0x03: return COLOUR_PAL_BROWN;
		case 0x04: return COLOUR_PAL_BROWN;
		case 0x05: return COLOUR_PAL_GREY;
		case 0x06: return COLOUR_PAL_ORANGE;
		case 0x07: return COLOUR_PAL_LIGHTBROWN;
		case 0x08: return COLOUR_PAL_PL08;
		case 0x09: return COLOUR_PAL_PL09;
		case 0x0A: return COLOUR_PAL_DARKGREY;
		case 0x0B: return COLOUR_PAL_PL0B;
		case 0x0C: return COLOUR_PAL_LIGHTGREY;
		case 0x0D: return COLOUR_PAL_PL0D;
		case 0x0E: return COLOUR_PAL_PL0E;
		case 0x10: return COLOUR_PAL_LIGHTGREY;
		case 0x11: return COLOUR_PAL_LIGHTERGREY;
		case 0x12: return COLOUR_PAL_DARKERBROWN;
		case 0x13: return COLOUR_PAL_LIGHTBROWN;
		//case 0x14: return COLOUR_24TO16(0x55EE55);
		case 0x15: return COLOUR_PAL_DARKBLUE;
		
		case 0x16: return COLOUR_PAL_PL16;
		case 0x17: return COLOUR_PAL_GREEN;
		//case 0x18: return COLOUR_PAL_BLUE_SEA;
		case 0x19: return COLOUR_PAL_DARKBROWN;
		case 0x1A: return COLOUR_PAL_PL1A;
		//case 0x1F: return COLOUR_24TO16(0x0000BB);
		case 0x27: return COLOUR_PAL_LIGHTGREY;
		//case 0x2A: return COLOUR_PAL_BLUE;
		case 0x2B: return COLOUR_PAL_BLUE_SEA;

		//default:
		//	printf("polylineToColour: unhandled type: 0x%X\n", type);
		};

		return COLOUR_PAL_BLACK;
	}
}


static inline uint16_t polygonToColour (const uint8_t type)
{
	if (inst.colourScheme){
		if (type == 0x0B) return COLOUR_PAL_REDFUZZ;
		if (type <= 0x10) return COLOUR_PAL_LIGHTERGREY;	
		if (type == 0x17) return COLOUR_PAL_PG_17;
		if (type >= 0x14 && type <= 0x1A) return COLOUR_PAL_GREENHUE;
		if (type <= 0x20) return COLOUR_PAL_LIGHTGREY;
		if (type == 0x23) return COLOUR_PAL_GREEN;
		if (type == 0x26) return COLOUR_PAL_LIGHTERGREY;
		if (type == 0x34) return COLOUR_PAL_REDFUZZ;
		if (type >= 0x28 && type <= 0x4C) return COLOUR_PAL_BLUE_SEA;
		if (type >= 0x4E && type <= 0x53) return COLOUR_PAL_BRIGHTGREEN;

		return 0;
		

	}else{
		switch (type){
		case 0x01: return COLOUR_PAL_PG_01;   //		Large urban area (>200K) 
		case 0x02: return COLOUR_PAL_PG_02;   //		Small urban area (<200K) 
		case 0x03: return COLOUR_PAL_PG_03;   //		Rural housing area       
	                                                    
		case 0x04: return COLOUR_PAL_PG_04;   //		Military base            
		case 0x05: return COLOUR_PAL_PG_05;   //		Parking lot              
		case 0x06: return COLOUR_PAL_PG_06;   //		Parking garage           
		case 0x07: return COLOUR_PAL_PG_07;   //		Airport                  
		case 0x08: return COLOUR_PAL_PG_08;   //		Shopping center          
		case 0x09: return COLOUR_PAL_PG_09;   //		Marina                   
		case 0x0a: return COLOUR_PAL_PG_0A;   //		University/College       
		case 0x0b: return COLOUR_PAL_PG_0B;   //		Hospital                 
		case 0x0c: return COLOUR_PAL_PG_0C;   //		Industrial complex       
		case 0x0d: return COLOUR_PAL_PG_0D;   //		Reservation              
		case 0x0e: return COLOUR_PAL_PG_0E;   //		Airport runway         
	  
		case 0x0F: return COLOUR_PAL_PG_05;   //		Airport runway         
	  
		case 0x10: return COLOUR_PAL_DARKGREEN;	  //	unknown
		case 0x13: return COLOUR_PAL_PG_13;   //		Building/Man-made area   
                                                    
		case 0x14: return COLOUR_PAL_PG_14;   //		National park             
		case 0x15: return COLOUR_PAL_PG_15;   //		National park             
		case 0x16: return COLOUR_PAL_PG_16;   //		National park             
		case 0x17: return COLOUR_PAL_PG_17;     //		City park                 
                                                    
		//case 0x18: return COLOUR_PAL_GREEN;   //		Golf course               
		case 0x19: return COLOUR_PAL_PINK;   	//		Sports complex            
		case 0x1a: return COLOUR_PAL_LIGHTGREEN;   //	Cemetery                  
                                                    
		case 0x1e: return COLOUR_PAL_LIGHTGREEN;   //	State park                
		case 0x1f: return COLOUR_PAL_LIGHTGREEN;   //	State park                
		case 0x20: return COLOUR_PAL_LIGHTGREEN;   //	State park
	
		case 0x21: return COLOUR_PAL_RED;		   //	Auto Fuel
	
		case 0x23: return COLOUR_PAL_BROWN;   //	food garden
    
    	case 0x26: return COLOUR_PAL_LIGHTGREY;		//  Jail/Gaol?
                                                    
		case 0x28: return COLOUR_PAL_BLUE_SEA;	  //	Sea/Ocean                 
		case 0x29: return COLOUR_PAL_BLUE_SEA;   //		Blue-Unknown              
                                                    
		case 0x32: return COLOUR_PAL_BLUE_SEA;   //		Sea                       
                                                    
		case 0x34: return COLOUR_PAL_REDFUZZ;	 //     Emergency/Police service
                                                    
		case 0x39: return COLOUR_PAL_DARKGREY;	//		Tower
                                                    
		case 0x3b: return COLOUR_PAL_BLUE_SEA;   //		Blue-Unknown              
		case 0x3c: return COLOUR_PAL_WATER;   //		Large lake (250-600 km2)  
		case 0x3d: return COLOUR_PAL_WATER;   //		Large lake (77-250 km2)   
		case 0x3e: return COLOUR_PAL_WATER;   //		Medium lake (25-77 km2)   
		case 0x3f: return COLOUR_PAL_WATER;   //		Medium lake (11-25 km2)   
		case 0x40: return COLOUR_PAL_BLUE_SEA;   //		Small lake (0.25-11 km2)  
		case 0x41: return COLOUR_PAL_BLUE_SEA;   //		Small lake (<0.25 km2)    
		case 0x42: return COLOUR_PAL_BLUE_SEA;   //		Major lake (>3.3 tkm2)    
		case 0x43: return COLOUR_PAL_BLUE_SEA;   //		Major lake (1.1-3.3 tkm2) 
		case 0x44: return COLOUR_PAL_BLUE_SEA;   //		Large lake (0.6-1.1 tkm2) 
                                                    
		case 0x45: return COLOUR_PAL_AQUA;   //			Blue-Unknown              
		case 0x46: return COLOUR_PAL_BLUE_SEA;   //		Major river (>1 km)       
		case 0x47: return COLOUR_PAL_BLUE_SEA;   //		Large river (200 m-1 km)  
		case 0x48: return COLOUR_PAL_AQUA;   //			Medium river (40-200 m)   
		case 0x49: return COLOUR_PAL_AQUA;   //			Small river (<40 m)       
                                                    
		//case 0x4a: return 0x0000;   //				Map selection area        
		//case 0x4b: return 0x0000;   //				Map coverage area         
                                                    
		case 0x4c: return COLOUR_PAL_BLUE_SEA;   //		Intermittent water        
                                                    
		case 0x4d: return COLOUR_PAL_CYAN;   //			Glacier                   
		case 0x4e: return COLOUR_PAL_DARKGREEN;   //	Orchard/plantation        
		case 0x4f: return COLOUR_PAL_PG_4F ;   //		Scrub                     
		case 0x50: return COLOUR_PAL_DARKGREEN;   //	Forest                    
		case 0x51: return COLOUR_PAL_PG_51;   //		Wetland/swamp             
		case 0x52: return COLOUR_PAL_DARKGREEN;   //	Tundra                    
		case 0x53: return COLOUR_PAL_BROWN;   //		Sand/tidal/mud flat       
	
		//default:
			//printf("polygonToColour: 0x%X\n", type);
		};
		
		return COLOUR_PAL_BLACK;
	}
}

static inline uint32_t polylineToThickness (const uint8_t roadClass)
{
	int32_t thickness = 0;
	
	switch (roadClass){
	case 0x00: thickness = 3; break;
	case 0x01: thickness = 5; break;
	case 0x02: thickness = 4; break;
	case 0x03: thickness = 3; break;
	case 0x04: thickness = 3; break;
	case 0x05: thickness = 2; break;
	case 0x06: thickness = 2; break;
	case 0x07: thickness = 0; break;
	case 0x08: thickness = 2; break;
	case 0x09: thickness = 2; break;
	case 0x0A: thickness = 2; break;
	case 0x0B: thickness = 2; break;
	case 0x0C: thickness = 2; break;
	
	case 0x15: thickness = -5; break;	//sea border
	case 0x16: thickness = -2; break;
	
	case 0x18: thickness = 3; break;
	case 0x1F: thickness = 1; break;

	case 0x2A: thickness = 0; break;
	case 0x2B: thickness = 2; break;

	case 0x27: thickness = 1; break;
	//default:
	//	printf("polylineToThickness: 0x%X\n", roadClass);
	}

	if (!inst.colourScheme)
		thickness *= 3;

	thickness += 5;
	if (thickness < 1) thickness = 1;
	return thickness;
}


static inline float viewportGetWidth (application_t *inst)
{
	return inst->viewport.dw;
}

static inline float viewportGetHeight (application_t *inst)
{
	return inst->viewport.dh;
}

static inline vectorPt4_t *viewportGetWindow (application_t *inst)
{
	return &inst->viewport.window;
}

static inline void createViewport (application_t *inst, const vectorPt2_t *center, const float spanMeters)
{
	vectorPt4_t *window	= viewportGetWindow(inst);
	sceneMakeGPSWindow(center, spanMeters, window);

	inst->viewport.dx = fabsf(window->v2.lon - window->v1.lon);
	inst->viewport.dy = fabsf(window->v2.lat - window->v1.lat);
	inst->viewport.dw = inst->viewport.dx / (float)VWIDTH;
	inst->viewport.dh = inst->viewport.dy / (float)VHEIGHT;
}

static inline void drawMapFileBoundries (application_t *inst, const vectorPt2_t *center, const float spanMeters, const uint16_t colour)
{
	const vectorPt4_t *window = viewportGetWindow(inst);
	const float dw = viewportGetWidth(inst);
	const float dh = viewportGetHeight(inst);
	const int blockTotalLon = tilesTotalAcross()+1;
	const int blockTotalLat = tilesTotalDown()+1;

	
	vectorPt2_t loc[2];
	int x1, y1, x2, y2;


	for (int32_t x = 1; x < blockTotalLon; x++){
		block2Location(x, 0, &loc[0]);
		block2Location(x, blockTotalLat-1, &loc[1]);
			
		x1 = (((loc[0].lon - window->v1.lon)) / dw);
		y1 = (((window->v1.lat - loc[0].lat) / aspectCorrection) / dh) - aspectOffset;
		y2 = (((window->v1.lat - loc[1].lat) / aspectCorrection) / dh) - aspectOffset;
		drawLineV(x1, y1, y2, colour);
	}

	for (int32_t y = 1; y < blockTotalLat; y++){
		block2Location(0, y, &loc[0]);
		block2Location(blockTotalLon-1, y, &loc[1]);
		
		y1 = (((window->v1.lat - loc[0].lat) / aspectCorrection) / dh) - aspectOffset;
		x1 = (((loc[0].lon - window->v1.lon)) / dw);
		x2 = (((loc[1].lon - window->v1.lon)) / dw);
		drawLineH(y1, x1, x2, colour);
	}
}

void drawTiles_FileBoundries (application_t *inst, vectorPt2_t *loc, const vectorPt2_t *center, const float spanMeters)
{
	drawMapFileBoundries(inst, center, spanMeters, COLOUR_PAL_GREY);
}

static inline vectors_t *getVectors (polyline_t *polyline)
{
	return &polyline->vectors;
}

static inline vector2_t *getVector2 (const vectors_t *vectors, const uint32_t vIdx)
{
	return &vectors->list[vIdx];
}

static inline int isVectorInRegion (const vectorPt2_t *vec, const vectorPt4_t *win)
{
	return  (vec->lat <= win->v1.lat && vec->lat >= win->v2.lat) &&
			(vec->lon >= win->v1.lon && vec->lon <= win->v2.lon);
}

static inline void drawFilledObject (vectors_t *vectors, const int type, vectorPt4_t *window, const float dw, const float dh, const uint16_t colour)
{
	v16_t verts[vectors->total];
	vectorPt2_t *vec;
			
	for (uint32_t j = 0; j < vectors->total; j++){
		vec = (vectorPt2_t*)getVector2(vectors, j);
		verts[j].x = (int16_t)((vec->lon - window->v1.lon) / dw);
		verts[j].y = (int16_t)((((window->v1.lat - vec->lat) / aspectCorrection) / dh) - aspectOffset);
	}
	drawPolygon(verts, vectors->total, colour);
}

void blockDrawFills (application_t *inst, block_t *block, const vectorPt2_t *center, const float spanMeters, const int32_t pass)
{
	vectorPt4_t *window = viewportGetWindow(inst);
	const float dw = viewportGetWidth(inst);
	const float dh = viewportGetHeight(inst);
	vectorPt4_t region;
	sceneMakeGPSWindow(center, spanMeters*COVERAGE_OVERSCAN, &region);
	

#if VERTICAL_DISPLAY		// fudge for vertical displays
	float delta2 = (region.v2.lat - region.v1.lat) / 4.0f;
	region.v1.lat -= delta2;
	region.v2.lat += delta2;
#endif

	for (int i = 0; i < (int)block->total; i++){
		polyline_t *polyline = &block->list[i];
		if (!(polyline->type&0x8000)) continue;

		const uint32_t type = polyline->type&0x7FFF;

		//if (findUnknown(type)) continue;
		
#if 1
		if (pass == 1){
			if (type != 0x0C && type != 0x07 && type != 0x0A)
				continue;
		}else if (pass == 2){
			if (type != 0x05 && type != 0x06)
				continue;
		}else if (pass == 3){
			if (type != 0x16 && type != 0x03 && type != 0x04 && type != 0x17 && type != 0x1E)
				continue;
		}else if (pass == 4){
			if (type != 0x19 && type != 0x1A)
				continue;
		}else if (pass == 5){
			if (type != 0x0B && type != 0x18 && type != 0x26 && type != 0x34)
				continue;
		}else if (pass == 6){
			if (type != 0x13)
				continue;
		}else if (pass == 7){
			if (type != 0x1F && type != 0x50 && type != 0x15 && type != 0x52)	// state parks, sometimes over/around water
				continue;
		}else if (pass == 8){
			if (!(type >= 0x35 && type <= 0x49) && type != 0x51 && type != 0x32 && type != 0x33 && type != 0x08)	// water, sea, lakes
				continue;
		}else{
			if (type == 0x17 || type == 0x0C || type == 0x07 || (type >= 0x32 && type <= 0x49)
			 || type == 0x0B || type == 0x18 || type == 0x50 || type == 0x16 || type == 0x1f 
			 || type == 0x1A || type == 0x0A || type == 0x08 || type == 0x13 || type == 0x03 || type == 0x15 || type == 0x52 || type == 0x1E
			 || type == 0x04 || type == 0x19 || type == 0x05 || type == 0x06 || type == 0x26
			){
				continue;
			}else{
				//printf("pass %i, type 0x%X\n", pass, type);
				//continue;
			}
		}
#endif
		vectors_t *vectors = getVectors(polyline);
		
		for (uint32_t j = 0; j < vectors->total; ){
			const vectorPt2_t *vec = (vectorPt2_t*)getVector2(vectors, j);
			if (isVectorInRegion(vec, &region)){
				uint16_t colour = polygonToColour(type);
				drawFilledObject(vectors, type, window, dw, dh, colour);
				break;
			}
			j += 2;	// should be j++, reset if there are edge artifacts
		}
	}
}

void drawTiles_Fills (application_t *inst, vectorPt2_t *loc, const vectorPt2_t *center, const float spanMeters)
{
	int bx, by, blocksAcross, blocksDown;
	tilesGetBlockCoverage(loc, spanMeters, &bx, &by, &blocksAcross, &blocksDown);
	
	
#if VERTICAL_DISPLAY		// fudge for vertical displays
	by -= (PACK_DOWN*2);
	blocksDown += (PACK_DOWN*4);
#endif

	for (int i = by; i < by+blocksDown; i++){
		for (int j = bx; j < bx+blocksAcross; j++){
			block_t *block = tilesBlock8Get(j, i);
			if (!block) continue;

			block->lastRendered = inst->renderPassCt;

			if (spanMeters < 18000.0f){
				if (spanMeters < 15000.0f)
					blockDrawFills(inst, block, center, spanMeters, 1);
				if (spanMeters < 6000.0f)
					blockDrawFills(inst, block, center, spanMeters, 3);
				if (spanMeters < 17000.0f)
					blockDrawFills(inst, block, center, spanMeters, 4);
				if (spanMeters < 2000.0f)
					blockDrawFills(inst, block, center, spanMeters, 2);
				if (spanMeters < 5000.0f)
					blockDrawFills(inst, block, center, spanMeters, 5);

				blockDrawFills(inst, block, center, spanMeters, 0);
			}
			blockDrawFills(inst, block, center, spanMeters, 7);
				
			if (spanMeters < 18000.0f){
				if (spanMeters < 2500.0f)
					blockDrawFills(inst, block, center, spanMeters, 6);
				if (spanMeters < 5000.0f)
					blockDrawFills(inst, block, center, spanMeters, 8);
			}
		}
	}
}

static void drawSinglePath (const vectors_t *vectors, const vectorPt4_t *window, const float dw, const float dh, const float lineThickness, const uint16_t colour)
{
	vectorPt2_t *vec = (vectorPt2_t*)getVector2(vectors, 0);

	float preX = (vec->lon - window->v1.lon) / dw;
	float preY = (((window->v1.lat - vec->lat) / aspectCorrection) / dh) - aspectOffset;

	for (uint32_t j = 1; j < vectors->total; j++){
		vec = (vectorPt2_t*)getVector2(vectors, j);
		float x = (vec->lon - window->v1.lon) / dw;
		float y = (((window->v1.lat - vec->lat) / aspectCorrection) / dh) - aspectOffset;

		if (!((y >= VHEIGHT && preY >= VHEIGHT) || (y < 0 && preY < 0))){
			if (!((x >= VWIDTH && preX >= VWIDTH) || (x < 0 && preX < 0))){
				// draw shaded roads, paths, etc..
				vector2_t v1, v2, v3;
				v1.x = preX;
				v1.y = preY;
				v2.x = x;
				v2.y = y;

				if (j+1 < vectors->total){
					vec = (vectorPt2_t*)getVector2(vectors, j+1);
					v3.x = (vec->lon - window->v1.lon) / dw;
					v3.y = (((window->v1.lat - vec->lat) / aspectCorrection) / dh) - aspectOffset;
					
					drawPolyV3Filled(&v1, &v2, &v3, lineThickness, colour);
				}else{
					drawPolylineSolid(preX, preY, x, y, lineThickness, colour);
				}
			}
		}
		preX = x; preY = y;
	}
}

static inline int isTrackPointInRegion (const trackPoint_t *tp, const vectorPt4_t *win)
{
	return  (tp->location.latitude <= win->v1.lat && tp->location.latitude >= win->v2.lat) &&
			(tp->location.longitude >= win->v1.lon && tp->location.longitude <= win->v2.lon);
}

static inline float fixed_atan2 (float y, float x)
{

  const float pi = M_PI;
  const float pi_2 = M_PI_2;  

  int swap = fabs(x) < fabs(y);
  float atan_input = (swap ? x : y) / (swap ? y : x);

  // Approximate atan
  const float a1  =  0.99997726f;
  const float a3  = -0.33262347f;
  const float a5  =  0.19354346f;
  const float a7  = -0.11643287f;
  const float a9  =  0.05265332f;
  const float a11 = -0.01172120f;
  float z_sq = atan_input * atan_input;
  //float res = atan_input * (a1 + z_sq * (a3 + z_sq * (a5 + z_sq * (a7 + z_sq * (a9 + z_sq * a11)))));
  float res = atan_input * fmaf(z_sq, fmaf(z_sq, fmaf(z_sq, fmaf(z_sq, fmaf(z_sq, a11, a9), a7), a5), a3), a1);

  res = swap ? (pi_2 - res) : res;
  res = (x < 0.0) ? (pi - res) : res;

  //res = copysignf(res, y);
  union { float flVal; uint32_t nVal; } tempRes = { res };
  union { float flVal; uint32_t nVal; } tempY = { y };
  tempRes.nVal = (tempRes.nVal & 0x7fffffffu) | (tempY.nVal & 0x80000000u);

  // Store result
  return tempRes.flVal; // res

}

static inline float calcDistMf (float lat1, float lon1, float lat2, float lon2)
{
	const float R = 6378137.0;		// Earths radius
	const float pi80 = M_PI / 180.0f;
	
	lat1 *= pi80;
	lon1 *= pi80;
	lat2 *= pi80;
	lon2 *= pi80;
	float dlat = fabsf(lat2 - lat1);
	float dlon = fabsf(lon2 - lon1);
	float a = sinf(dlat / 2.0f) * sinf(dlat / 2.0f) + cosf(lat1) * cosf(lat2) * sinf(dlon /2.0f) * sinf(dlon / 2.0f);
	float c = 2.0f * fixed_atan2(sqrtf(a), sqrtf(1.0f - a));
	float d = R * c;
	return d;
}

static inline float calcDistMetersTrkPt (const trackPoint_t *pt1, const trackPoint_t *pt2)
{
	const float lat1 = pt1->location.latitude;
	const float lon1 = pt1->location.longitude;
	const float lat2 = pt2->location.latitude;
	const float lon2 = pt2->location.longitude;

	return calcDistMf(lat1, lon1, lat2, lon2);
}

float sceneCaleDistanceVecPt2 (const vectorPt2_t *pt1, const vectorPt2_t *pt2)
{
	const float lat1 = pt1->lat;
	const float lon1 = pt1->lon;
	const float lat2 = pt2->lat;
	const float lon2 = pt2->lon;

	return calcDistMf(lat1, lon1, lat2, lon2);
}


static inline void drawTrackPath (application_t *inst, trackPoint_t *points, const uint32_t total, const float lineThickness, const uint16_t colour)
{
	if (total < 5) return;

	const vectorPt4_t *window = viewportGetWindow(inst);
	const float dw = viewportGetWidth(inst);
	const float dh = viewportGetHeight(inst);
	vectorPt4_t region;

	sceneMakeGPSWindow(&inst->viewport.location, sceneGetZoom(inst), &region);

	trackPoint_t *tp = &points[0];
	float preX = (tp->location.longitude - window->v1.lon) / dw;
	float preY = (((window->v1.lat - tp->location.latitude) / aspectCorrection) / dh) - aspectOffset;

	int altCol = 0;
	uint16_t col[2];
	col[0] = colour;
	col[1] = COLOUR_PAL_GOLD;

	for (uint32_t j = 1; j < total-1; j += 3){
		trackPoint_t *tp = &points[j];

		float x = (tp->location.longitude - window->v1.lon) / dw;
		float y = (((window->v1.lat - tp->location.latitude) / aspectCorrection) / dh) - aspectOffset;

		if (isTrackPointInRegion(tp, &region)){
			if (!((y >= VHEIGHT && preY >= VHEIGHT) || (y < 0 && preY < 0))){
				if (!((x >= VWIDTH && preX >= VWIDTH) || (x < 0 && preX < 0))){
					const float distance = calcDistMetersTrkPt(tp, &points[j-1]);
					if (distance >= 0.20f){
						if (distance < 11.0f)
							drawPolylineSolid(preX, preY, x, y, lineThickness, col[altCol]);
					}else{
						x = preX;
						y = preY;
					}
				}
			}
		}
		altCol++;
		altCol &= 0x01;
		
		preX = x;
		preY = y;
	}
}

static inline void drawPixel8 (const int x, const int y, const uint8_t colour)
{
	uint8_t *pixels = (uint8_t*)renderBuffer;	
	pixels[(y*VWIDTH)+x] = colour;
}

static inline void drawCircleFilled2 (const int x0, const int y0, const int radius, const uint8_t colour)
{
/*	const int radiusMul = radius*radius + radius;
	
	for (int y = -radius; y <= radius; y++)
	    for (int x = -radius; x <= radius; x++)
        	if (x*x+y*y < radiusMul)
	            drawPixel8(x0+x, y0+y, colour);
*/

	const int radiusMul = radius*radius;
	
	for (int x = -radius; x < radius ; x++)
	{
	    int height = (int)sqrtf(radiusMul - x * x);
	
		int X0 = x + x0;
    	for (int y = -height; y < height; y++)
        	drawPixel8(X0, y + y0, colour);
	}
}

static inline void drawTrackSpot (application_t *inst, trackPoint_t *points, const uint32_t total, const float radius, const uint16_t colour)
{
	const vectorPt4_t *window = viewportGetWindow(inst);
	const float dw = viewportGetWidth(inst);
	const float dh = viewportGetHeight(inst);
	vectorPt4_t region;

	sceneMakeGPSWindow(&inst->viewport.location, sceneGetZoom(inst)*COVERAGE_OVERSCAN, &region);

	int altCol = 0;
	uint8_t col[2];
	col[0] = colour;
	col[1] = COLOUR_PAL_GOLD;

	for (uint32_t j = 1; j < total; j += 1){
		trackPoint_t *tp = &points[j];

		if (isTrackPointInRegion(tp, &region)){
			float x = (tp->location.longitude - window->v1.lon) / dw;
			float y = (((window->v1.lat - tp->location.latitude) / aspectCorrection) / dh) - aspectOffset;

			if (!((y - radius < 0) || (y + radius >= VHEIGHT))){
				if (!((x - radius < 0) || (x + radius >= VWIDTH))){
					drawCircleFilled2(x, y, radius, col[altCol++]);
					//altCol++;
					altCol &= 0x01;
				}
			}
		}
	}
}

static inline void drawTrackPath_Line (application_t *inst, trackPoint_t *points, const uint32_t total, const uint16_t colour)
{
	if (total < 5) return;

	const vectorPt4_t *window = viewportGetWindow(inst);
	const float dw = viewportGetWidth(inst);
	const float dh = viewportGetHeight(inst);

	trackPoint_t *tp = &points[2];

	float preX = (tp->location.longitude - window->v1.lon) / dw;
	float preY = (((window->v1.lat - tp->location.latitude) / aspectCorrection) / dh) - aspectOffset;

	for (uint32_t j = 3; j < total-1; j += 1){
		trackPoint_t *tp = &points[j];

		float x = (tp->location.longitude - window->v1.lon) / dw;
		float y = (((window->v1.lat - tp->location.latitude) / aspectCorrection) / dh) - aspectOffset;

		if (!((y >= VHEIGHT && preY >= VHEIGHT) || (y < 0 && preY < 0))){
			if (!((x >= VWIDTH && preX >= VWIDTH) || (x < 0 && preX < 0))){
				drawPolyline(preX, preY, x, y, colour);
			}
		}
		preX = x;
		preY = y;
	}
}

static void drawSinglePath_Line (const vectors_t *vectors, const vectorPt4_t *window, const float dw, const float dh, const uint16_t colour)
{
	vectorPt2_t *vec = (vectorPt2_t*)getVector2(vectors, 0);
			
	float preX = (vec->lon - window->v1.lon) / dw;
	float preY = (((window->v1.lat - vec->lat) / aspectCorrection) / dh) - aspectOffset;

	for (uint32_t j = 1; j < vectors->total; j++){
		vec = (vectorPt2_t*)getVector2(vectors, j);
		float x = (vec->lon - window->v1.lon) / dw;
		float y = (((window->v1.lat - vec->lat) / aspectCorrection) / dh) - aspectOffset;

		if (!((y >= VHEIGHT && preY >= VHEIGHT) || (y < 0 && preY < 0))){
			if (!((x >= VWIDTH && preX >= VWIDTH) || (x < 0 && preX < 0))){
				// draw shaded roads, paths, etc..
				drawPolyline(preX, preY, x, y, colour);
			}
		}
		preX = x; preY = y;
	}
}

void blockDrawPaths (application_t *inst, block_t *block, const vectorPt2_t *center, const float spanMeters, const int32_t pass, const int drawPolyLine)
{
	
	const vectorPt4_t *window = viewportGetWindow(inst);
	const float dw = viewportGetWidth(inst);
	const float dh = viewportGetHeight(inst);
	vectorPt4_t region;
	sceneMakeGPSWindow(center, spanMeters*COVERAGE_OVERSCAN, &region);

	for (int i = 0; i < (int)block->total; i++){
		polyline_t *polyline = &block->list[i];
		if (polyline->type&0x8000) continue;
		const int type = polyline->type&0x7FFF;

		if (pass){
			if (type != pass)
				continue;
		}else if (type <= 0x0C || /*type == 0x18 || type == 0x1F ||*/ /*type == 0x15 ||*/ type == 0x16){		// roads, stream and river
			continue;
		}
		
		vectors_t *vectors = getVectors(polyline);
		if (!vectors) continue;

		if (!drawPolyLine){
			const uint16_t colour = polylineToColour(polyline->type);
			
			for (uint32_t j = 0; j < vectors->total; ){
				const vectorPt2_t *vec = (vectorPt2_t*)getVector2(vectors, j);
				if (isVectorInRegion(vec, &region)){
					float lineThickness = polylineToThickness(polyline->type);
					drawSinglePath(vectors, window, dw, dh, lineThickness, colour);
					break;
				}
				j += 2;	// should be j++, reset if there are edge artifacts or poping paths
			}
		}else{
			for (uint32_t j = 0; j < vectors->total; ){
				const vectorPt2_t *vec = (vectorPt2_t*)getVector2(vectors, j);
				if (isVectorInRegion(vec, &region)){
					//float lineThickness = polylineToThickness(polyline->type);
					//drawSinglePath_Line(vectors, window, dw, dh, polylineToColour(polyline->type));
					drawSinglePath_Line(vectors, window, dw, dh, COLOUR_PAL_BLACK);
					break;
				}
				j += 2;	// should be j++, reset if there are edge artifacts or poping paths
			}			
		}
	}
}


void drawTiles_Paths (application_t *inst, vectorPt2_t *loc, const vectorPt2_t *center, const float spanMeters, const int drawPolyLine)
{
	int bx, by, blocksAcross, blocksDown;
	tilesGetBlockCoverage(loc, spanMeters, &bx, &by, &blocksAcross, &blocksDown);


#if VERTICAL_DISPLAY		// fudge for vertical displays
	by -= (PACK_DOWN*1);
	blocksDown += (PACK_DOWN*2);
#endif

	
	for (int i = by; i < by+blocksDown; i++){
		for (int j = bx; j < bx+blocksAcross; j++){
			block_t *block = tilesBlock8Get(j, i);
			if (!block) continue;

			block->lastRendered = inst->renderPassCt;

			if (spanMeters < 2000.0f)
				blockDrawPaths(inst, block, center, spanMeters, 0x16, drawPolyLine);
			if (spanMeters < 6000.0f)
				blockDrawPaths(inst, block, center, spanMeters, 0x0B, drawPolyLine);
			if (spanMeters < 5000.0f)
				blockDrawPaths(inst, block, center, spanMeters, 0x0A, drawPolyLine);

			if (spanMeters < 16000.0f)
				blockDrawPaths(inst, block, center, spanMeters, 0x09, drawPolyLine);

			if (spanMeters < 4000.0f)
				blockDrawPaths(inst, block, center, spanMeters, 0x08, drawPolyLine);
			if (spanMeters < 2500.0f)
				blockDrawPaths(inst, block, center, spanMeters, 0x06, drawPolyLine);
			if (spanMeters < 2000.0f)
				blockDrawPaths(inst, block, center, spanMeters, 0x07, drawPolyLine);
			if (spanMeters < 15000.0f)
				blockDrawPaths(inst, block, center, spanMeters, 0x05, drawPolyLine);
			if (spanMeters < 35000.0f)
				blockDrawPaths(inst, block, center, spanMeters, 0x04, drawPolyLine);

			if (spanMeters < 50000.0f)
				blockDrawPaths(inst, block, center, spanMeters, 0x03, drawPolyLine);
	
			blockDrawPaths(inst, block, center, spanMeters, 0x02, drawPolyLine);
			blockDrawPaths(inst, block, center, spanMeters, 0x01, drawPolyLine);
			

			if (spanMeters < 10000.0f)
				blockDrawPaths(inst, block, center, spanMeters, 0x0C, drawPolyLine);
			if (spanMeters < 18000.0f)
				blockDrawPaths(inst, block, center, spanMeters, 0x00, drawPolyLine);
		}
	}
}

static void blockDrawFillOutline (application_t *inst, block_t *block, const vectorPt2_t *center, const float spanMeters)
{
	vectorPt4_t *window = viewportGetWindow(inst);
	const float dw = viewportGetWidth(inst);
	const float dh = viewportGetHeight(inst);
	vectorPt4_t region;
	sceneMakeGPSWindow(center, spanMeters*COVERAGE_OVERSCAN, &region);


#if VERTICAL_DISPLAY		// fudge for vertical displays
	float delta2 = (region.v2.lat - region.v1.lat) / 4.0f;
	region.v1.lat -= delta2;
	region.v2.lat += delta2;
#endif

	const uint16_t colour = COLOUR_PAL_BLACK;

	for (int i = 0; i < (int)block->total; i++){
		polyline_t *polyline = &block->list[i];
		if (!(polyline->type&0x8000)) continue;
		
		int drawVectors = 0;
		vectors_t *vectors = getVectors(polyline);

		for (uint32_t j = 0; j < vectors->total; j++){
			const vectorPt2_t *vec = (vectorPt2_t*)getVector2(vectors, j);
			if (isVectorInRegion(vec, &region)){
				drawVectors = 1;
				break;
			}
		}
		if (!drawVectors) continue;
		
		v16_t verts[vectors->total];
		vectorPt2_t *vec;
		
		for (uint32_t j = 0; j < vectors->total; j++){
			vec = (vectorPt2_t*)getVector2(vectors, j);
			verts[j].x = (int16_t)((vec->lon - window->v1.lon) / dw);
			verts[j].y = (int16_t)((((window->v1.lat - vec->lat) / aspectCorrection) / dh) - aspectOffset);
		}

		for (uint32_t j = 0; j < (uint32_t)vectors->total-1; j++)
			drawPolyline(verts[j].x, verts[j].y, verts[j+1].x, verts[j+1].y, colour);
		drawPolyline(verts[0].x, verts[0].y, verts[vectors->total-1].x, verts[vectors->total-1].y, colour);
	}
}

void drawTiles_Outlines (application_t *inst, vectorPt2_t *loc, const vectorPt2_t *center, const float spanMeters)
{
	int bx, by, blocksAcross, blocksDown;
	tilesGetBlockCoverage(loc, spanMeters, &bx, &by, &blocksAcross, &blocksDown);
	
	for (int i = by; i < by+blocksDown; i++){
		for (int j = bx; j < bx+blocksAcross; j++){
			block_t *block = tilesBlock8Get(j, i);
			if (!block) continue;

			block->lastRendered = inst->renderPassCt;
			blockDrawFillOutline(inst, block, center, spanMeters);		
		}
	}
}

static void inline drawTiles (application_t *inst, const vectorPt2_t *center, const float spanMeters, const int drawLayer)
{
	vectorPt2_t loc = sceneGetLocation(inst);

	switch (drawLayer){
	  case DRAWLAYER_POLYGON: 
	  	drawTiles_Fills(inst, &loc, center, spanMeters);
	  	break;
	  case DRAWLAYER_POLYGON_OUTLINE: 
	  	drawTiles_Outlines(inst, &loc, center, spanMeters);
	  	break;
	  case DRAWLAYER_PATH: 
	  	drawTiles_Paths(inst, &loc, center, spanMeters, 0);
	  	break;
	  case DRAWLAYER_PATH_LINE: 
	  	drawTiles_Paths(inst, &loc, center, spanMeters, 1);
	  	break;
	  case DRAWLAYER_TILE_BOUNDRY:
		drawTiles_FileBoundries(inst, &loc, center, spanMeters);
		break;
	};
}

static inline void drawCompass (application_t *inst, float course)
{
	course = 360.0f - course;
	float x = VWIDTH-80.0, y = VHEIGHT-80;
	float x1, y1;
	float x2, y2;

	const uint16_t colour = COLOUR_PAL_GREY;

	for (float c = 0.0f; c < 10.0f; c += 1.0f)
		drawCircle(x, y, 30.0f + (11.0f*M_PI) + c, colour);

	for (float a = 45.0f; a < 360.0f+45.0f; a += 90.0f){
		rotateZ(DEG2RAD(a+course), 30.0f, 40.0f, &x1, &y1);
		rotateZ(DEG2RAD(a+course), 40.0f, 50.0f, &x2, &y2);
		drawPolylineSolid(x+x1, y+y1, x+x2, y+y2, 4, colour);
	}

	for (float a = 0.0f; a < 360.0f; a += 90.0f){
		rotateZ(DEG2RAD(a+course), 35.0f, 40.0f, &x1, &y1);
		rotateZ(DEG2RAD(a+course), 40.0f, 45.0f, &x2, &y2);
		drawPolylineSolid(x+x1, y+y1, x+x2, y+y2, 8, colour);
	}
	
	
	int tsize = 15;
	drawTriangleFilled(x+tsize, y+tsize, x-tsize, y+tsize, x, y-(tsize*2), colour);
	
	vfont_t *ctx = inst->vfont;
	float angle = fmodf(course-90.0f, 360.0f);

	setRotationAngle(ctx, -angle, angle);
	setRenderFilter(ctx, RENDEROP_ROTATE_GLYPHS|RENDEROP_ROTATE_STRING);
	setBrushSize(ctx, 2.0f);
	setGlyphScale(ctx, 0.7f);
	setBrushStep(ctx, 1.0f);
	
	int dx = 0;
	rotateZ(DEG2RAD(course), 5.0f, -30.0f, &x1, &y1);
	drawString(ctx, "N", (x+x1)-dx, y+y1);
	
	rotateZ(DEG2RAD(course), -5.0f, 45.0f, &x1, &y1);
	drawString(ctx, "S", (x+x1)-dx, y+y1);
	
	rotateZ(DEG2RAD(course), 40.0f, 12.0f, &x1, &y1);
	drawString(ctx, "E", (x+x1)-dx, y+y1);
	
	rotateZ(DEG2RAD(course), -37.0f, 2.0f, &x1, &y1);
	drawString(ctx, "W", (x+x1)-dx, y+y1);
	
	setRenderFilter(ctx, RENDEROP_NONE);
}

static inline void drawMapCenter (application_t *inst, const vectorPt2_t *center, const float course, const float size)
{
	const vectorPt4_t *window = viewportGetWindow(inst);
	const float dw = viewportGetWidth(inst);
	const float dh = viewportGetHeight(inst);
	
	float x1, y1;
	rotateZ(DEG2RAD(course), 0.0f, size, &x1, &y1);
	float x2 = y1;
	float y2 = -x1;
	
	float x = (center->lon - window->v1.lon) / dw;
	float y = (((window->v1.lat - center->lat) / aspectCorrection) / dh) - aspectOffset;
	
#if 1
	//drawPolylineSolid(x-x2, y-y2, x+x2, y+y2, 5, COLOUR_PAL_RED);
	//drawTriangle(x+x2, y+y2, x+x1, y+y1, x-x2, y-y2, COLOUR_PAL_RED);

	drawTriangleFilled(x-x2+x1+x1, y-y2+y1+y1, x-x1, y-y1, x+x2+x1+x1, y+y2+y1+y1, COLOUR_PAL_RED);
	drawTriangle(x-x2+x1+x1, y-y2+y1+y1, x-x1, y-y1, x+x2+x1+x1, y+y2+y1+y1, COLOUR_PAL_PLYEDGE);

	drawTriangleFilled(x-x2+x1, y-y2+y1, x-x1, y-y1, x+x2+x1, y+y2+y1, COLOUR_PAL_RED);
	drawTriangle(x-x2+x1, y-y2+y1, x-x1, y-y1, x+x2+x1, y+y2+y1, COLOUR_PAL_PLYEDGE);
	drawCircleFilled (x, y, 5.0f, COLOUR_PAL_WHITE);
#else

	drawPolylineSolid(x-x1, y-y1, x+x1, y+y1, 12, COLOUR_PAL_PLYEDGE);
	drawPolylineSolid(x-x1, y-y1, x+x1, y+y1, 8, COLOUR_PAL_WHITE);
	drawCircleFilled (x, y, 5.0f, COLOUR_PAL_BLUE);
#endif


}

static inline void overlayRender (application_t *inst)
{
#if 0
	vfont_t *ctx = inst->vfont;
	vectorPt2_t loc = sceneGetLocation(inst);

	char text[2][16];
	snprintf((char*)text[0], sizeof(text[0]), "%f", loc.lat);
	snprintf((char*)text[1], sizeof(text[1]), "%f", loc.lon);
	
	int x = 10;
	int y = 50;

	setBrushStep(inst->vfont, 1.0f);
	setBrushSize(ctx, 1.0f);
	setGlyphScale(ctx, 1.0f);
	setBrushColour(ctx, COLOUR_PAL_BLACK);
	
	drawString(ctx, text[0], x, y);
	drawString(ctx, text[1], x, y + 80);

	float zoom = sceneGetZoom(inst);
	snprintf((char*)text[0], sizeof(text[0]), "%im", (int)zoom);
	drawString(ctx, text[0], VWIDTH-100, y + 40);

#if 0
	int32_t polyTotal = blocksCountVts(&inst->blocks);
	snprintf(text[0], sizeof(text[0]), "%i", polyTotal);
	drawString(ctx, text[0], VWIDTH-220, y + 80);
#endif
#endif
}

#if 1
static inline void drawPOI (application_t *inst, poi_t *poi, vfont_t *vctx, const vectorPt2_t *center, const float spanMeters)
{
	const vectorPt4_t *window = viewportGetWindow(inst);
	const float dw = viewportGetWidth(inst);
	const float dh = viewportGetHeight(inst);

	vectorPt4_t region;
	sceneMakeGPSWindow(center, spanMeters*1.5f, &region);
	
	setGlyphScale(vctx, 1.2f);
	
	for (uint16_t i = 0; i < POI_MAX_BLOCKS; i++){
		poi_file_t *blk = &poi->blocks[i];
		if (blk->x == 0xFFFF) continue;
		
		for (uint16_t j = 0; j < POI_MAX_STRING; j++){
			if (blk->poi[j].strIdx == 0xFFFF) continue;
			vectorPt2_t vec = blk->poi[j].vec;
			
			if (isVectorInRegion(&vec, &region)){
				const char *str = (char*)poiGetString(poi, blk->poi[j].strIdx);
				if (str){
					box_t box;
					getStringMetrics(vctx, str, &box);

					float w = ((box.x2 - box.x1)+1) / 2.0f;
					//float h = ((box.y2 - box.y1)+1) / 2.0f;

					int y = ((((window->v1.lat - vec.lat) / aspectCorrection) / dh) - aspectOffset);					
					int x = ((vec.lon - window->v1.lon) / dw);
					//drawCircle(x, y, 4.0f, COLOUR_PAL_BLUE);
					x -= w;
					
					
					setBrushColour(vctx, COLOUR_PAL_REDFUZZ);
					setBrushQuality(vctx, 1);
					setBrushStep(vctx, 2.0f);
					setBrushSize(vctx, 10.5f);
					drawString(vctx, str, x, y);
					
					setBrushColour(vctx, COLOUR_PAL_WHITE);
					//setBrushQuality(vctx, 1);
					setBrushStep(vctx, 1.0f);
					setBrushSize(vctx, 2.0f);
					drawString(vctx, str, x, y);
				}
			}
		}
	}
}
#endif

static inline void renderFrame (application_t *inst, const vectorPt2_t *center)
{
	drawTiles(inst, center, sceneGetZoom(inst), DRAWLAYER_POLYGON);				// filled areas
	drawTiles(inst, center, sceneGetZoom(inst), DRAWLAYER_POLYGON_OUTLINE);		// polygon outlines	
	drawTiles(inst, center, sceneGetZoom(inst), DRAWLAYER_PATH);				// paths
	//drawTiles(inst, center, sceneGetZoom(inst), DRAWLAYER_PATH_LINE);			// single pixel width poly paths
	//drawTiles(inst, center, sceneGetZoom(inst), DRAWLAYER_TILE_BOUNDRY);		// tile boundries
}

static inline void sceneRender (application_t *inst)
{
	inst->renderPassCt++;
	renderFrame(inst, &inst->viewport.location);
}

void sceneMakeGPSWindow (const vectorPt2_t *center, const double spanMeters, vectorPt4_t *out)
{
	tilesMakeGPSWindow(center, spanMeters, out);
}

void sceneRenderViewport (application_t *inst)
{
	sceneRender(inst);
}

void sceneRenderCompass (application_t *inst)
{
	drawCompass(inst, inst->viewport.heading);
}

void sceneRenderPOI (application_t *inst)
{
	drawPOI(inst, &inst->poi, inst->vfont, &inst->viewport.location, inst->viewport.zoom);
}

void sceneRenderOverlay (application_t *inst)
{
	overlayRender(inst);
}

void sceneRenderTrackPoints (application_t *inst, trackRecord_t *trackRecord)
{
	//const int total = trackRecord->marker;
	
	//drawTrackPath(inst, &trackRecord->trackPoints[trackRecord->marker - total], total, 6, COLOUR_PAL_AQUA);
	//drawTrackSpot(inst, trackRecord->trackPoints, trackRecord->marker, inst->scheme.spotRadius, COLOUR_PAL_AQUA);
	drawTrackPath(inst, trackRecord->trackPoints, trackRecord->marker, inst->scheme.pathThickness, COLOUR_PAL_AQUA);
	//drawTrackPath_Line(inst, trackRecord->trackPoints, trackRecord->marker, COLOUR_PAL_DARKGREY);
}

void sceneRenderLocGraphic (application_t *inst)
{
	drawMapCenter(inst, &inst->viewport.location, inst->viewport.heading, 15.0f);
}

void sceneLocation2Tile (const vectorPt2_t *loc, int32_t *x_lon, int32_t *y_lat)
{
	location2Block(loc, x_lon, y_lat);
}

void sceneClose ()
{
	tilesClose();
}

FLASHMEM void sceneInit ()
{
	tilesInit();
}

uint32_t sceneGetSize (application_t *inst)
{
	return 0;
}

void sceneSetZoom (application_t *inst, const float zoomMeters)
{
	inst->viewport.zoom = zoomMeters;
}

float sceneGetZoom (application_t *inst)
{
	return inst->viewport.zoom;
}

void sceneSetHeading (application_t *inst, const float heading)
{
	inst->viewport.heading = heading;
}

void sceneSetLocation (application_t *inst, const vectorPt2_t *loc)
{
	inst->viewport.location = *loc;
	createViewport(inst, loc, inst->viewport.zoom);
}

void sceneResetViewport (application_t *inst)
{
	createViewport(inst, &inst->viewport.location, inst->viewport.zoom);
}

vectorPt2_t sceneGetLocation (application_t *inst)
{
	return inst->viewport.location;
}

float sceneGetLocationLat (application_t *inst)
{
	return inst->viewport.location.lat;
}

float sceneGetLocationLon (application_t *inst)
{
	return inst->viewport.location.lon;
}

void sceneFlushTiles (application_t *inst)	
{
	// release old tiles here
}

void sceneSetColourScheme (const int colourScheme)
{
	inst.colourScheme = colourScheme;
}
