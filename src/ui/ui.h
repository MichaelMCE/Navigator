
#ifndef _UI_H_
#define _UI_H_


#define UI_WIDGET_ENABLED			1
#define UI_WIDGET_DISABLED			0


enum _ui_ids {
	UI_IDBASE				=		100,
	UI_ID_PANEL_MAPCTRL,
	UI_ID_PANEL_RECEIVER,
	UI_ID_PANEL_MPU,
	UI_ID_PANEL_DISPLAY,

	UI_ID_BUTTON_empty,
	
	UI_ID_BUTTON_START,
	UI_ID_BUTTON_STOP,
	UI_ID_BUTTON_PAUSE,
	UI_ID_BUTTON_RESET,
	UI_ID_BUTTON_DISPLAY,
	UI_ID_BUTTON_RECEIVER,
	UI_ID_BUTTON_MPU,
		
	UI_ID_BUTTON_MODE,
	UI_ID_BUTTON_OFF,
		
	UI_ID_BUTTON_REBOOT,
	UI_ID_BUTTON_FREQ528,
	UI_ID_BUTTON_FREQ600,
	UI_ID_BUTTON_FREQ720,
	UI_ID_BUTTON_FREQ816,
	
	UI_ID_BUTTON_HOTSTART,
	UI_ID_BUTTON_WARMSTART,
	UI_ID_BUTTON_COLDSTART,
	UI_ID_BUTTON_REINIT,
	UI_ID_BUTTON_RECONNECT,
	
	UI_ID_BUTTON_CONFIG
};


#define UI_WIDGET_PANEL				10
#define UI_WIDGET_BUTTON			20

#define WIDGET(w)					((ui_widget_t*)(w))
#define CHILDREN(w)					((ui_widget_t**)&(w))
#define CHILD_WIDGET_OBJ(o,n)		((ui_widget_t*)WIDGET(o)->children.widgets[(n)]);



typedef struct _ui_widget_t {
	uint8_t id;
	uint8_t type;				// UI_WIDGET_
	uint16_t isEnabled:1;
	uint16_t stub:15;
	
	struct {
		uint8_t total;
		uint8_t stub[3];
		_ui_widget_t **widgets;
	}children;
	
	_ui_widget_t *parent;		// ui_panel_t, or NULL of root
}ui_widget_t;


typedef int (*ui_widget_cb_t) (ui_widget_t *opaque, const uint8_t id, const uint32_t flags);



typedef struct {
	void *opaque;
	ui_widget_cb_t func;
}ui_callback_t;

typedef struct {		// location and size within parent (local to parent)
	uint16_t x;
	uint16_t y;
	uint16_t width;		// includes touch hit test
	uint16_t height;
}ui_rect_t;

typedef struct {
	ui_widget_t widget;
	ui_rect_t rect;
	ui_callback_t callback;
}ui_all_t;

typedef struct {
	ui_widget_t widget;
	ui_rect_t rect;
	ui_callback_t callback;
	
	struct {
		int16_t x;				// offset of string/image from within .rect
		int16_t y;				// can be negative. is not hit-tested
	}offset;

	struct {
		const char *text;
		uint8_t colour;			// COLOUR_PAL_		
		uint8_t scale;			// vFont scale * 10
		uint8_t quality;		// vFont quality
		uint8_t stub;

		float size;				// vFont size * 10

		//uint32_t renderFlags;	// vFont flags
	}label;
	
	struct {
		uint8_t *pixels;		// array of COLOUR_PAL_
		uint16_t width;			// clipped to rect.width/height
		uint16_t height;
	}image;
}ui_button_t;

typedef struct {
	ui_widget_t widget;
	ui_rect_t rect;
	ui_callback_t callback;

}ui_panel_t;


void ui_init ();

// returns 1 if successful, 0 if you're an idiot
int ui_enable (void *opaque, const uint8_t child_id);
int ui_disable (void *opaque, const uint8_t child_id);
int ui_isEnabled (void *opaque, const uint8_t child_id);
void ui_draw (const uint32_t unused1, const uint32_t unused2);
int ui_input (const int32_t x, const int32_t y, const uint32_t flags);

FLASHMEM void uiBuild ();





enum _opcodes {
	OP_IDLE				= 0,
	OP_READY			= 1,
	OP_FUNC_LOG_START	= 100,
	OP_FUNC_LOG_STOP,
	OP_FUNC_LOG_PAUSE,
	OP_FUNC_LOG_RESET,
	
	OP_FUNC_MODE,
	OP_FUNC_OFF,
	
	OP_FUNC_REBOOT,
	OP_FUNC_FREQ528,
	OP_FUNC_FREQ600,
	OP_FUNC_FREQ720,
	OP_FUNC_FREQ816,
	
	OP_FUNC_HOTSTART,
	OP_FUNC_WARMSTART,
	OP_FUNC_COLDSTART,
	OP_FUNC_REINIT,
	OP_FUNC_RECONNECT,
	
	OP_FUNC_CONFIG
};

#define OP_QUEUE_LENGTH		16


uint32_t op_state ();
uint32_t op_pop ();
int32_t op_execute (const uint32_t opCode);

#endif

