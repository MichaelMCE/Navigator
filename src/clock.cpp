

#include "commonGlue.h"




extern application_t inst;



FLASHMEM void clock_init ()
{

}

void clock_render (const char *timeStr, const char *timeStrSeconds)
{
	setBrush(inst.vfont, BRUSH_DISK);
	setBrushColour(inst.vfont, COLOUR_PAL_CREAM);
	setBrushQuality(inst.vfont, 1);
	setGlyphScale(inst.vfont, 9.2);
	setBrushSize(inst.vfont, 32.0);


	int x = (VWIDTH/2);
	int y = (VHEIGHT/2);
	
	box_t box = {0};
	getStringMetrics(inst.vfont, timeStr, &box);
	x -= box.x2 / 2;
	y -= 30;
	drawString(inst.vfont, timeStr, x, y);
	
	
	setGlyphScale(inst.vfont, 4.0);
	setBrushSize(inst.vfont, 8.0);
	
	x = (VWIDTH/2);
	getStringMetrics(inst.vfont, timeStrSeconds, &box);
	x -= box.x2 / 2;
	y = (VHEIGHT - box.y2) - 30;
	drawString(inst.vfont, timeStrSeconds, x, y);
}

