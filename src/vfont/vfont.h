
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


#ifndef COLOUR_24TO16
#define COLOUR_24TO16(c)		((((c>>16)&0xF8)<<8) | (((c>>8)&0xFC)<<3) | ((c&0xF8)>>3))
#endif

#ifdef RGB
#undef RGB
#endif
#define RGB(r,g,b)				(((r)<<16)|((g)<<8)|(b))


// LFRM_BPP_16 - RGB 565
#define RGB_16_RED		0xF800
#define RGB_16_GREEN	0x07E0
#define RGB_16_BLUE		0x001F	
#define RGB_16_WHITE	(RGB_16_RED|RGB_16_GREEN|RGB_16_BLUE)
#define RGB_16_BLACK	0x0000
#define RGB_16_MAGENTA	(RGB_16_RED|RGB_16_BLUE)
#define RGB_16_YELLOW	(RGB_16_RED|RGB_16_GREEN)
#define RGB_16_CYAN		(RGB_16_GREEN|RGB_16_BLUE)


#define COLOUR_RED				RGB_16_RED
#define COLOUR_GREEN			RGB_16_GREEN
#define COLOUR_BLUE				RGB_16_BLUE
#define COLOUR_WHITE			RGB_16_WHITE
#define COLOUR_BLACK			RGB_16_BLACK
#define COLOUR_MAGENTA			RGB_16_MAGENTA
#define COLOUR_YELLOW			RGB_16_YELLOW
#define COLOUR_CYAN				RGB_16_CYAN

#define COLOUR_CREAM			(COLOUR_24TO16(0xEEE7D0))
#define COLOUR_AQUA				(COLOUR_24TO16(0x00B7EB))
#define COLOUR_ORANGE			(COLOUR_24TO16(0xFF7F11))
#define COLOUR_BLUE_SEA_TINT	(COLOUR_24TO16(0x508DC5))		/* blue, but not too dark nor too bright. eg; Glass Lite:Volume */
#define COLOUR_GREEN_TINT		(COLOUR_24TO16(0x00FF1E))		/* softer green. used for highlighting */
#define COLOUR_PURPLE_GLOW		(COLOUR_24TO16(0xFF10CF))
#define COLOUR_GRAY				(COLOUR_24TO16(0x777777))
#define COLOUR_HOVER			(COLOUR_24TO16(0x28C672))
#define COLOUR_TASKBARFR		(COLOUR_24TO16(0x141414))
#define COLOUR_TASKBARBK		(COLOUR_24TO16(0xD4CAC8))
#define COLOUR_SOFTBLUE			(COLOUR_24TO16(0X7296D3))
#define COLOUR_PINK				(COLOUR_24TO16(RGB(248,184,128)))




enum _pal {
	COLOUR_PAL_BLACK,

	COLOUR_PAL_LIGHTGREY,
	COLOUR_PAL_LIGHTERGREY,
	COLOUR_PAL_DARKGREY,
	COLOUR_PAL_GREY,
	COLOUR_PAL_GRAY = COLOUR_PAL_GREY,
	
	COLOUR_PAL_DARKERGREEN,
	COLOUR_PAL_DARKGREEN,   
	COLOUR_PAL_LIGHTGREEN,
	COLOUR_PAL_BRIGHTGREEN,
	COLOUR_PAL_GREEN,
	COLOUR_PAL_GREENHUE,
		
	COLOUR_PAL_AQUA,
	COLOUR_PAL_PINK,
	
	COLOUR_PAL_HOVER,
	
	COLOUR_PAL_BLUE_SEA,
	COLOUR_PAL_SOFTBLUE,
	COLOUR_PAL_CYAN,
	COLOUR_PAL_BLUE,
	COLOUR_PAL_DARKBLUE,
		
	COLOUR_PAL_HOMER,
	COLOUR_PAL_YELLOW,

	COLOUR_PAL_RED,
	COLOUR_PAL_REDISH,
	COLOUR_PAL_MAGENTA,
	COLOUR_PAL_REDFUZZ,
	
	COLOUR_PAL_ORANGE,

	COLOUR_PAL_DARKBROWN,
	COLOUR_PAL_DARKERBROWN,
	COLOUR_PAL_LIGHTBROWN,
	COLOUR_PAL_BROWN,

