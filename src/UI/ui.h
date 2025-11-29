
#ifndef _UI_H_
#define _UI_H_


#define UI_WIDGET_ENABLED		1
#define UI_WIDGET_DISABLED		0


#define UI_IDBASE					100
#define UI_ID_PANEL_MAPCTRL			(UI_IDBASE+1)
#define UI_ID_BUTTON_START			(UI_IDBASE+2)
#define UI_ID_BUTTON_STOP			(UI_IDBASE+3)
#define UI_ID_BUTTON_PAUSE			(UI_IDBASE+4)
#define UI_ID_BUTTON_HOTSTART		(UI_IDBASE+5)




typedef struct {
	uint8_t id;
	uint8_t colour;				// background/bass fill colour_pal_. 0xFF for no colour op
	uint16_t isEnabled:1;
	uint16_t stub:15;
}ui_widget_t;

typedef struct {
	ui_widget_t widget;

	struct {				// location and size within panel (local to panel)
		uint16_t x;
		uint16_t y;
		uint16_t width;		// includes touch hit test
		uint16_t height;
	}rect;
		
	struct {
		int16_t x;			// offset of string/image from within .rect
		int16_t y;			// can be negative
	}offset;

	struct {
		const char *text;
		uint8_t colour;			// COLOUR_PAL_		
		uint8_t scale;			// vFont scale * 10
		uint8_t size;			// vFont size * 10
		uint8_t quality;		// vFont quality
		uint32_t renderFlags;	// vFont flags
	}label;
	
	struct {
		uint8_t *image;		// array of COLOUR_PAL_
		uint16_t width;		// clipped to rect.width/height
		uint16_t height;
	}image;
	
	struct {
		void *opaque;
		void (*func) (uint8_t id, void *opaque, uint32_t flags);
	}callback;
}ui_button_t;

typedef struct {
	ui_widget_t widget;

	struct {				// location and size of panel
		uint16_t x;
		uint16_t y;
		uint16_t width;		// includes touch hit test
		uint16_t height;
	}rect;
		
	ui_button_t buttons[4];
}ui_panel_t;


#endif

