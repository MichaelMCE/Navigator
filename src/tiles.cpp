

#include "commonGlue.h"




static tile8_t ***tiles8;



// 	get source map data for your region from https://extract.bbbike.org/ 
//  convert to .mp with GPSMapEdit

#if 0
//  original			155meg
mp_coverage_t coverage = {		// MAP_SOURCE coverage, Standard/gmapsupp.mp
		{{55.25573,  -8.188934},
		 {54.022522, -5.413513}},
		2.775421/*2.77542*/,	// degrees width
		1.233208/*1.23322*/,	// degrees height
		0, 0
};
#else
// newest 
// 600mb, June 2025
mp_coverage_t coverage = {
		{{55.3985791, -8.8708173},
		 {53.9454972, -5.3821777}},
		3.4886396,		// 3.48816° width
		1.4530819,		// 1.45294° height
		0, 0
};
#endif

static void fileAdvance (fileio_t *file, const size_t amount)
{
	fio_advance(file, amount);
}

void polyfileAdvance (fileio_t *file, const size_t amount)
{
	fileAdvance(file, amount);
}

static size_t polyfileRead (fileio_t *file, void *buffer, const size_t len)
{
	return fio_read(file, buffer, len);
}

static int polyfileSeek (fileio_t *file, const size_t pos)
{
	return fio_seek(file, pos);
}

static void polyfileClose (fileio_t *file)
{
	fio_close(file);
}

static fileio_t *polyfileOpen (const uint8_t *dir, const int32_t x_lon, const int32_t y_lat, size_t *polyLen)
{
	const int xm = (x_lon % PACK_ACROSS);
	const int ym = (y_lat % PACK_DOWN);
	const int x = x_lon - xm;
	const int y = y_lat - ym;


	//printf("\r\n");

	*polyLen = 0;
	uint8_t filename[1024];
	snprintf((char*)filename, sizeof(filename)-1, "%s/%03i_%03i.pk32", dir, y, x);

	//printf(CS("polyfileOpen: %i %i: %s"), y, x, filename);

	fileio_t *file = fio_open(filename, FIO_READ);
	if (!file){
		printf(CS("polyfileOpen(): open failed: %s"), filename);
		return NULL;
	}else{
		//printf("polyfileOpen(): ready for '%s'\r\n", filename);
	}
	
	//printf("poly_pack_header_t size %i\r\n", (int)sizeof(poly_pack_header_t));

	// poly files are stored as 4x4 for pk16 and 8x8 for pk32 files, 1 pk16 covers and area of 2km*2km
	size_t pos = (ym * PACK_ACROSS * sizeof(poly_pack_file_t)) + (xm * sizeof(poly_pack_file_t));
	if (!polyfileSeek(file, pos)){
		//printf("polyfileOpen(): '%s' can not seek to %i\r\n", filename, (int)pos);
		polyfileClose(file);
		return NULL;
	}
	
	poly_pack_file_t *poly = (poly_pack_file_t*)filename;	// sharing the buffer, saving a few bytes of uc stack/ram
	if (polyfileRead(file, poly, sizeof(poly_pack_file_t)) != 1){
		//printf("polyfileOpen(): can not read\r\n");
		polyfileClose(file);
		return NULL;
	}	

	if (!poly->offset || !poly->length){
		//printf("polyfileOpen(): zero length %i %i\r\n", (int)poly->offset, (int)poly->length);
		polyfileClose(file);
		return NULL;
	}
	
	if (!polyfileSeek(file, poly->offset)){
		//printf("polyfileOpen(): can not seek 2 %i\r\n", (int)poly->offset);
		polyfileClose(file);
		return NULL;
	}

	*polyLen = poly->length;
	return file;
}

