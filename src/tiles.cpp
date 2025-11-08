


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
		3.4886396,		// 3.48816 width
		1.4530819,		// 1.45294 height
		0, 0
};
#endif



EXTMEM static uint8_t pkMemAllocAddr[(int)(4.5f*1024*1024)];
EXTMEM static uint8_t pkFileBuffer[400*1024];

static const size_t extBaseAddr = (size_t)pkMemAllocAddr;
static size_t extCurrentAddr = (size_t)extBaseAddr;

static uint16_t pk_x = 0xFFFF;
static uint16_t pk_y = 0xFFFF;
static uint64_t pk_len = 0;






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
/*
static int polyfileSeek (fileio_t *file, const size_t pos)
{
	return fio_seek(file, pos);
}*/

static inline void polyfileClose (fileio_t *file)
{
	fio_close(file);
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

static inline void *extMalloc (size_t size)
{
	size_t addr = extCurrentAddr;

	size >>= 2;		// round up to 4 bytes
	size <<= 2;
	size += 4;
	extCurrentAddr += size;

	return (void*)(addr);
}

static inline void *extCalloc (const size_t nmemb, const size_t size)
{
	void *addr = extMalloc(nmemb*size);
	if (addr){
		char *paddr = (char*)addr;
		for (int i = 0; i < (int)(nmemb*size); i++)
			paddr[i] = 0;
	}
	return addr;
}

static inline void extMemcpy (void *dest_str, const void *src_str, size_t n)
{
	char *d = (char*)dest_str;
	char *s = (char*)src_str;
	
	for (int i = 0; i < (int)n; i++)
		*d++ = *s++;
}

FASTRUN static inline void tileInit (block_t *block, const int tPolySize)
{
	block->total = 0;
	block->size = tPolySize;
	block->list = (polyline_t*)extMalloc(block->size * sizeof(polyline_t));
	block->lastRendered = 0;
}

FASTRUN static inline void vectorsAlloc (const uint32_t total, vectors_t *vectors)
{
	vectors->total = total;
	vectors->list = (vector2_t*)extMalloc(total * sizeof(vector2_t));
}

#if 0
FASTRUN static void blockRelease (block_t *block)
{
	if (block->list){
		for (int j = 0; j < (int)block->total; j++)
			l_free(block->list[j].vectors.list);
		l_free(block->list);
	}
}
#endif

FASTRUN static const uint8_t *pkLoad (const uint8_t *dir, const int32_t x_lon, const int32_t y_lat, int *polyLen)
{
	const int xm = (x_lon % PACK_ACROSS);
	const int ym = (y_lat % PACK_DOWN);
	const int x = x_lon - xm;
	const int y = y_lat - ym;

	*polyLen = 0;

	if (x != pk_x || y != pk_y){
		pk_x = x;
		pk_y = y;
		
		uint8_t filename[64];
		snprintf((char*)filename, sizeof(filename)-1, "%s/%03i_%03i.pk32", dir, y, x);

		fileio_t *file = fio_open(filename, FIO_READ);
		if (!file){
			printf(CS(" pkLoad(): open failed for '%s'"), filename);
			return NULL;
		}
		
		pk_len = fio_length(file);
		if (polyfileRead(file, pkFileBuffer, pk_len) != 1){
			printf(CS("pkLoad(): read failed for '%s'"), filename);
			polyfileClose(file);
			return NULL;
		}
		polyfileClose(file);
	}
	
	size_t pos = (ym * PACK_ACROSS * sizeof(poly_pack_file_t)) + (xm * sizeof(poly_pack_file_t));
	poly_pack_file_t *poly = (poly_pack_file_t*)&pkFileBuffer[pos];
	
	if (poly->offset >= pk_len)
		return NULL;

	*polyLen = poly->length;
	return &pkFileBuffer[poly->offset];
}

FASTRUN static int pkBlockLoad (block_t *block, const uint8_t *dir, const int32_t y_lat, const int32_t x_lon)
{
	int polyLen = 0;
	const uint8_t * const buffer = pkLoad(dir, x_lon, y_lat, &polyLen);
	if (!buffer){
		//printf("could not open %i %i from '%s\\'\n", x_lon, y_lat, dir);
		return 0;
	}

	int bCt = 0;
	int lengthRead = 0;

	while (lengthRead < polyLen){
		poly_field_t *field = (poly_field_t*)&buffer[lengthRead];
		lengthRead += sizeof(poly_field_t);
		lengthRead += sizeof(vector2_t) * field->total;
		bCt++;
	};
	
	if (!bCt) return bCt;

	tileInit(block, bCt);
	bCt = 0;
	lengthRead = 0;

	while (lengthRead < polyLen){
		poly_field_t *field = (poly_field_t*)&buffer[lengthRead];
		lengthRead += sizeof(poly_field_t);

		polyline_t *polyline = &block->list[bCt++];
		polyline->type = field->type;
		vectorsAlloc(field->total, &polyline->vectors);
		
		extMemcpy(polyline->vectors.list, &buffer[lengthRead], sizeof(vector2_t) * field->total);
		lengthRead += sizeof(vector2_t) * field->total;
	};

	block->total = bCt;
	return bCt;
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
	
FASTRUN uint32_t tilesTotalAcross ()
{
	return coverage.tilesAcross;
}

FASTRUN uint32_t tilesTotalDown ()
{
	return coverage.tilesDown;
}

FASTRUN static inline tile8_t *tile8Get (const int y_lat, const int x_lon)
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

static inline void tilesClean ()
{
	extCurrentAddr = (size_t)extBaseAddr;
	memset(pkMemAllocAddr, 0, sizeof(pkMemAllocAddr));
}

void tilesUnloadAll (application_t *inst)
{
	tilesInit();
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

static int tiles8LoadByLocation (application_t *inst, vectorPt2_t *location)
{
	int bx, by, blocksAcross, blocksDown;
	getBlockCoverage(location, 2.0f, &bx, &by, &blocksAcross, &blocksDown);

	if (!tilesClipRect(&bx, &by, &blocksAcross, &blocksDown))
		return 0;

	int tileY = (by & ~PACK_MASK) / PACK_DOWN;
	int tileX = (bx & ~PACK_MASK) / PACK_ACROSS;
	int y = by & PACK_MASK;
	int x = bx & PACK_MASK;

	const int tAcrossRowLength = (tilesTotalAcross() / PACK_ACROSS) + 1;
	if (!tiles8[tileY])
		tiles8[tileY] = (tile8_t**)extCalloc(tAcrossRowLength, sizeof(tile8_t*));

	tile8_t *tile = tiles8[tileY][tileX];	
	if (tile){
		if (tile->block && tile->block[y] && tile->block[y][x])
			return 1;
	}

	block_t block;
	block.size = 0;
	int blkTotal = pkBlockLoad(&block, POLY_PATH, by, bx);
	if (!blkTotal) memset(&block, 0, sizeof(block));
	if (!tile){
		tile = (tile8_t*)extCalloc(1, sizeof(tile8_t));
		tile8Set(by, bx, tile);
	}

	if (!tile->block)
		tile->block = (block_t***)extCalloc(PACK_DOWN, sizeof(block_t*));
	if (!tile->block[y])
		tile->block[y] = (block_t**)extCalloc(PACK_ACROSS, sizeof(block_t*));
	if (!tile->block[y][x])
		tile->block[y][x] = (block_t*)extMalloc(sizeof(block_t));

	*tile->block[y][x] = block;

	return 1;
}

int tilesCount ()
{
	const int across = tilesTotalAcross() / PACK_ACROSS;
	const int down = tilesTotalDown() / PACK_DOWN;
	
	int ct = 0;
	for (int y = 0; y < down; y++){
		if (!tiles8[y]) continue;
		
		for (int x = 0; x < across; x++){
			ct += (tiles8[y][x] != NULL);
		}
	}
	
	return ct;
}

int blocksCount ()
{
	const int across = tilesTotalAcross() / PACK_ACROSS;
	const int down = tilesTotalDown() / PACK_DOWN;
	
	int ct = 0;
	for (int y = 0; y < down; y++){
		if (!tiles8[y]) continue;
		
		for (int x = 0; x < across; x++){
			if (tiles8[y][x]){
				for (int i = 0; i < PACK_DOWN; i++){
					if (!tiles8[y][x]->block[i]) continue;

					for (int j = 0; j < PACK_ACROSS; j++){
						ct += (tiles8[y][x]->block[i][j] != NULL);
					}
				}
			}
		}
	}
	
	return ct;
}

size_t tileMemoryUsage ()
{
	return (extCurrentAddr - extBaseAddr);
}

// load tile(s) that would be visible across the viewport
static int tiles8LoadBySpan (application_t *inst, vectorPt2_t *location, const float zoom, const int maxLoadable)
{
	int ct = 0;
	int bx, by, blocksAcross, blocksDown;
	getBlockCoverage(location, zoom, &bx, &by, &blocksAcross, &blocksDown);

	if (!tilesClipRect(&bx, &by, &blocksAcross, &blocksDown))
		return ct;

	block_t ext_block;
	const int tAcrossRowLength = (tilesTotalAcross() / PACK_ACROSS) + 1;
	
	for (int i = by; i < by+blocksDown; i++){
		for (int j = bx; j < bx+blocksAcross; j++){
			int tileY = (i & ~PACK_MASK) / PACK_DOWN;
			int tileX = (j & ~PACK_MASK) / PACK_ACROSS;
			
			if (!tiles8[tileY])
				tiles8[tileY] = (tile8_t**)extCalloc(tAcrossRowLength, sizeof(tile8_t*));

			tile8_t *tile = tiles8[tileY][tileX];
			if (tile){
				int y = i & PACK_MASK;
				int x = j & PACK_MASK;
				if (tile->block && tile->block[y] && tile->block[y][x])
					continue;
			}

			ext_block.size = 0;
			int blkTotal = pkBlockLoad(&ext_block, POLY_PATH, i, j);
			
#if (MEMORYPROFILE == 1)		// if memory is expensive. slower to repeat load larger maps
			if (!blkTotal) continue;
#else							// if memory is cheap. faster tile reloads 
			if (!blkTotal) memset(&ext_block, 0, sizeof(ext_block));
#endif
			const int y = i & PACK_MASK;
			const int x = j & PACK_MASK;
				
			if (!tile){
				tile = (tile8_t*)extCalloc(1, sizeof(tile8_t));
				tile8Set(i, j, tile);
			}
			if (!tile->block)
				tile->block = (block_t***)extCalloc(PACK_DOWN, sizeof(block_t*));
			if (!tile->block[y])
				tile->block[y] = (block_t**)extCalloc(PACK_ACROSS, sizeof(block_t*));
				
			tile->block[y][x] = (block_t*)extMalloc(sizeof(block_t));
			*tile->block[y][x] = ext_block;

			if (++ct >= maxLoadable){
				//loadPOI(&inst->poi, j, i, 0, 0);
				return ct;
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

int sceneLoadTiles (application_t *inst)
{
	vectorPt2_t loc = sceneGetLocation(inst);
	tiles8LoadByLocation(inst, &loc);
	int ct = tiles8LoadBySpan(inst, &loc, sceneGetZoom(inst), 1);

	//printf(CS("extCurrentAddr: %i"), extCurrentAddr-extBaseAddr);

#if 0
	int bx, by, blocksAcross, blocksDown;
	getBlockCoverage(&loc, sceneGetZoom(inst), &bx, &by, &blocksAcross, &blocksDown);

	int x = bx + (blocksAcross/2);
	int y = by + (blocksDown/2);
	ct += loadPOI(&inst->poi, x, y, 0, 0);
#endif

	return ct;
}

void sceneLoadTilesMax (application_t *inst, const int max)
{
	vectorPt2_t loc = sceneGetLocation(inst);
	tiles8LoadBySpan(inst, &loc, sceneGetZoom(inst), max);
	
	//printf(CS("extCurrentAddr: %i"), extCurrentAddr-extBaseAddr);
}

void sceneLoadTilesComplete (application_t *inst)
{
	sceneLoadTilesMax(inst, 200);
}

FLASHMEM void tilesInit ()
{
	pk_x = 0xFFFF;
	pk_y = 0xFFFF;
	pk_len = 0;

	tilesClean();
	calcTileCoverage();
	tiles8 = (tile8_t***)extCalloc((tilesTotalDown()/PACK_DOWN)+1, sizeof(tile8_t*));
}

void tilesClose ()
{
	// this is a memleak but is never called
	//l_free(tiles8);
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
