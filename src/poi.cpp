

// A Vector font libarary beased upon the Hershey font set
// 
// Michael McElligott
// okio@users.sourceforge.net
// https://youtu.be/T0WgGcm7ujM

//  Copyright (c) 2005-2017  Michael McElligott
// 
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU LIBRARY GENERAL PUBLIC LICENSE
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU LIBRARY GENERAL PUBLIC LICENSE for more details.
//
//	You should have received a copy of the GNU Library General Public
//	License along with this library; if not, write to the Free
//	Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.



#include "commonGlue.h"



static inline void poiDeleteBlock (poi_file_t *blk)
{
	for (uint16_t i = 0; i < POI_MAX_STRING; i++)
		blk->poi[i].strIdx = 0xFFFF;
	blk->x = 0xFFFF;
	blk->y = 0xFFFF;
}

void poiCleanBlocks (poi_t *poi)
{
	memset(poi, 0, sizeof(*poi));
	
	for (uint16_t i = 0; i < POI_MAX_BLOCKS; i++)
		poiDeleteBlock(&poi->blocks[i]);
		
	for (uint16_t i = 0; i < POI_MAX_STRINGS; i++)
		poi->string.offsets[i] = 0xFFFF;
}

FLASHMEM void poiInit (poi_t *poi)
{
	//memset(poi, 0, sizeof(*poi));
	poiCleanBlocks(poi);
}

int poiIsBlockLoaded (poi_t *poi, const uint16_t x, const uint16_t y)
{
	for (uint16_t i = 0; i < POI_MAX_BLOCKS; i++){
		poi_file_t *blk = &poi->blocks[i];
		
		if (blk->x == x && blk->y == y)
			return 1;
	}
	return 0;
}

static inline uint16_t poiGetStringOffset (poi_t *poi, const uint16_t strIdx)
{
	return poi->string.offsets[strIdx];
}

const uint8_t *poiGetString (poi_t *poi, const uint16_t strIdx)
{
	return &poi->string.storage[poiGetStringOffset(poi, strIdx)];
}

static inline void poiRemoveString (poi_t *poi, const uint16_t strIdx)
{
	const uint8_t *str = poiGetString(poi, strIdx);
	const uint16_t space = strlen((char*)str)+1;
	const uint16_t strOffset = poiGetStringOffset(poi, strIdx);
	
	poi->string.offsets[strIdx] = 0xFFFF;
		
	uint8_t *to = (uint8_t*)str;
	uint8_t *from = (uint8_t*)(str + space);
	size_t len = &poi->string.storage[POI_MAX_STORAGE-1] - from;
	
	memcpy(to, from, len);
	poi->string.end -= space;
	
	for (uint16_t i = 0; i < POI_MAX_STRINGS; i++){
		if (poi->string.offsets[i] != 0xFFFF && poi->string.offsets[i] > strOffset){
			if (poi->string.offsets[i] <= space)
				poi->string.offsets[i] = 0;
			else
				poi->string.offsets[i] -= space;
		}
	}
}

uint16_t poiCountStringSlot (poi_t *poi)
{
	uint16_t count = 0;
	
	for (uint16_t i = 0; i < POI_MAX_STRINGS; i++)
		count += (poi->string.offsets[i] == 0xFFFF);
	
	return count;
}

static inline uint16_t poiGetFreeStringSlot (poi_t *poi)
{
	for (uint16_t i = 0; i < POI_MAX_STRINGS; i++){
		if (poi->string.offsets[i] == 0xFFFF)
			return i;
	}
	return 0xFFFF;
}

static inline uint16_t poiAddString (poi_t *poi, const uint8_t *str)
{
	const uint16_t space = strlen((char*)str)+1;
	if ((POI_MAX_STORAGE - poi->string.end)+1 < space){			// no space left
		return 0xFFFF;
	}

	const uint16_t strIdx = poiGetFreeStringSlot(poi);
	if (strIdx == 0xFFFF) return 0xFFFF;					// space available for nowhere to put string lookup index 

	poi->string.offsets[strIdx] = poi->string.end;
	void *pos = &poi->string.storage[poi->string.end];
	memcpy(pos, str, space);
	poi->string.end += space;

	return strIdx;
}

static inline int poiRemoveBlock (poi_t *poi, const uint16_t x, const uint16_t y)
{
	for (uint16_t i = 0; i < POI_MAX_BLOCKS; i++){
		poi_file_t *blk = &poi->blocks[i];
		
		if (blk->x == x && blk->y == y){
			for (uint16_t j = 0; j < POI_MAX_STRING; j++){
				if (blk->poi[j].strIdx != 0xFFFF){
					poiRemoveString(poi, blk->poi[j].strIdx);
				}
			}
			poiDeleteBlock(blk);
			return 1;
		}
	}
	return 0;
}

static inline poi_file_t *poiGetFreeBlock (poi_t *poi)
{
	for (uint16_t i = 0; i < POI_MAX_BLOCKS; i++){
		poi_file_t *blk = &poi->blocks[i];
		if (blk->x == 0xFFFF)
			return blk;
	}
	
	return NULL;
}


poi_file_t *poiGetNewBlock (poi_t *poi, const int32_t flush)
{
	poi_file_t *blk = poiGetFreeBlock(poi);

	
	if (!blk && flush){						// flush oldest block if non available
		blk = &poi->blocks[0];
		poiRemoveBlock(poi, blk->x, blk->y);

		for (int i = 1; i < POI_MAX_BLOCKS; i++)
			poi->blocks[i-1] = poi->blocks[i];

		poiDeleteBlock(&poi->blocks[POI_MAX_BLOCKS-1]);
		blk = poiGetFreeBlock(poi);
	}
	return blk;
}


int poiBlockAddObj (poi_t *poi, poi_file_t *blk, poi_obj_t *obj, void *data)
{
	for (int i = 0; i < POI_MAX_STRING; i++){
		if (blk->poi[i].strIdx == 0xFFFF){
			blk->poi[i].strIdx = poiAddString(poi, (const uint8_t*)data);
			blk->poi[i].type = obj->type;
			blk->poi[i].vec = obj->vec;
			return blk->poi[i].strIdx;
		}
	}
	return 0xFFFF;
}
