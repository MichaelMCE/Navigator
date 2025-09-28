
#if 0

#include <Arduino.h>
#include "vfont/vfont.h"
#include "fileio.h"
#include "map.h"
#include "tracks.h"


#define l_malloc(n)		malloc(n)
#define l_calloc(a,b)	calloc((a),(b))
#define l_realloc(p, n)	realloc((p),(n))
#define l_free(p)		free(p)




track_t *loadPath (const uint8_t *filename)
{
	fileio_t *file = fio_open(filename, FIO_READ);
	if (!file){
		return NULL;
	}
	
	track_t *trk = (track_t*)l_calloc(1, sizeof(track_t));
	if (!trk){
		return NULL;
	}
	
	uint32_t len = (uint32_t)fio_length(file);
	if (len <= sizeof(log_header_t)){
		l_free(trk);
		fio_close(file);
		return NULL;
	}
	size_t allocLen = ((len/sizeof(gps_datapt_t))+1) * sizeof(gps_datapt_t);
	trk->pt = (gps_datapt_t*)l_calloc(1, allocLen);		// a few more than enough
	if (!trk->pt){
		l_free(trk);
		fio_close(file);
		return NULL;
	}
	
	
	fio_read(file, &trk->header, sizeof(log_header_t));
	trk->total = 0;
	
	while (1){
		if (!fio_read(file, &trk->pt[trk->total], sizeof(gps_datapt_t)))
			break;
		trk->total++;
	}
	
	fio_close(file);
	return trk;
}
#endif



