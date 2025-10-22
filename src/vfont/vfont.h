
#ifndef _VFONT_H_
#define _VFONT_H_


#include "display.h"
#include "hfont.h"
#include "../config.h"


#define VWIDTH		(TFT_WIDTH)
#define VHEIGHT		(TFT_HEIGHT)



//#define LINE_STD			1		// slowest: standard Bresenham algorithm
//#define LINE_FAST			1		// fast: similar to Bresenham but forgoes accuracy for performance
//#define LINE_FASTEST16	1		// faster: render direct to a 16bit buffer. about 3x faster than LINE_STD
#define LINE_FASTEST8		1		// fastest: render direct to 8bit buffer, with a palette.



enum _brush {
	BRUSH_POINT,			// .
	BRUSH_CIRCLE,			// O
	BRUSH_CIRCLE_FILLED,
	BRUSH_SQUARE,		
	BRUSH_SQUARE_FILLED,	
	BRUSH_TRIANGLE,			// north facing
	BRUSH_TRIANGLE_FILLED,	
	BRUSH_STROKE_1,			// slope up		/	
	BRUSH_STROKE_2,			// slope down	\ symbol
	BRUSH_STROKE_3,			// horizontal	-	
	BRUSH_STROKE_4,			// vertical		|	
	BRUSH_STROKE_5,			// 
	BRUSH_STROKE_6,			// 
	BRUSH_STROKE_7,
	BRUSH_STAR,				// *
	BRUSH_X,				// X
	BRUSH_CARET,			// ^
	BRUSH_PLUS,				// +
	BRUSH_BITMAP,

	BRUSH_TOTAL,
	BRUSH_DISK = BRUSH_CIRCLE_FILLED		
};



#define RENDEROP_NONE			0x00
#define RENDEROP_SHEAR_X		0x01
#define RENDEROP_SHEAR_Y		0x02
#define RENDEROP_SHEAR_XYX		0x04
#define RENDEROP_ROTATE_STRING	0x08
#define RENDEROP_ROTATE_GLYPHS	0x10


#ifndef DEG2RAD
#define DEG2RAD(a)				((a)*0.017453292519943295769236907684886)
#endif

#ifndef RAD2DEG
#define RAD2DEG(a) 				((a)*57.295779513082320876798154814105)
#endif


#define CALC_PITCH_1(w)			(((w)>>3)+(((w)&0x07)!=0))	// 1bit packed, calculate number of storage bytes per row given width (of glyph)
#define CALC_PITCH_16(w)		((w)*sizeof(uint16_t))		// 16bit, 8 bits per byte


typedef struct{
	float x1;
	float y1;
	float x2;
	float y2;
}box_t;


typedef struct {
	float angle;
	float cos;
	float sin;
}rotate_t;

typedef struct {
	const uint8_t *pixels;
	uint8_t width;
	uint8_t height;
	uint8_t stubUnused;
}image_t;
		
typedef struct {
	const hfont_t *font;
	int x;				// initial rendering position
	int y;
	
	struct {			// current rendering position
		float x;
		float y;
	}pos;

	float xpad;			// add horizontal padding. can be minus. eg; -0.5f

	struct {
		float glyph;		// vector scale/glyph size (2.0 = float size glyph)
		float horizontal;
		float vertical;
	}scale;

	struct {
		rotate_t string;
		rotate_t glyph;
	}rotate;
	
	struct {
		float angleX;
		float angleY;
		float cos;
		float sin;
		float tan;
	}shear;

	struct {
		float size;
		float step;
		float advanceMult;
		uint16_t colour;
		
		uint8_t type;		// BRUSH_
		uint8_t qMode;		// quality mode

		image_t image;
	}brush;
	
	uint16_t renderOp;
}vfont_t;



#include "fonts.h"
#include "brushes.h"