static inline block_t *block8Get (const int x_lon, const int y_lat)
{	
	if (x_lon < 0 || (x_lon >= (int)tilesTotalAcross()) || y_lat < 0 || (y_lat >= (int)tilesTotalDown()))
		return NULL;
		
	if (!tiles8[(y_lat & ~PACK_MASK)/PACK_DOWN])
		return NULL;

	tile8_t *tile = tiles8[(y_lat & ~PACK_MASK)/PACK_DOWN][(x_lon & ~PACK_MASK)/PACK_ACROSS];
	if (tile){
		int y = y_lat & PACK_MASK;
		int x = x_lon & PACK_MASK;
		if (tile->block && tile->block[y] && tile->block[y][x])
			return tile->block[y][x];
	}	
	
	return NULL;
}

static void tileInit (block_t *block, const int tPolySize)
{
	block->total = 0;
	block->size = tPolySize;
	block->list = (polyline_t*)l_malloc(block->size * sizeof(polyline_t));
	block->lastRendered = 0;
}

static inline void vectorsAlloc (const uint32_t total, vectors_t *vectors)
{
	vectors->total = total;
	vectors->list = (vector2_t*)l_malloc(total * sizeof(vector2_t));
}

static inline void blockRelease (block_t *block)
{
	if (block->list){
		for (int j = 0; j < (int)block->total; j++)
			l_free(block->list[j].vectors.list);
		l_free(block->list);
	}
}

int blockLoad (block_t *block, const uint8_t *dir, const int32_t y_lat, const int32_t x_lon)
{
	poly_field_t field;
	vectors_t vectors;
	size_t lengthRead = 0;
	size_t polyLen = 0;


	fileio_t *file = polyfileOpen(dir, x_lon, y_lat, &polyLen);
	if (!file){
		//printf("could not open %i %i from '%s\\'\n", x_lon, y_lat, dir);
		return 0;
	};


	tileInit(block, 64);

	while (lengthRead < polyLen){
		int ret = polyfileRead(file, &field, sizeof(field));
		if (ret != 1) break;
		lengthRead += sizeof(field);

		vectorsAlloc(field.total, &vectors);
		
		if (polyfileRead(file, vectors.list, sizeof(vector2_t)*field.total) == 1){
			lengthRead += sizeof(vector2_t) * field.total;
			//printf("polyfileRead %i\n", sizeof(vector2_t) * field.total);

			if (block->total == block->size){
				//printf("block->size %i\n", block->size);
				block->size += 32;
				void *newList = (void*)l_realloc(block->list, block->size * sizeof(polyline_t));
				if (!newList) break;
				block->list = (polyline_t*)newList;
			}

			polyline_t *polyline = &block->list[block->total];
			polyline->type = field.type;
			polyline->vectors = vectors;
			block->total++;
		}else{
			printf(CS("blockLoad() failed: %i, %i %i: %s"), (int)(sizeof(vector2_t)*field.total), (int)y_lat, (int)x_lon, dir);
		}
	};
	polyfileClose(file);
	
	if (!block->total) blockRelease(block);
	return block->total;
}

static inline uint32_t calcTilesTotalAcross ()
{
	return ceilf(coverage.width / GPS_LENGTH_LON);
}

static inline uint32_t calcTilesTotalDown ()
{
	return ceilf(coverage.height / GPS_LENGTH_LAT);
}

static inline void calcTileCoverage ()
{
	coverage.tilesAcross = calcTilesTotalAcross();
	coverage.tilesDown = calcTilesTotalDown();
}
	
uint32_t tilesTotalAcross ()
{
	return coverage.tilesAcross;
}

uint32_t tilesTotalDown ()
{
	return coverage.tilesDown;
}

static inline tile8_t *tile8Get (const int y_lat, const int x_lon)
{
	if (x_lon < 0 || (x_lon >= (int)tilesTotalAcross()) || y_lat < 0 || (y_lat >= (int)tilesTotalDown()))
		return NULL;
		
	int tileY = (y_lat & ~PACK_MASK) / PACK_DOWN;
	int tileX = (x_lon & ~PACK_MASK) / PACK_ACROSS;
	
	if (!tiles8[tileY]) return NULL;
	return tiles8[tileY][tileX];
}

