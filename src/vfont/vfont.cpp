



#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>


#define VFONT_RENDER_QUALITY	1		// 1=higher performance. 0=lower performing but higher quality rendering

#include "vfont.h"



extern uint8_t renderBuffer[VWIDTH*VHEIGHT];	// our render distination. defined in primitives.cpp



const inline void drawPixel (const int x, const int y, const uint16_t colourIdx)
{
	uint8_t *pixels = (uint8_t*)renderBuffer;	
	pixels[(y*VWIDTH)+x] = colourIdx;
}

#if 0
static inline void drawPixel16 (const int x, const int y, const uint16_t colour)
{
	uint16_t *pixels = (uint16_t*)renderBuffer;	
	pixels[(y*VWIDTH) + x] = colour;
}
#endif

static inline void drawBrushBitmap (vfont_t *ctx, const int x, const int y, const uint16_t colour)
{
	drawBitmap(&ctx->brush.image, x, y, colour);
}

static inline void drawCircleFilled3 (const int x0, const int y0, const int radius, const uint8_t colour)
{
	if ((x0 - radius < 0) || (x0 + radius >= VWIDTH)) return;
	if ((y0 - radius < 0) || (y0 + radius >= VHEIGHT)) return;
	
#if 1	// better looking
	const int radiusMul = radius*radius + radius;
	
	for (int y = -radius; y <= radius; y++){
		int yy = y*y;
		int y0y = y0+y;
	    for (int x = -radius; x <= radius; x++)
        	if (x*x+yy < radiusMul)
	            drawPixel(x0+x, y0y, colour);
	}
#else
	const int radiusMul = radius*radius + radius;
	
	for (int x = -radius; x < radius ; x++)
	{
	    int height = (int)sqrtf(radiusMul - x * x);
	
		int X0 = x + x0;
    	for (int y = -height; y < height; y++)
        	drawPixel(X0, y + y0, colour);
	}
#endif
}