void vfont_init (vfont_t *ctx);
void setAspect (vfont_t *ctx, const float hori, const float vert);
void setShearAngle (vfont_t *ctx, const float shrX, const float shrY);
void setFont (vfont_t *ctx, const hfont_t *font);
int setBrushBitmap (vfont_t *ctx, const void *bitmap, const uint8_t width, const uint8_t height);
const hfont_t *getFont (vfont_t *ctx);
int setBrush (vfont_t *ctx, const int brush);
float setBrushSize (vfont_t *ctx, const float size);
void setBrushStep (vfont_t *ctx, const float step);
float getBrushStep (vfont_t *ctx);
void setGlyphScale (vfont_t *ctx, const float scale);
float getGlyphScale (vfont_t *ctx);
void setGlyphPadding (vfont_t *ctx, const float pad);
float getGlyphPadding (vfont_t *ctx);
uint8_t setBrushColour (vfont_t *ctx, const uint8_t colour);
void setRenderFilter (vfont_t *ctx, const uint32_t op);
uint32_t getRenderFilter (vfont_t *ctx);

void setBrushQuality (vfont_t *ctx, const uint8_t qMode);		// 1= lower qualirt for fast, 2= high quality for slow performing
uint8_t getBrushQuality (vfont_t *ctx);

void setRotationAngle (vfont_t *ctx, const float rotGlyph, const float rotString);
void setShearAngle (vfont_t *ctx, const float shrX, const float shrY);

float getCharMetrics (vfont_t *ctx, const hfont_t *font, const uint16_t c, float *adv, box_t *box);
void getGlyphMetrics (vfont_t *ctx, const uint16_t c, int *w, int *h);
void getStringMetrics (vfont_t *ctx, const char *text, box_t *box);

void drawString (vfont_t *ctx, const char *text, const int x, const int y);


// primitives, one should never be compelled to use these
void drawBitmap (image_t *img, int x, int y, const uint8_t colour);
void drawRectangle (int x1, int y1, int x2, int y2, const uint8_t colour);
void drawRectangleFilled (int x1, int y1, int x2, int y2, const uint8_t colour);
void drawTriangle (const int x1, const int y1, const int x2, const int y2, const int x3, const int y3, const uint8_t colour);
void drawTriangleFilled (int x0, int y0, int x1, int y1, int x2, int y2, const uint8_t colour);
void drawCircle (const int xc, const int yc, const float radius, const uint8_t colour);
void drawCircleFilled (int x0, const int y0, const float radius, const uint8_t colour);
void drawLine (const int x1, const int y1, const int x2, const int y2, const uint8_t colour);
void drawFillArc (const uint16_t x, const uint16_t y, const float radius, const float thickness, const float start, const float end, const uint8_t colour);
void drawTriangleFilled3 (int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, const uint8_t colour);
void drawLineV (int32_t x, int32_t y1, int32_t y2, const uint8_t colour);
void drawLineH (int32_t y, int32_t x1, int32_t x2, const uint8_t colour);


typedef struct {
	float y;
	float x;
}vector2_t;

typedef struct {
	int16_t x;
	int16_t y;
}v16_t;


void drawPolygon (v16_t *verts, const int32_t total, const uint8_t colour);
void drawPolyline (const float x1, const float y1, const float x2, const float y2, const uint8_t colour);
void drawPolyV3 (vector2_t *v1, vector2_t *v2, vector2_t *v3, const uint16_t thickness, const uint8_t colour);
void drawPolyV3Filled (vector2_t *v1, vector2_t *v2, vector2_t *v3, const uint16_t thickness, const uint8_t colour);
void drawPolylineSolid (const float x1, const float y1, const float x2, const float y2, const float thickness, const uint8_t colour);

const inline float cosDegrees (const float angle)
{
	return cosf(angle * DEG_TO_RAD);
}

const inline float sinDegrees (const float angle)
{
	return sinf(angle * DEG_TO_RAD);
}

//const void drawPixel (const int x, const int y, const uint16_t colourIdx);



#endif


