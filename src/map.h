

#ifndef _MAP_H_
#define _MAP_H_




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




void map_init (vfont_t *vfont);
void map_render (trackRecord_t *trackRecord, const pos_rec_t *location, const float heading, const uint32_t flags);
void map_setDetail (const uint32_t detail, uint32_t state);



#endif