	COLOUR_PAL_BLUE_SEA_TINT,
	COLOUR_PAL_GREEN_TINT,
	COLOUR_PAL_PURPLE_GLOW,	
	COLOUR_PAL_TASKBARFR,
	COLOUR_PAL_TASKBARBK,
	
	COLOUR_PAL_BACKGROUND,
	COLOUR_PAL_WATER,

	COLOUR_PAL_CREAM,
	COLOUR_PAL_WHITE,

	COLOUR_PAL_MAROON,
	COLOUR_PAL_GOLD,
	
	COLOUR_PAL_PL08,
	COLOUR_PAL_PL09,
	COLOUR_PAL_PL0B,
	COLOUR_PAL_PL0D,
	COLOUR_PAL_PL0E,
	COLOUR_PAL_PL16,
	COLOUR_PAL_PL1A,

	COLOUR_PAL_PG_01,
	COLOUR_PAL_PG_02,
	COLOUR_PAL_PG_03,
	COLOUR_PAL_PG_04,
	COLOUR_PAL_PG_05,
	COLOUR_PAL_PG_06,
	COLOUR_PAL_PG_07,
	COLOUR_PAL_PG_08,
	COLOUR_PAL_PG_09,
	COLOUR_PAL_PG_0A,
	COLOUR_PAL_PG_0B,
	COLOUR_PAL_PG_0C,
	COLOUR_PAL_PG_0D,
	COLOUR_PAL_PG_0E,
	COLOUR_PAL_PG_11,
	COLOUR_PAL_PG_12,
	COLOUR_PAL_PG_13,
	COLOUR_PAL_PG_14,
	COLOUR_PAL_PG_15,
	COLOUR_PAL_PG_16,
	COLOUR_PAL_PG_17,
	COLOUR_PAL_PG_4F,
	COLOUR_PAL_PG_51,

	COLOUR_PAL_PLYEDGE,
	
	COLOUR_PAL_TOTAL,
	PALETTE_TOTAL = COLOUR_PAL_TOTAL
};

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
uint16_t setBrushColour (vfont_t *ctx, const uint16_t colour);
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
void drawBitmap (image_t *img, int x, int y, const uint16_t colour);
void drawRectangle (int x1, int y1, int x2, int y2, const uint16_t colour);
void drawRectangleFilled (int x1, int y1, int x2, int y2, const uint16_t colour);
void drawTriangle (const int x1, const int y1, const int x2, const int y2, const int x3, const int y3, const uint16_t colour);
void drawTriangleFilled (int x0, int y0, int x1, int y1, int x2, int y2, const uint16_t colour);
void drawCircle (const int xc, const int yc, const float radius, const uint16_t colour);
void drawCircleFilled (int x0, const int y0, const float radius, const uint16_t colour);
void drawLine (const int x1, const int y1, const int x2, const int y2, const uint16_t colour);
void drawFillArc (const uint16_t x, const uint16_t y, const float radius, const float thickness, const float start, const float end, const uint16_t colour);
void drawTriangleFilled3 (int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3, const uint16_t colour);
void drawLineV (int32_t x, int32_t y1, int32_t y2, const uint16_t colour);
void drawLineH (int32_t y, int32_t x1, int32_t x2, const uint16_t colour);


typedef struct {
	float y;
	float x;
}vector2_t;

typedef struct {
	int16_t x;
	int16_t y;
}v16_t;


void drawPolygon (v16_t *verts, const int32_t total, const uint16_t colour);
void drawPolyline (const float x1, const float y1, const float x2, const float y2, const uint16_t colour);
void drawPolyV3 (vector2_t *v1, vector2_t *v2, vector2_t *v3, const uint16_t thickness, const uint16_t colour);
void drawPolyV3Filled (vector2_t *v1, vector2_t *v2, vector2_t *v3, const uint16_t thickness, const uint16_t colour);
void drawPolylineSolid (const float x1, const float y1, const float x2, const float y2, const float thickness, const uint16_t colour);

const inline float cosDegrees (const float angle)
{
	return cosf(angle * DEG_TO_RAD);
}

const inline float sinDegrees (const float angle)
{
	return sinf(angle * DEG_TO_RAD);
}

const void drawPixel (const int x, const int y, const uint16_t colourIdx);



#endif