static inline void tile8Set (const int y_lat, const int x_lon, tile8_t *tile)
{
	int tileY = (y_lat & ~PACK_MASK) / PACK_DOWN;
	int tileX = (x_lon & ~PACK_MASK) / PACK_ACROSS;
	
	tiles8[tileY][tileX] = tile;
}

static inline int tileUnload (const uint32_t renderPass, const int32_t y_lat, const int32_t x_lon)
{
	
	//printf("tileUnload %i %i\r\n", (int)y_lat, (int)x_lon);
	
	int ct = 0;
	
	tile8_t *tile = tile8Get(y_lat, x_lon);
	if (tile){
		block_t ***blocks = tile->block;
		if (!blocks) return 0;
		int freeTile = 1;
		
		for (int y = 0; y < PACK_DOWN; y++){
			if (!blocks[y]) continue;

			for (int x = 0; x < PACK_ACROSS; x++){
				if (blocks[y][x]){
					block_t *block = blocks[y][x];
					int rDiff = (renderPass - block->lastRendered);
					if (rDiff > TILE_UNLOAD_DELTA){
						
						//printf(CS("tileUnload, blockRelease: %i %i"), (int)x, (int)y);
						
						blockRelease(block);
						l_free(block);
						blocks[y][x] = NULL;
						ct++;
					}else{
						freeTile = 0;
					}
				}
			}
		}

		if (freeTile){
			if (blocks){
				for (int j = 0; j < PACK_DOWN; j++)
					if (blocks[j]) l_free(blocks[j]);
				l_free(blocks);
			}
			l_free(tile);
			tile8Set(y_lat, x_lon, NULL);
			return ct;
		}
	}
	return 0;
}

int tilesUnload (const uint32_t renderPass)
{
	int ct = 0;
	const int blockTotalLon = tilesTotalAcross();
	const int blockTotalLat = tilesTotalDown();
	
	for (int i = 0; i < blockTotalLat; i++){
		for (int j = 0; j < blockTotalLon; j++){
			ct += tileUnload(renderPass, i, j);
		}
	}
	return ct;
}

int tilesUnloadAll (application_t *inst)
{
	int ct = 0;
	
	const uint32_t renderPass = inst->renderPassCt + TILE_UNLOAD_DELTA + 1;
	const int blockTotalLon = tilesTotalAcross();
	const int blockTotalLat = tilesTotalDown();
	
	for (int i = 0; i < blockTotalLat; i++){
		for (int j = 0; j < blockTotalLon; j++){
			ct += tileUnload(renderPass, i, j);
		}
	}

	const int yTotal = (tilesTotalDown()/PACK_DOWN)+1;
	for (int i = 0; i < yTotal; i++){
		if (tiles8[i]){
			l_free(tiles8[i]);
			tiles8[i] = NULL;
		}
	}
	return ct;
}

static inline void makeGPSWindow (const vectorPt2_t *center, const double spanMeters, vectorPt4_t *out)
{
	double m = (spanMeters/1000.0);
	out->v1.lon = center->lon - (m * GPS_1000M_LON);		// top left
	out->v1.lat = center->lat + (m * GPS_1000M_LAT);
	
	out->v2.lon = center->lon + (m * GPS_1000M_LON);		// bottom right
	out->v2.lat = center->lat - (m * GPS_1000M_LAT);
}

