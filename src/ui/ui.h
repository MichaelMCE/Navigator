
#ifndef _UI_H_
#define _UI_H_

#define UI_DRAW_TOUCH_RECTS			0		// draw button's touch bounding box

#define UI_WIDGET_ENABLED			1
#define UI_WIDGET_DISABLED			0


enum _ui_ids {
	UI_IDBASE				=		100,
	UI_ID_PANEL_MENU,
	UI_ID_PANEL_RECEIVER,
	UI_ID_PANEL_RECEIVER_RESTART,
	UI_ID_PANEL_RECEIVER_RATE,
	UI_ID_PANEL_RECEIVER_GNSS,
	UI_ID_PANEL_MPU,
	UI_ID_PANEL_DISPLAY,
	UI_ID_PANEL_MAP,
	UI_ID_PANEL_LOGCTRL,
	UI_ID_PANEL_LOGS,

	UI_ID_BUTTON_empty,
	
	UI_ID_BUTTON_AREAFILL,
	UI_ID_BUTTON_AREAOUTLINR,
	UI_ID_BUTTON_PATHFILL,
	UI_ID_BUTTON_FATHLINE,
	UI_ID_BUTTON_TITLEBOUND,
	UI_ID_BUTTON_SCHEME,
	
	UI_ID_BUTTON_START,
	UI_ID_BUTTON_STOP,
	UI_ID_BUTTON_PAUSE,
	UI_ID_BUTTON_RESET,
	
	UI_ID_BUTTON_LOGS,
	UI_ID_BUTTON_DISPLAY,
	UI_ID_BUTTON_MAP,
	UI_ID_BUTTON_RECEIVER,
	UI_ID_BUTTON_MPU,
		
	UI_ID_BUTTON_DRAW_TEXT,
	UI_ID_BUTTON_DRAW_SATW,
	UI_ID_BUTTON_DRAW_AVAIL,
	UI_ID_BUTTON_DRAW_LVLS,
	UI_ID_BUTTON_DRAW_COMP,
	UI_ID_BUTTON_DRAW_RULER,
	UI_ID_BUTTON_DRAW_LOC,
	UI_ID_BUTTON_DRAW_ROUTE,
	UI_ID_BUTTON_OFF,
		
	UI_ID_BUTTON_REBOOT,
	UI_ID_BUTTON_FREQ528,
	UI_ID_BUTTON_FREQ600,
	UI_ID_BUTTON_FREQ720,
	UI_ID_BUTTON_FREQ816,
	
	UI_ID_BUTTON_HOTSTART,
	UI_ID_BUTTON_WARMSTART,
	UI_ID_BUTTON_COLDSTART,
	
	UI_ID_BUTTON_GPS_RESTART,
	UI_ID_BUTTON_REINIT,
	UI_ID_BUTTON_RECONNECT,
	UI_ID_BUTTON_STATUS,
	UI_ID_BUTTON_VERSION,
	UI_ID_BUTTON_RATE,
	UI_ID_BUTTON_GNSS,
	
	UI_ID_BUTTON_RATE1,
	UI_ID_BUTTON_RATE2,
	UI_ID_BUTTON_RATE3,
	UI_ID_BUTTON_RATE4,
	UI_ID_BUTTON_RATE5,
	UI_ID_BUTTON_RATE6,
	UI_ID_BUTTON_RATE7,
	
	UI_ID_BUTTON_GPS,
	UI_ID_BUTTON_GALILEO,
	UI_ID_BUTTON_BEIDOU,
	UI_ID_BUTTON_GLONASS,
	UI_ID_BUTTON_SBAS,
	UI_ID_BUTTON_QZSS,
	UI_ID_BUTTON_IMES,

	UI_ID_BUTTON_CONFIG,
	UI_ID_BUTTON_OVERLAYDETAIL,
	UI_ID_BUTTON_LOGCTRL,
	UI_ID_BUTTON_ZOOM_IN,
	UI_ID_BUTTON_ZOOM_OUT,
	UI_ID_BUTTON_REFRESH,
	UI_ID_BUTTON_UP,
	UI_ID_BUTTON_DOWN,
	UI_ID_BUTTON_LOAD
};


#define UI_WIDGET_PANEL				10
#define UI_WIDGET_BUTTON			20

#define UI_WIDGET_FLAG_HASPANEL		0x0001

#define UI_WIDGET_MSG_INPUT			0x01
#define UI_WIDGET_MSG_RENDER		0x02


#define WIDGET(w)					((ui_widget_t*)(w))
#define CHILDREN(w)					((ui_widget_t**)&(w))
#define CHILD_WIDGET_OBJ(o,n)		((ui_widget_t*)WIDGET(o)->children.widgets[(n)]);


typedef struct ui_widget_t ui_widget_t;

struct ui_widget_t {
	uint8_t id;
	uint8_t type;				// UI_WIDGET_
	
	uint16_t isEnabled:1;		// render and accept input
	uint16_t isNotReady:1;		// button is enabled & disablable but not accepting input
	uint16_t isHighlighted:1;
	uint16_t stub:5;
	uint16_t flags:8;			// is a button that'll open a menu panel;
	
	struct {
		uint8_t total;
		uint8_t stub[3];
		ui_widget_t **widgets;
	}children;
	
	ui_widget_t *parent;		// ui_panel_t, or NULL of root
};


typedef int (*ui_widget_cb_t) (ui_widget_t *opaque, const uint8_t id, const uint32_t flags, const uint32_t msg, const int32_t var1, const int32_t var2);



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
	
	int8_t buttonHeight;
	int8_t buttonX;
	int8_t buttonY;
	int8_t stub;
}ui_panel_t;


void ui_init ();
int ui_disable (void *opaque, const uint8_t child_id);
int ui_enable (void *opaque, const uint8_t child_id);
int ui_enableReady (void *opaque, const uint8_t child_id);
int ui_enableNotReady (void *opaque, const uint8_t child_id);	// enabled, rendered but with input disabled
int ui_setHighlight (const uint8_t child_id, const uint8_t state);
int ui_getHighlight (const uint8_t child_id);
int ui_isEnabled (void *opaque, const uint8_t child_id);
void ui_draw (const uint32_t unused1, const uint32_t unused2);
int ui_input (const int32_t x, const int32_t y, const uint32_t flags);

FLASHMEM void uiBuild ();

void uiLogs_clear ();





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
	OP_FUNC_STATUS,
	OP_FUNC_VERSION,
	
	OP_FUNC_ZOOMOUT,
	OP_FUNC_ZOOMIN,
	OP_FUNC_ZOOMRESET,

	OP_FUNC_LOG_OPEN,		// ui panel
	OP_FUNC_LOG_CLOSE,
	OP_FUNC_LOG_LOAD,		// import log file
	OP_FUNC_LOG_UP,
	OP_FUNC_LOG_DOWN,
	
	OP_FUNC_RECEIVER_RATE,
	
	OP_FUNC_CONFIG,
	OP_FUNC_OVERLAYDETAIL,
};

#define OP_QUEUE_LENGTH		32


uint32_t op_state ();
uint32_t op_pop ();
void op_go ();
uint32_t op_push (const uint32_t op_code);
int32_t op_execute (const uint32_t opCode);



#endif