static inline void drawBrush (vfont_t *ctx, float xc, float yc, const float radius, const uint16_t colour)
{
	
	if (ctx->brush.type == BRUSH_DISK){
		if (ctx->brush.size < 2.0f)
			drawCircleFilled(xc, yc, radius, colour);
		else
			drawCircleFilled3(xc, yc, radius, colour);

	}else if (ctx->brush.type == BRUSH_SQUARE_FILLED){
		int d = (int)radius>>1;
		drawRectangleFilled(xc-d, yc-d, xc+d, yc+d, colour);

	}else if (ctx->brush.type == BRUSH_SQUARE){
		int d = (int)radius>>1;
		drawRectangle(xc-d, yc-d, xc+d, yc+d, colour);

	}else if (ctx->brush.type == BRUSH_TRIANGLE_FILLED){
		int d = (int)radius>>1;
		drawTriangleFilled(xc-d, yc+d, xc, yc-d, xc+d, yc+d, colour);
		
	}else if (ctx->brush.type == BRUSH_TRIANGLE){
		int d = (int)radius>>1;
		drawTriangle(xc-d, yc+d, xc, yc-d, xc+d, yc+d, colour);

	}else if (ctx->brush.type == BRUSH_CIRCLE){
		drawCircle(xc, yc, radius, colour);
	
	}else if (ctx->brush.type == BRUSH_STROKE_1){
		int d = (int)radius>>1;
		drawLine(xc-d, yc+d, xc+d, yc-d, colour);		// slope up
		xc++;
		drawLine(xc-d, yc+d, xc+d, yc-d, colour);
		
	}else if (ctx->brush.type == BRUSH_STROKE_2){
		int d = (int)radius>>1;
		drawLine(xc-d, yc-d, xc+d, yc+d, colour);		// slope down
		xc++;
		drawLine(xc-d, yc-d, xc+d, yc+d, colour);

	}else if (ctx->brush.type == BRUSH_STROKE_3){
		int d = (int)radius>>1;
		drawLine(xc-d, yc, xc+d, yc, colour);			// horizontal
		yc++;
		drawLine(xc-d, yc, xc+d, yc, colour);

	}else if (ctx->brush.type == BRUSH_STROKE_4){
		int d = (int)radius>>1;
		drawLine(xc, yc-d, xc, yc+d, colour);			// vertical
		xc++;
		drawLine(xc, yc-d, xc, yc+d, colour);

	}else if (ctx->brush.type == BRUSH_STROKE_5){
		int d = (int)radius>>1;
		drawLine(xc-d, yc+d, xc+d, yc-d, colour);		// forward slope up with smaller siblings either side
		d = radius*0.3;
		xc++;
		drawLine(xc-d, yc+d, xc+d, yc-d, colour);
		xc -= 2;
		//yc -= 2;
		drawLine(xc-d, yc+d, xc+d, yc-d, colour);

	}else if (ctx->brush.type == BRUSH_STROKE_6){
		int d = (int)radius>>1;
		drawLine(xc-d, yc+d, xc+d, yc-d, colour);		// BRUSH_STROKE_5 but thicker
		xc++;
		drawLine(xc-d, yc+d, xc+d, yc-d, colour);
		d = radius*0.3;
		xc++;
		drawLine(xc-d, yc+d, xc+d, yc-d, colour);		// right side
		xc -= 2; yc -= 1;
		drawLine(xc-d, yc+d, xc+d, yc-d, colour);		// left side

	}else if (ctx->brush.type == BRUSH_STROKE_7){
		int d = (int)radius>>1;
		//drawLine(xc-d, yc+d, xc+d, yc-d);
		
		int d2 = d + 2;
		drawLine(xc-d2, yc+d2, xc-d, yc+d, colour);
		drawLine((xc-d2)-1, yc+d2, (xc-d)-1, yc+d, colour);
		drawLine(xc+d, yc-d, xc+d2, yc-d2, colour);
		drawLine(xc+d+1, yc-d, xc+d2+1, yc-d2, colour);

		drawLine(xc+d2, yc+d2, xc+d, yc+d, colour);
		drawLine(xc+d2+1, yc+d2, xc+d+1, yc+d, colour);
		drawLine(xc-d, yc-d, xc-d2, yc-d2, colour);
		drawLine((xc-d)-1, yc-d, (xc-d2)-1, yc-d2, colour);
				
	}else if (ctx->brush.type == BRUSH_STAR){
		int d = (int)radius>>1;
		drawLine(xc-d, yc+d, xc+d, yc-d, colour);		// slope up
		drawLine(xc-d, yc-d, xc+d, yc+d, colour);		// slope down
		drawLine(xc-d, yc, xc+d, yc, colour);			// horizontal
		drawLine(xc, yc-d, xc, yc+d, colour);			// vertical
		
	}else if (ctx->brush.type == BRUSH_X){
		int d = (int)radius>>1;
		drawLine(xc-d, yc+d, xc+d, yc-d, colour);		// slope up
		drawLine(xc-d, yc-d, xc+d, yc+d, colour);		// slope down
		
	}else if (ctx->brush.type == BRUSH_PLUS){
		int d = (int)radius>>1;
		drawLine(xc-d, yc, xc+d, yc, colour);			// horizontal
		drawLine(xc, yc-d, xc, yc+d, colour);			// vertical

	}else if (ctx->brush.type == BRUSH_CARET){
		int d = (int)radius>>1;							// ^ (XOR operator)
		drawLine(xc-d, yc+d, xc, yc-d, colour);			// left
		drawLine(xc, yc-d, xc+d, yc+d, colour);			// right
		
	}else if (ctx->brush.type == BRUSH_BITMAP){
		drawBrushBitmap(ctx, xc, yc, colour);
		
	}else{	// BRUSH_POINT
		drawPixel(xc, yc, colour);
	}
}