static inline void getBlockCoverage (vectorPt2_t *center, const float spanMeters, int *x_lon, int *y_lat, int *blocksAcross, int *blocksDown)
{
	vectorPt4_t window;
	makeGPSWindow(center, spanMeters, &window);

	float lat1 = floorf((coverage.region.v1.lat - window.v1.lat) / (float)GPS_LENGTH_LAT);
	float lon1 = floorf((window.v1.lon - coverage.region.v1.lon) / (float)GPS_LENGTH_LON);
	float lat2 = ceilf((coverage.region.v1.lat - window.v2.lat) / (float)GPS_LENGTH_LAT);
	float lon2 = ceilf((window.v2.lon - coverage.region.v1.lon) / (float)GPS_LENGTH_LON);

	*blocksAcross = floorf((lon2-lon1) + 0.5f);
	*blocksDown = ceilf(((lat2-lat1))*(aspectCorrection + 0.25f));
	*y_lat = lat1;
	*x_lon = lon1;
}

static int tilesClipRect (int *bx, int *by, int *blocksAcross, int *blocksDown)
{
	const int tAcross = tilesTotalAcross();
	const int tDown = tilesTotalDown();
	
	if (*blocksAcross > tAcross) *blocksAcross = tAcross;
	if (*blocksDown > tDown) *blocksDown = tDown;

	if (*bx < 0)
		*bx = 0;
	else if (*bx >= tAcross)
		return 0;

	if (*by < 0)
		*by = 0;
	else if (*by >= tDown)
		return 0;

	if (*bx + *blocksAcross > tAcross) *blocksAcross = (tAcross - *bx)+1;
	if (*by + *blocksDown > tDown) *blocksDown = (tDown - *by)+1;

	return 1;
}


#if 0
int loadPOI (poi_t *poi, const float xlon, const float ylat, int32_t x, int32_t y)
{

	x += xlon;
	x -= x&0x03;
	y += ylat;
	y -= y&0x03;

	if (poiIsBlockLoaded(poi, x, y)){
		//printf("blocked loaded %i %i\n", x, y);
		return 1;
	}

	//printf(CS("loadPOI: %i %i, %i"), (int)x, (int)y, (int)poi->string.end);

	uint8_t buffer[128];
	snprintf((char*)buffer, sizeof(buffer)-1,  "%s/%03i_%03i.poi", POI_PATH, (int)y, (int)x);

	fileio_t *fpoi = fio_open(buffer, FIO_READ);
	if (!fpoi){
		//printf("loadPOI(): open failed for '%s'\n", buffer);
		return 0;
	}

	poi_file_t *blk = poiGetNewBlock(poi, 1);
	if (!blk){
		fio_close(fpoi);
		return 0;
	}
	
	poi_obj_t poiobj;
	int addCt = 0;

	while(1){
		if (!fio_read(fpoi, &poiobj, sizeof(poiobj))){		// read header
			//printf("loadPOI(): header read failed for '%s'\n", buffer);
			break;	// end of block reached
		}

		if (poiobj.len < sizeof(buffer)){						// if text isn't too large
			if (!fio_read(fpoi, buffer, poiobj.len)){		// read string
				//printf("loadPOI(): string read failed for %i %i, len = %i\n", (int)y, (int)x, poiobj.len);
				break;
			}

			buffer[64] = 0;	// limit strings to a sensible - viewable length
			buffer[poiobj.len] = 0;
			uint16_t strIdx = poiBlockAddObj(poi, blk, &poiobj, buffer);
			addCt += (strIdx != 0xFFFF);
		}else{
			polyfileAdvance(fpoi, poiobj.len);
		}
	}

	if (addCt){			// we're successful so
		blk->x = x;		// tag block as used
		blk->y = y;
	}
	
	fio_close(fpoi);
	return 1;
}
#endif

