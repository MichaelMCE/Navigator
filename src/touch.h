#pragma once
#ifndef _TOUCH_H_
#define _TOUCH_H_

#if ENABLE_TOUCH_FT5216


enum _touchdir {		// touch rotate direction
	TOUCH_DIR_NONE = 1, // don't rotate
	TOUCH_DIR_DEFAULT,	// use compiled in rotation
	TOUCH_DIR_LRTB,		// left right top bottom
	TOUCH_DIR_LRBT,		// left right bottom top		
	TOUCH_DIR_RLTB,		// right left top bottom
	TOUCH_DIR_RLBT,		// right left bottom top
	TOUCH_DIR_TBLR,		// top bottom left right
	TOUCH_DIR_BTLR,		// bottom top left right
	TOUCH_DIR_TBRL,		// top bottom right left
	TOUCH_DIR_BTRL,		// bottom top right left
	
	TOUCH_DIR_SWAP_A_INVERT_V,	// swap axis then invert vertical axis
	TOUCH_DIR_SWAP_A_INVERT_H,	// swap axis then invert horizontal axis
};

typedef struct _touch {
	uint8_t idx;		// points to which multi point register we wish to read
	uint8_t flags;		// RAWHID_OP_TOUCH_xxx
	uint8_t tPoints;	// number of points (fingers) measured on panel this scan
	uint8_t direction;	// TOUCH_DIR_xx. touch rotation direction
	
	uint32_t time;
	
	uint16_t x;
	uint16_t y;
	
	struct {
		uint16_t x;
		uint16_t y;
	}points[10];

	uint8_t xh;
	uint8_t xl;
	uint8_t yh;
	uint8_t yl;
}touch_t;


void touch_start (const int intPin);
int touch_isPressed ();
int touch_process (touch_t *touch, const uint32_t rotateDirection);


#endif



#endif