static inline float distance (const float x1, const float y1, const float x2, const float y2)
{
	const float x = x1 - x2;
	const float y = y1 - y2;
	return sqrtf((x * x) + (y * y));

	//return sqrtf((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

//check if point2 is between point1 and point3 (the 3 points should be on the same line)
static inline int isBetween (const float x1, const float y1, const float x2, const float y2, const float x3, const float y3)
{
	return ((int)(x1 - x2) * (int)(x3 - x2) <= 0.0f) && ((int)(y1 - y2) * (int)(y3 - y2) <= 0.0f);
}

static inline void rotate (float angle, const float x, const float y, float *xr, float *yr)
{
	*xr = x * cosf(angle) - y * sinf(angle);
	*yr = x * sinf(angle) + y * cosf(angle);
}

void drawPoly (const float x1, const float y1, const float x2, const float y2, const float thickness, float angle, const uint16_t colour)
{
	//if (!clipLinef(x1, y1, x2, y2, &x1, &y1, &x2, &y2))
		//return;
		
	// cross product
	float x = (y1 * 1.0f) - (1.0f * y2);
	float y = (1.0f * x2) - (x1 * 1.0f);


	const float length = sqrtf((x * x) + (y * y));
	const float mul = (1.0 / length) * thickness;

	// set thickness;
	x *= mul ; //thickness * (1.0 / length);
	y *= mul ; //thickness * (1.0 / length);
	float xh = (x / 2.0f);	// center path by precomputing and using half thickness per side
	float yh = (y / 2.0f);


	float xr, yr;
	rotate(-DEG2RAD(angle), xh, yh, &xr, &yr);
	drawTriangle(x1+xh, y1+yh, x1-xh, y1-yh, x2+xh, y2+yh, colour);
	drawTriangle(x1-xh, y1-yh, x2+xh, y2+yh, x2-xh, y2-yh, colour);


	if (angle >= 180.0f)
		angle -= 360.0f;
	else if (angle <= -180.0f)
		angle += 360.0f;
	
	box_t box;
	if (angle <= 0.0f){
		box.x1 = x1 + xh;		// top left (at 0 degrees)
		box.y1 = y1 + yh;
		box.x2 = x2 - xh;		// bottom right
		box.y2 = y2 - yh;
		drawTriangle(x2, y2, box.x2, box.y2,  x2-xr, y2-yr, colour);
	}else{
		box.x1 = x1 + xh;		// top right (at 0 degrees)
		box.y1 = y1 + yh;
		box.x2 = x2 + xh;		// bottom left
		box.y2 = y2 + yh;
		drawTriangle(x2, y2, box.x2, box.y2,  x2+xr, y2+yr, colour);
	}
}

static inline void drawPolyFilled (float x1, float y1, float x2, float y2, const float thickness, float angle, const uint16_t colour)
{
	//if (!clipLinef(x1, y1, x2, y2, &x1, &y1, &x2, &y2))
	//	return;
		
	// cross product
	float x = (y1 * 1.0f) - (1.0f * y2);
	float y = (1.0f * x2) - (x1 * 1.0f);


	const float length = sqrtf((x * x) + (y * y));
	const float mul = (1.0 / length) * thickness;

	// set thickness;
	x *= mul ; //thickness * (1.0 / length);
	y *= mul ; //thickness * (1.0 / length);
	float xh = (x / 2.0f);	// center path by precomputing and using half thickness per side
	float yh = (y / 2.0f);


	float xr, yr;
	rotate(-DEG2RAD(angle), xh, yh, &xr, &yr);
	drawTriangleFilled(x1+xh, y1+yh, x1-xh, y1-yh, x2+xh, y2+yh, colour);
	drawTriangleFilled(x1-xh, y1-yh, x2+xh, y2+yh, x2-xh, y2-yh, colour);


	if (angle >= 180.0f)
		angle -= 360.0f;
	else if (angle <= -180.0f)
		angle += 360.0f;
	
	box_t box;
	if (angle <= 0.0f){
		box.x1 = x1 + xh;		// top left (at 0 degrees)
		box.y1 = y1 + yh;
		box.x2 = x2 - xh;		// bottom right
		box.y2 = y2 - yh;
		drawTriangleFilled(x2, y2, box.x2, box.y2,  x2-xr, y2-yr, colour);
	}else{
		box.x1 = x1 + xh;		// top right (at 0 degrees)
		box.y1 = y1 + yh;
		box.x2 = x2 + xh;		// bottom left
		box.y2 = y2 + yh;
		drawTriangleFilled(x2, y2, box.x2, box.y2,  x2+xr, y2+yr, colour);
	}
}

void drawPolyV3 (vector2_t *v1, vector2_t *v2, vector2_t *v3, const uint16_t thickness, const uint16_t colour)
{
	
	float x1 = (v1->y * 1.0f) - (1.0f * v2->y);
	float y1 = (1.0f * v2->x) - (v1->x * 1.0f);
	float x2 = (v2->y * 1.0f) - (1.0f *  v3->y);
	float y2 = (1.0f *  v3->x) - (v2->x * 1.0f);
	float a = RAD2DEG(atan2f(x2, y2) - atan2f(x1, y1));
		
	drawPoly(v1->x, v1->y, v2->x, v2->y, thickness, a, colour);
}

void drawPolyV3Filled (vector2_t *v1, vector2_t *v2, vector2_t *v3, const uint16_t thickness, const uint16_t colour)
{
	float x1 = (v1->y * 1.0f) - (1.0f * v2->y);
	float y1 = (1.0f * v2->x) - (v1->x * 1.0f);
	float x2 = (v2->y * 1.0f) - (1.0f *  v3->y);
	float y2 = (1.0f *  v3->x) - (v2->x * 1.0f);
	float a = RAD2DEG(atan2f(x2, y2) - atan2f(x1, y1));
		
	drawPolyFilled(v1->x, v1->y, v2->x, v2->y, thickness, a, colour);
}

static inline void drawBrushVector (vfont_t *ctx, const float x1, const float y1, const float x2, const float y2, const float x3, const float y3, const uint16_t colour)
{
	if (ctx->brush.size > 1.0f){
		if (ctx->brush.qMode == 1){
			const float d = distance(x1, y1, x2, y2);
			const float bmulX = (x2 - x1) / d;
			const float bmulY = (y2 - y1) / d;
			const float bsize = ctx->brush.size / 2.0f;
			const float bmul = ctx->brush.advanceMult;

			for (float i = 1.0f; i <= d; i += bmul){
				float x = (x1 + i * bmulX);
				float y = (y1 + i * bmulY);
				drawBrush(ctx, x, y, bsize, colour);
			}
		}else if (ctx->brush.qMode == 2){
			float i = 0.0f;
			float x = x1;
			float y = y1;
			const float d = distance(x1, y1, x2, y2);
			const float dx = x2 - x1;
			const float dy = y2 - y1;
			const float bmul = ctx->brush.advanceMult;
			const float bmulX = bmul * dx / d;
			const float bmulY = bmul * dy / d;
		
			while (distance(x, y, x2, y2) > bmul && isBetween(x1, y1, x, y, x2, y2)){
				i += 1.0f;
				x = (x1 + i * bmulX);
				y = (y1 + i * bmulY);
      
				drawBrush(ctx, x, y, ctx->brush.size / 2.0f, colour);
			}
		}
	}else{
		if (ctx->brush.size >= -1.0f){
			drawLine(x1, y1, x2, y2, colour);
		}else{
			vector2_t v1, v2, v3;
			v1.x = x1;
			v1.y = y1;
			v2.x = x2;
			v2.y = y2;
			v3.x = x3;
			v3.y = y3;
			drawPolyV3Filled(&v1, &v2, &v3, -ctx->brush.size, colour);
		}
	}
}

static inline void rotateZ (rotate_t *rot, const float x, const float y, float *xr, float *yr)
{
	*xr = x * rot->cos - y * rot->sin;
	*yr = x * rot->sin + y * rot->cos;
}

static inline void drawVector (vfont_t *ctx, float x1, float y1, float x2, float y2, float x3, float y3)
{

	if (ctx->renderOp == RENDEROP_NONE){
		drawBrushVector(ctx, x1, y1, x2, y2, x3, y3, ctx->brush.colour);
		return;

	}else{
		// transform to 0
		x1 -= ctx->x; x2 -= ctx->x;
		y1 -= ctx->y; y2 -= ctx->y;
	}
	

	if (ctx->renderOp&RENDEROP_SHEAR_X){
		x1 += y1 * ctx->shear.tan;
		x2 += y2 * ctx->shear.tan;
	}
	
	if (ctx->renderOp&RENDEROP_SHEAR_Y){
		y1 = (x1 * ctx->shear.sin + y1 * ctx->shear.cos) + 1.0f;
		y2 = (x2 * ctx->shear.sin + y2 * ctx->shear.cos) + 1.0f;
	}

	if (ctx->renderOp&RENDEROP_ROTATE_GLYPHS){
		//float x1r, y1r, x2r, y2r;

		// undo string transform
		x1 += ctx->x; x2 += ctx->x;
		y1 += ctx->y; y2 += ctx->y;

		// apply glyph transform
		x1 -= ctx->pos.x; x2 -= ctx->pos.x;
		y1 -= ctx->pos.y; y2 -= ctx->pos.y;

		rotateZ(&ctx->rotate.glyph, x1, y1, &x1, &y1);
		rotateZ(&ctx->rotate.glyph, x2, y2, &x2, &y2);

		x1 += ctx->pos.x - ctx->x; x2 += ctx->pos.x - ctx->x;
		y1 += ctx->pos.y - ctx->y; y2 += ctx->pos.y - ctx->y;

	}
	
	if (ctx->renderOp&RENDEROP_ROTATE_STRING){
		rotateZ(&ctx->rotate.string, x1, y1, &x1, &y1);
		rotateZ(&ctx->rotate.string, x2, y2, &x2, &y2);
	}

	//  transform back
	x1 += ctx->x; x2 += ctx->x;
	y1 += ctx->y; y2 += ctx->y;
	
	// now draw the brush
	drawBrushVector(ctx, x1, y1, x2, y2, x3, y3, ctx->brush.colour);

}

static inline float char2float (vfont_t *ctx, const uint8_t c)
{
	return ctx->scale.glyph * (float)(c - 'R');
}

static inline float drawGlyph (vfont_t *ctx, const hfont_t *font, const uint16_t c)
{

	if (c >= font->glyphCount) return 0.0f;
	
	const uint8_t *hc = (uint8_t*)font->glyphs[c];
	const float lm = char2float(ctx, *hc++) * fabsf(ctx->scale.horizontal);
	const float rm = char2float(ctx, *hc++) * fabsf(ctx->scale.horizontal);

	ctx->pos.x -= lm;

	float x1 = 0.0f;
	float y1 = 0.0f;
	float x2, y2, x3, y3;
	int newPath = 1;

	vector2_t start = {0};
	vector2_t startNext = {0};
	int ct = 0;
	
	while (*hc){
		if (*hc == ' '){
			hc++;
			newPath = 1;

		}else{
			const float x = char2float(ctx, *hc++) * ctx->scale.horizontal;
			const float y = char2float(ctx, *hc++) * ctx->scale.vertical;

			if (newPath){
				ct = 0;
				newPath = 0;
				start.x = x1 = ctx->pos.x + x;
				start.y = y1 = ctx->pos.y + y;

				//drawCircleFilled(x1, y1, 5_PAL_BLUE);	// path start
			}else{
				x2 = ctx->pos.x + x;
				y2 = ctx->pos.y + y;

				if (ct++ == 1){
					startNext.x = x2;
					startNext.y = y2;
				}
			
				if (*hc && *hc != ' '){
					x3 = ctx->pos.x + (char2float(ctx, *hc)     * ctx->scale.horizontal);
					y3 = ctx->pos.y + (char2float(ctx, *(hc+1)) * ctx->scale.vertical);
				}else{
					if (start.x == x2 && start.y == y2){
						x3 = startNext.x;
						y3 = startNext.y;
					}else{
						x3 = x1;
						y3 = y1;
					}
				}

				drawVector(ctx, x1, y1, x2, y2, x3, y3);
				x1 = x2;
				y1 = y2;
				//if (*hc == ' ' || !*hc) drawCircleFilled(x1, y1, 5_PAL_BLUE); // path end
			}
		}
	}

	ctx->pos.x += rm + ctx->xpad;
	return (rm - lm) + ctx->xpad;
}
#if 0
// returns horizontal glyph advance
static inline float drawGlyph_old (vfont_t *ctx, const hfont_t *font, const uint16_t c)
{

	if (c >= font->glyphCount) return 0.0;
	
	//const uint8_t *hc = (uint8_t*)font->glyphs[c].data;
	const uint8_t *hc = (uint8_t*)font->glyphs[c];
	const float lm = char2float(ctx, *hc++) * fabs(ctx->scale.horizontal);
	const float rm = char2float(ctx, *hc++) * fabs(ctx->scale.horizontal);
	
	ctx->pos.x -= lm;

	float x1 = 0.0;
	float y1 = 0.0;
	float x2, y2;
	int newPath = 1;
	

	while (*hc){
		if (*hc == ' '){
			hc++;
			newPath = 1;

		}else{
			const float x = char2float(ctx, *hc++) * ctx->scale.horizontal;
			const float y = char2float(ctx, *hc++) * ctx->scale.vertical;

			if (newPath){
				newPath = 0;
				x1 = ctx->pos.x + x;
				y1 = ctx->pos.y + y;
				//drawCircleFilled(x1, y1, 3, COLOUR_GREEN);	// path start
				
			}else{
				x2 = ctx->pos.x + x;
				y2 = ctx->pos.y + y;
				
				drawVector(ctx, x1, y1, x2, y2);
				x1 = x2;
				y1 = y2;
				//if (*hc == ' ' || !*hc) drawCircleFilled(x1, y1, 3, COLOUR_BLUE); // path end
			}
		}
	}

	ctx->pos.x += rm + ctx->xpad;
	return (rm - lm) + ctx->xpad;
}
#endif

// returns scaled glyph stride 
float getCharMetrics (vfont_t *ctx, const hfont_t *font, const uint16_t c, float *adv, box_t *box)
{

	if (c >= font->glyphCount) return 0.0;
	
	//const uint8_t *hc = (uint8_t*)font->glyphs[c].data;
	const uint8_t *hc = (uint8_t*)font->glyphs[c];
	const float lm = char2float(ctx, *hc++) * fabs(ctx->scale.horizontal);
	const float rm = char2float(ctx, *hc++) * fabs(ctx->scale.horizontal);

	float startX;
	if (adv)
		startX = *adv - lm;
	else
		startX = 0.0 - lm;
	
	
	float startY = 0.0;
	float miny = 9999.0;
	float maxy = -9999.0;
	float minx = 9999.0;
	float maxx = -9999.0;
	
	
	while (*hc){
		if (*hc == ' '){
			hc++;

		}else{
			const float x = char2float(ctx, *hc++);
			if (x > maxx) maxx = x;
			if (x < minx) minx = x;
			
			const float y = char2float(ctx, *hc++);
			if (y > maxy) maxy = y;
			if (y < miny) miny = y;
		}
	}
	
	minx *= fabs(ctx->scale.horizontal);
	maxx *= fabs(ctx->scale.horizontal);
	miny *= fabs(ctx->scale.vertical);
	maxy *= fabs(ctx->scale.vertical);
	
	float brushSize = ctx->brush.size / 2.0;
	box->x1 = (startX + minx) - brushSize;
	box->y1 = (startY + miny) - brushSize;
	box->x2 = (startX + maxx) + brushSize;
	box->y2 = (startY + maxy) + brushSize;

	if (adv) *adv = startX + rm + ctx->xpad;
	return startX + rm + ctx->xpad;
}

void getGlyphMetrics (vfont_t *ctx, const uint16_t c, int *w, int *h)
{
	box_t box = {0};
	getCharMetrics(ctx, ctx->font, c-32, NULL, &box);

	if (w) *w = ((box.x2 - box.x1) + 1.0f) + 0.5f;
	if (h) *h = ((box.y2 - box.y1) + 1.0f) + 0.5f;
}

void getStringMetrics (vfont_t *ctx, const char *text, box_t *box)
{
#if 0
	int x = ctx->x;
	int y = ctx->y;
#endif

	float miny = 9999.0f;
	float maxy = -9999.0f;
	float minx = 9999.0f;
	float maxx = -9999.0f;
	float adv = 0.0f;
	
	while (*text){
		getCharMetrics(ctx, ctx->font, (*text++)-32, &adv, box);
#if 0
		drawRectangle(x+box->x1, y+box->y1, x+box->x2, y+box->y2, COLOUR_BLUE);
#endif
		
		if (box->x1 < minx) minx = box->x1;
		if (box->x2 > maxx) maxx = box->x2;
		if (box->y1 < miny) miny = box->y1;
		if (box->y2 > maxy) maxy = box->y2;
	}

	box->x1 = minx;
	box->y1 = miny;
	box->x2 = maxx;
	box->y2 = maxy;
}

void drawString (vfont_t *ctx, const char *text, const int x, const int y)
{
	ctx->x = ctx->pos.x = x;
	ctx->y = ctx->pos.y = y;

	while (*text)
		drawGlyph(ctx, ctx->font, (*text++)-32);
}

int setBrushBitmap (vfont_t *ctx, const void *bitmap, const uint8_t width, const uint8_t height)
{
	ctx->brush.image.pixels = (uint8_t*)bitmap;
	ctx->brush.image.width = width;
	ctx->brush.image.height = height;
	
	return (bitmap && width && height);
}

void setFont (vfont_t *ctx, const hfont_t *font)
{
	if (font)
		ctx->font = font;
}

const hfont_t *getFont (vfont_t *ctx)
{
	return ctx->font;
}

int setBrush (vfont_t *ctx, const int brush)
{
	if (brush < BRUSH_TOTAL){
		int old = ctx->brush.type;
		ctx->brush.type = brush;
		return old;
	}
	return ctx->brush.type;
}


// calculate glyph stride
static inline void brushCalcAdvance (vfont_t *ctx)
{
	ctx->brush.advanceMult = (ctx->brush.size * ctx->brush.step) / 100.0f;
}

float setBrushSize (vfont_t *ctx, const float size)
{
	if (size >= 0.5f){
		float old = ctx->brush.size;
		ctx->brush.size = size;
		brushCalcAdvance(ctx);
		return old;
	}
	return ctx->brush.size;
}

// 0.5 to 100.0
void setBrushStep (vfont_t *ctx, const float step)
{
	if (step >= 0.5){
		ctx->brush.step = step;
		brushCalcAdvance(ctx);
	}
}

uint8_t getBrushQuality (vfont_t *ctx)
{
	return ctx->brush.qMode;
}

void setBrushQuality (vfont_t *ctx, const uint8_t qMode)
{
	ctx->brush.qMode = qMode;
}

float getBrushStep (vfont_t *ctx)
{
	return ctx->brush.step;
}

void setGlyphScale (vfont_t *ctx, const float scale)
{
	if (scale >= 0.1)
		ctx->scale.glyph = scale;
}

float getGlyphScale (vfont_t *ctx)
{
	return ctx->scale.glyph;
}

// Extra space added to every glyph. (can be 0 or minus)
void setGlyphPadding (vfont_t *ctx, const float pad)
{
	ctx->xpad = pad;
}

float getGlyphPadding (vfont_t *ctx)
{
	return ctx->xpad;
}

uint16_t setBrushColour (vfont_t *ctx, const uint16_t colour)
{
	uint16_t old = ctx->brush.colour;
	ctx->brush.colour = colour;
	return old;
}

uint16_t getBrushColour (vfont_t *ctx)
{
	return ctx->brush.colour;
}

void setRenderFilter (vfont_t *ctx, const uint32_t op)
{
	ctx->renderOp = op&0xFFFF;
}

uint32_t getRenderFilter (vfont_t *ctx)
{
	return ctx->renderOp;
}

// when rotation from horizontal, when enabled via setRenderFilter()
void setRotationAngle (vfont_t *ctx, const float rotGlyph, const float rotString)
{
	const float oldG = ctx->rotate.glyph.angle;
	ctx->rotate.glyph.angle = fmodf(rotGlyph, 360.0);

	if (oldG != ctx->rotate.glyph.angle){
		ctx->rotate.glyph.cos = cosf(DEG2RAD(ctx->rotate.glyph.angle));
		ctx->rotate.glyph.sin = sinf(DEG2RAD(ctx->rotate.glyph.angle));
	}
	
	const float oldS = ctx->rotate.string.angle;
	ctx->rotate.string.angle = fmodf(rotString, 360.0);
	
	if (oldS != ctx->rotate.string.angle){
		ctx->rotate.string.cos = cosf(DEG2RAD(ctx->rotate.string.angle));
		ctx->rotate.string.sin = sinf(DEG2RAD(ctx->rotate.string.angle));
	}
}

void setShearAngle (vfont_t *ctx, const float shrX, const float shrY)
{
	ctx->shear.angleX = shrX;
	ctx->shear.angleY = shrY;
	
	ctx->shear.tan = -tanf(DEG2RAD(ctx->shear.angleX));
	ctx->shear.cos = cosf(DEG2RAD(ctx->shear.angleY));
	ctx->shear.sin = sinf(DEG2RAD(ctx->shear.angleY));
}

void setAspect (vfont_t *ctx, const float hori, const float vert)
{
	if (hori >= 0.05 || hori <= -0.05)
		ctx->scale.horizontal = hori;
	
	if (vert >= 0.05 || vert <= -0.05)
		ctx->scale.vertical = vert;
}

FLASHMEM void vfont_init (vfont_t *ctx)
{
	memset(ctx, 0, sizeof(*ctx));

	setAspect(ctx, 1.0, 1.0);
	setGlyphPadding(ctx, -1.0);
	setGlyphScale(ctx, 1.0);
	setBrush(ctx, BRUSH_POINT);
	setBrushSize(ctx, 1.0);
	setBrushStep(ctx, 1.0);
	setBrushQuality(ctx, 2);
	setBrushColour(ctx, COLOUR_RED);
	setRenderFilter(ctx, RENDEROP_NONE);

}