// load tile(s) that would be visible across the viewport
static int tiles8LoadBySpan (application_t *inst, vectorPt2_t *location, const float zoom)
{
	
	int ct = 0;
	int bx, by, blocksAcross, blocksDown;
	getBlockCoverage(location, zoom, &bx, &by, &blocksAcross, &blocksDown);


#if VERTICAL_DISPLAY		// fudge for vertical displays
	by -= (PACK_DOWN*2);
	blocksDown += (PACK_DOWN*4);
#endif

	if (!tilesClipRect(&bx, &by, &blocksAcross, &blocksDown))
		return ct;

	const int tAcrossRowLength = (tilesTotalAcross() / PACK_ACROSS) + 1;
	block_t block;
	
	for (int i = by; i < by+blocksDown; i++){
		for (int j = bx; j < bx+blocksAcross; j++){
			int tileY = (i & ~PACK_MASK) / PACK_DOWN;
			int tileX = (j & ~PACK_MASK) / PACK_ACROSS;
			
			if (!tiles8[tileY])
				tiles8[tileY] = (tile8_t**)l_calloc(tAcrossRowLength, sizeof(tile8_t*));

			tile8_t *tile = tiles8[tileY][tileX];
			if (tile){
				int y = i & PACK_MASK;
				int x = j & PACK_MASK;
				if (tile->block && tile->block[y] && tile->block[y][x])
					continue;
			}

			block.size = 0;
			int blkTotal = blockLoad(&block, POLY_PATH, i, j);
			
#if (MEMORYPROFILE == 1)		// if memory is expensive. slower to repeat load larger maps
			if (!blkTotal) continue;
#else							// if memory is cheap. faster tile reloads 
			if (!blkTotal) memset(&block, 0, sizeof(block));
#endif
			const int y = i & PACK_MASK;
			const int x = j & PACK_MASK;
				
			if (!tile){
				tile = (tile8_t*)l_calloc(1, sizeof(tile8_t));
				tile8Set(i, j, tile);
			}
			if (!tile->block)
				tile->block = (block_t***)l_calloc(PACK_DOWN, sizeof(block_t*));
			if (!tile->block[y])
				tile->block[y] = (block_t**)l_calloc(PACK_ACROSS, sizeof(block_t*));
				
			tile->block[y][x] = (block_t*)l_malloc(sizeof(block_t));
			*tile->block[y][x] = block;
			
			ct++;
			
			// load one block at a time
			if (blkTotal){
				//loadPOI(&inst->poi, j, i, 0, 0);
				break;
			}
		}
	}

	return ct;
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

void windowToLocation (application_t *inst, const int x, const int y, vectorPt2_t *loc)
{
	vectorPt4_t *win = viewportGetWindow(inst);
	double _y = (y + aspectOffset) * aspectCorrection;
	loc->lat = win->v1.lat - (_y * viewportGetHeight(inst));
	loc->lon = win->v1.lon + ((double)x * viewportGetWidth(inst));
}

void sceneLoadTiles (application_t *inst)
{
	vectorPt2_t loc = sceneGetLocation(inst);
	tiles8LoadBySpan(inst, &loc, sceneGetZoom(inst));

#if 0
	int bx, by, blocksAcross, blocksDown;
	getBlockCoverage(&loc, sceneGetZoom(inst), &bx, &by, &blocksAcross, &blocksDown);

	int x = bx + (blocksAcross/2);
	int y = by + (blocksDown/2);
	loadPOI(&inst->poi, x, y, 0, 0);
#endif
	
}

FLASHMEM void tilesInit ()
{
	calcTileCoverage();
	tiles8 = (tile8_t***)l_calloc((tilesTotalDown()/PACK_DOWN)+1, sizeof(tile8_t*));
}

void tilesClose ()
{
	// this is a memleak but is never called
	l_free(tiles8);
}

void tilesMakeGPSWindow (const vectorPt2_t *center, const double spanMeters, vectorPt4_t *out)
{
	makeGPSWindow(center, spanMeters, out);
}

void tilesGetBlockCoverage (vectorPt2_t *center, const float spanMeters, int *x_lon, int *y_lat, int *blocksAcross, int *blocksDown)
{
	getBlockCoverage(center, spanMeters, x_lon, y_lat, blocksAcross, blocksDown);
}

block_t *tilesBlock8Get (const int x_lon, const int y_lat)
{
	return block8Get(x_lon, y_lat);
}
