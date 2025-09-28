
#ifndef _tiles_h_
#define _tiles_h_




#define POLY_PATH		(uint8_t*)"/polys/500_32"
#define POI_PATH		(uint8_t*)"/POI"


#if 1
#define l_malloc(n)		malloc(n)
#define l_calloc(a,b)	calloc((a),(b))
#define l_realloc(p, n)	realloc((p),(n))
#define l_free(p)		free(p)
#else
#define l_malloc(n)		extmem_malloc(n)
#define l_calloc(a,b)	extmem_calloc((a),(b))
#define l_realloc(p, n)	extmem_realloc((p),(n))
#define l_free(p)		extmem_free(p)
#endif

void tilesClose ();
void tilesInit ();

void tilesMakeGPSWindow (const vectorPt2_t *center, const double spanMeters, vectorPt4_t *out);
uint32_t tilesTotalAcross ();
uint32_t tilesTotalDown ();
int tilesUnloadAll (application_t *inst);
int tilesUnload (const uint32_t renderPass);
void tilesGetBlockCoverage (vectorPt2_t *center, const float spanMeters, int *x_lon, int *y_lat, int *blocksAcross, int *blocksDown);
block_t *tilesBlock8Get (const int x_lon, const int y_lat);


#endif

