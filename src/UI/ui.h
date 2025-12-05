
#ifndef _UI_H_
#define _UI_H_


#define UI_WIDGET_ENABLED			1
#define UI_WIDGET_DISABLED			0


enum _ui_ids {
	UI_IDBASE				=		100,
	UI_ID_PANEL_MAPCTRL,
	UI_ID_BUTTON_START,
	UI_ID_BUTTON_STOP,
	UI_ID_BUTTON_PAUSE,
	UI_ID_BUTTON_HOTSTART,
	UI_ID_BUTTON_RESET,
	UI_ID_BUTTON_CONFIG
};


#define UI_WIDGET_PANEL				10
#define UI_WIDGET_BUTTON			20

#define WIDGET(w)					((ui_widget_t*)(w))
#define CHILD_WIDGET_OBJ(o,n)		((ui_widget_t*)&WIDGET(o)->children.widgets[(n)]);
#define CHILD_WIDGET_BUTTON(o,n)	((ui_button_t*)&WIDGET(o)->children.widgets[(n)]);



typedef struct _ui_widget_t {
	uint8_t id;
	uint8_t type;				// UI_WIDGET_
	uint16_t isEnabled:1;
	uint16_t stub:15;
	
	struct {
		uint8_t total;
		uint8_t stub[3];
		_ui_widget_t *widgets;
	}children;
	
	_ui_widget_t *parent;		// ui_panel_t, or NULL of root
}ui_widget_t;


typedef struct {
	ui_widget_t widget;

	struct {				// location and size within parent (local to parent)
		uint16_t x;
		uint16_t y;
		uint16_t width;		// includes touch hit test
		uint16_t height;
	}rect;
		
	struct {
		int16_t x;			// offset of string/image from within .rect
		int16_t y;			// can be negative. is not hit-tested
	}offset;

	struct {
		const char *text;
		uint8_t colour;			// COLOUR_PAL_		
		uint8_t scale;			// vFont scale * 10
		uint8_t size;			// vFont size * 10
		uint8_t quality;		// vFont quality
		//uint32_t renderFlags;	// vFont flags
	}label;
	
	struct {
		uint8_t *pixels;		// array of COLOUR_PAL_
		uint16_t width;			// clipped to rect.width/height
		uint16_t height;
	}image;
	
	struct {
		void *opaque;
		void (*func) (void *opaque, uint8_t id, uint32_t flags);
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
	
	struct {
		void *opaque;
		void (*func) (void *opaque, uint8_t id, uint32_t flags);
	}callback;
}ui_panel_t;



// returns 1 if successful, 0 if you're an idiot
FLASHMEM int ui_enable (void *opaque, const uint8_t child_id);
FLASHMEM int ui_disable (void *opaque, const uint8_t child_id);
FLASHMEM int ui_isEnabled (void *opaque, const uint8_t child_id);





enum _opcodes {
	OP_IDLE				= 0,
	OP_READY			= 1,
	OP_FUNC_LOG_START	= 100,
	OP_FUNC_LOG_STOP,
	OP_FUNC_LOG_PAUSE,
	OP_FUNC_LOG_RESET,
	OP_FUNC_HOTSTART,
	OP_FUNC_CONFIG
};

#define OP_QUEUE_LENGTH		32






#endif

