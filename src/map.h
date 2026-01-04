

#ifndef _MAP_H_
#define _MAP_H_




#define MAP_RENDER_VIEWPORT			0x0001
#define MAP_RENDER_TRACKPOINTS		0x0002
#define MAP_RENDER_LOCGRAPTHIC		0x0004
#define MAP_RENDER_MEASURE			0x0008
#define MAP_RENDER_COMPASS			0x0010
#define MAP_RENDER_POI				0x0020
#define MAP_RENDER_SLEVELS			0x0040
#define MAP_RENDER_SAVAIL			0x0080
#define MAP_RENDER_SWORLD			0x0100
#define MAP_RENDER_CONSOLE			0x0200
#define MAP_RENDER_STRINGS			0x0400
#define MAP_RENDER_AREAS			0x0800
#define MAP_RENDER_AREAS_OUTLINE	0x1000
#define MAP_RENDER_PATHS			0x2000
#define MAP_RENDER_PATHS_LINE		0x4000
#define MAP_RENDER_TITLE_BOUNDARY	0x8000



void map_init (vfont_t *vfont);
void map_render (trackRecord_t *trackRecord, const pos_rec_t *location, const float heading, const uint32_t flags);
void map_setDetail (const uint32_t detail, uint32_t state);
uint32_t map_getDetail (const uint32_t detail);


#endif
