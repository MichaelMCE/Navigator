


#include "../commonGlue.h"




extern trackRecord_t trackRecord;
extern application_t inst;
extern uint8_t external_psram_size;


#define FILES_DISPLAY_MAX		9

typedef struct {
	ui_rect_t rect;
	uint64_t size;
	char name[42];
}file_log_t;

typedef struct {
	int total = 0;
	int renderFrom = 0;
	
	file_log_t files[FILES_DISPLAY_MAX];
	file_log_t selected;
}file_list_t;
static file_list_t filelist;

static ui_button_t button_config;
static ui_button_t button_overlayDetail;
static ui_button_t button_logCtrl;
static ui_button_t button_zoom[2];
static ui_button_t button_updown[2];
static ui_button_t button_logRefresh;
static ui_button_t button_logLoad;


#define UI_WIDGETOBJS_TOTAL		23

#if (TFT_LOWERPANEL)
static ui_widget_t *widgetObjs[UI_WIDGETOBJS_TOTAL] = {WIDGET(&button_config), 0, 0, 0, 0, 0, WIDGET(&button_overlayDetail), 0, WIDGET(&button_logCtrl), 0, WIDGET(&button_zoom[0]), WIDGET(&button_zoom[1]), 0, WIDGET(&button_updown[0]), WIDGET(&button_updown[1]), WIDGET(&button_logRefresh), WIDGET(&button_logLoad), 0, 0, 0, 0, 0, 0};
#else
static ui_widget_t *widgetObjs[UI_WIDGETOBJS_TOTAL] = {WIDGET(&button_config), 0, 0, 0, 0, 0, NULL, 0, NULL, 0, NULL, NULL, 0, NULL, NULL, NULL, NULL, 0, 0, 0, 0, 0, 0};
#endif

static uint32_t opFuncs[OP_QUEUE_LENGTH];	// FIFO
static uint32_t opPosition = 0;
static uint32_t opState = OP_IDLE;
static uint32_t opSetBaud = 0;
static uint8_t  opSetRate = 37;

static uint8_t ui_activePanelId = 0;
const static uint8_t ui_menuColumn = 30;




static inline ui_widget_t *ui_getWidget (const uint8_t id)
{
	for (int i = 0; i < UI_WIDGETOBJS_TOTAL; i++){
		ui_all_t *obj = (ui_all_t*)widgetObjs[i];
		if (!obj) continue;

		if (obj->widget.id == id){
			return (ui_widget_t*)obj;
			
		}else if (obj->widget.children.total){
			ui_all_t **buttonList = (ui_all_t**)obj->widget.children.widgets;
			
			for (int j = 0; j < obj->widget.children.total; j++){
				ui_all_t *child = (ui_all_t*)buttonList[j];
				
				if (!child) continue;
				if (child->widget.id == id)
					return (ui_widget_t*)child;
			}
		}
	}
		
	return NULL;
}

static inline void ui_disablePanels ()
{
	for (int i = 0; i < UI_WIDGETOBJS_TOTAL; i++){
		ui_all_t *obj = (ui_all_t*)widgetObjs[i];
		if (!obj) continue;

		if (obj->widget.type == UI_WIDGET_PANEL)
			obj->widget.isEnabled = 0;
	}
}

static inline int ui_setPosition (const uint8_t id, const int32_t x, const int32_t y)
{
	ui_all_t *obj = (ui_all_t*)ui_getWidget(id);
	if (obj){
		if (x > -1) obj->rect.x = x;
		if (y > -1) obj->rect.y = y;
		return 1;
	}
	return 0;
}

static inline int ui_getPosition (const uint8_t id, int32_t *x, int32_t *y)
{
	ui_all_t *obj = (ui_all_t*)ui_getWidget(id);
	if (obj){
		if (x) *x = obj->rect.x;
		if (y) *y = obj->rect.y;
		return 1;
	}
	return 0;
}

static void ui_draw_button (ui_all_t *widget, const int32_t x, const int32_t y)
{
	ui_button_t *button = (ui_button_t*)widget;
	if (!button->label.text) return;

	setBrush(inst.vfont, BRUSH_DISK);
	setGlyphScale(inst.vfont, button->label.scale/10.0f);
	setBrushSize(inst.vfont, button->label.size);
	setBrushQuality(inst.vfont, button->label.quality);

	if (button->widget.isNotReady){
		setBrushColour(inst.vfont, COLOUR_PAL_LIGHTGREY);
		
	}else if (button->widget.isHighlighted){
		setBrushSize(inst.vfont, button->label.size+6.0f);
		setBrushColour(inst.vfont, COLOUR_PAL_WHITE);
		drawString(inst.vfont, button->label.text, x+button->offset.x, y+button->offset.y);
			
		setBrushSize(inst.vfont, button->label.size);
		setBrushColour(inst.vfont, button->label.colour);
	}else{
		setBrushColour(inst.vfont, button->label.colour);
	}

	drawString(inst.vfont, button->label.text, x+button->offset.x, y+button->offset.y);
	if (!button->rect.width)
		button->rect.width = (inst.vfont->pos.x - (x+button->offset.x))+4;
	
#if (UI_DRAW_TOUCH_RECTS)
	drawRectangle(x, y - (button->offset.y/2), x+button->rect.width, (y+button->rect.height), COLOUR_PAL_BLACK);
#endif
}

static void ui_draw_panel (ui_all_t *widget, const int32_t x, const int32_t y)
{
	ui_panel_t *panel = (ui_panel_t*)widget;
	
	drawRectangleFilled(x+1, y+1, x+panel->rect.width-2, y+panel->rect.height-2, COLOUR_PAL_LIGHTERGREY);
	drawRectangle(x, y, x+panel->rect.width-1, y+panel->rect.height-1, COLOUR_PAL_ORANGE);


	const int total = panel->widget.children.total;
	if (total){
		const int triangleSize = 20;
		int tx = x + panel->rect.width - triangleSize - 6;
	
		for (int i = 0; i < total; i++){
			ui_all_t *obj = (ui_all_t*)panel->widget.children.widgets[i];
			if (!obj || !obj->widget.isEnabled) continue;
			
			if (obj->widget.type == UI_WIDGET_BUTTON && (obj->widget.flags&UI_WIDGET_FLAG_HASPANEL)){
				ui_button_t *button = (ui_button_t*)obj;
				int ty = (y + button->rect.y) + ((button->rect.height - triangleSize)/2);
				drawTriangleFilled(tx, ty, tx+triangleSize, ty+(triangleSize/2), tx, ty+triangleSize, COLOUR_PAL_ELITE);
				
			}else if (obj->widget.type == UI_WIDGET_BUTTON && (obj->widget.flags&UI_WIDGET_FLAG_HASPOPUP)){
				ui_button_t *button = (ui_button_t*)obj;
				tx += 8;
				int ty = (y + button->rect.y) + ((button->rect.height - triangleSize)/2) + 6;
				drawCircleFilled(tx, ty, triangleSize/2.0f, COLOUR_PAL_ELITE);
			}
		}
	}
}

static void ui_render (ui_widget_t **widgets, const uint8_t total, const int32_t x, const int32_t y, const uint32_t flags)
{
	for (int i = 0; i < total; i++){
		ui_widget_t *widget = widgets[i];
		if (!widget) continue;
		
		if (widget->isEnabled){
			ui_all_t *obj = (ui_all_t*)widget;
			int32_t childX = x + obj->rect.x;
			int32_t childY = y + obj->rect.y;			

			if (widget->type == UI_WIDGET_PANEL){
				obj->callback.func(widget, widget->id, flags, UI_WIDGET_MSG_RENDER, childX, childY);
				
			}else if (widget->type == UI_WIDGET_BUTTON){
				obj->callback.func(widget, widget->id, flags, UI_WIDGET_MSG_RENDER, childX, childY);
			}

			if (widget->children.total)
				ui_render(widget->children.widgets, widget->children.total, childX, childY, flags);
		}
	}
}

static int ui_input (ui_widget_t **widgets, const uint8_t total, const int32_t x, const int32_t y, uint32_t *flags)
{
	if (*flags) return 1;	// handler found
	int ret = 0;
	
	for (int i = 0; i < total; i++){
		ui_widget_t *widget = widgets[i];
		if (!widget) continue;
		
		if (widget->isEnabled){
			int32_t childX = x;
			int32_t childY = y;
			int32_t localX = 0;
			int32_t localY = 0;
			
			if (widget->type == UI_WIDGET_PANEL){
				ui_all_t *obj = (ui_all_t*)widget;
				localX = obj->rect.x;
				localY = obj->rect.y;
				
				if (childX >= obj->rect.x && childY >= obj->rect.y){
					if (childX < obj->rect.x+obj->rect.width && childY < obj->rect.y+obj->rect.height){
						ret++;
						ret += obj->callback.func(widget, widget->id, *flags, UI_WIDGET_MSG_INPUT, x, y);
					}
				}
			}else if (widget->type == UI_WIDGET_BUTTON){
				ui_all_t *obj = (ui_all_t*)widget;
				localX = obj->rect.x;
				localY = obj->rect.y;

				if (!obj->widget.isNotReady){
					const int32_t offsetY = (((ui_button_t *)obj)->offset.y/2);
					if (childX >= obj->rect.x && childY >= (obj->rect.y - offsetY)){
						if (childX < obj->rect.x+obj->rect.width && childY < (obj->rect.y+obj->rect.height)-offsetY){
							if (obj->widget.flags&UI_WIDGET_FLAG_HASPANEL || obj->widget.flags&UI_WIDGET_FLAG_HASPOPUP){
								ui_disable(NULL, ui_activePanelId);
								ui_button_t *button = (ui_button_t*)obj;
								ui_activePanelId = button->child.id;
								ui_enable(NULL, ui_activePanelId);
								*flags = ui_activePanelId;

							}else if (obj->callback.func(widget, widget->id, *flags, UI_WIDGET_MSG_INPUT, x, y)){
								*flags = widget->id;
							}
							return ++ret;
						}
					}
				}
			}

			if (widget->children.total){
				ui_widget_t *objs[widget->children.total];
				for (int j = 0; j < widget->children.total; j++)
					objs[j] = CHILD_WIDGET_OBJ(widget,j);

				ret += ui_input(objs, widget->children.total, childX-localX, childY-localY, flags);
				if (*flags) return ret;
			}
		}
	}
	return ret;
}

int ui_isEnabled (void *opaque, const uint8_t child_id)
{
	ui_widget_t *obj = WIDGET(opaque);

	if (obj && !child_id){
		return obj->isEnabled;
		
	}else if (!obj && child_id){
		for (int i = 0; i < UI_WIDGETOBJS_TOTAL; i++){
			obj = widgetObjs[i];
			if (!obj) continue;
			
			if (obj->id == child_id){
				return obj->isEnabled;
			}else{
				for (int j = 0; j < obj->children.total; j++){
					ui_widget_t *widget = CHILD_WIDGET_OBJ(obj,j);
					if (widget->id == child_id)
						return widget->isEnabled;
				}
			}
		}
	}else{
		for (int i = 0; i < obj->children.total; i++){
			ui_widget_t *widget = CHILD_WIDGET_OBJ(obj,i);
			
			if (widget->id == child_id)
				return widget->isEnabled;
		}
	}
	return UI_WIDGET_DISABLED;
}

int ui_enableReady (void *opaque, const uint8_t child_id)
{
	ui_widget_t *obj = WIDGET(opaque);

	if (obj && !child_id){
		obj->isEnabled = UI_WIDGET_ENABLED;
		obj->isNotReady = 0;
		return 1;
		
	}else if (!obj && child_id){
		for (int i = 0; i < UI_WIDGETOBJS_TOTAL; i++){
			ui_all_t *obj = (ui_all_t*)widgetObjs[i];
			if (!obj) continue;
			
			if (obj->widget.id == child_id){
				obj->widget.isEnabled = UI_WIDGET_ENABLED;
				obj->widget.isNotReady = 0;
				return 1;
			}else{
				for (int j = 0; j < obj->widget.children.total; j++){
					ui_widget_t *widget = obj->widget.children.widgets[j]; //CHILD_WIDGET_OBJ(obj,j);
					
					if (widget->id == child_id){
						widget->isEnabled = UI_WIDGET_ENABLED;
						widget->isNotReady = 0;
						return 1;
					}
				}
			}
		}
	}else{
		for (int i = 0; i < obj->children.total; i++){
			ui_widget_t *widget = CHILD_WIDGET_OBJ(obj,i);
			
			if (widget->id == child_id){
				widget->isEnabled = UI_WIDGET_ENABLED;
				widget->isNotReady = 0;
				return 1;
			}
		}
	}
	return 0;
}

int ui_enableNotReady (void *opaque, const uint8_t child_id)
{
	ui_widget_t *obj = WIDGET(opaque);

	if (obj && !child_id){
		obj->isEnabled = UI_WIDGET_ENABLED;
		obj->isNotReady = 1;
		return 1;
		
	}else if (!obj && child_id){
		for (int i = 0; i < UI_WIDGETOBJS_TOTAL; i++){
			ui_all_t *obj = (ui_all_t*)widgetObjs[i];
			if (!obj) continue;
			
			if (obj->widget.id == child_id){
				obj->widget.isEnabled = UI_WIDGET_ENABLED;
				obj->widget.isNotReady = 1;
				return 1;
			}else{
				for (int j = 0; j < obj->widget.children.total; j++){
					ui_widget_t *widget = obj->widget.children.widgets[j]; //CHILD_WIDGET_OBJ(obj,j);
					
					if (widget->id == child_id){
						widget->isEnabled = UI_WIDGET_ENABLED;
						widget->isNotReady = 1;
						return 1;
					}
				}
			}
		}
	}else{
		for (int i = 0; i < obj->children.total; i++){
			ui_widget_t *widget = CHILD_WIDGET_OBJ(obj,i);
			
			if (widget->id == child_id){
				widget->isEnabled = UI_WIDGET_ENABLED;
				widget->isNotReady = 1;
				return 1;
			}
		}
	}
	return 0;
}

int ui_setHighlight (const uint8_t child_id, const uint8_t state)
{
	if (child_id){
		for (int i = 0; i < UI_WIDGETOBJS_TOTAL; i++){
			ui_all_t *obj = (ui_all_t*)widgetObjs[i];
			if (!obj) continue;
			
			if (obj->widget.id == child_id){
				obj->widget.isHighlighted = state&0x01;
				return 1;
			}else{
				for (int j = 0; j < obj->widget.children.total; j++){
					ui_widget_t *widget = obj->widget.children.widgets[j];
					
					if (widget->id == child_id){
						widget->isHighlighted = state&0x01;
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

int ui_getHighlight (const uint8_t child_id)
{
	if (child_id){
		for (int i = 0; i < UI_WIDGETOBJS_TOTAL; i++){
			ui_all_t *obj = (ui_all_t*)widgetObjs[i];
			if (!obj) continue;
			
			if (obj->widget.id == child_id){
				return obj->widget.isHighlighted;
			}else{
				for (int j = 0; j < obj->widget.children.total; j++){
					ui_widget_t *widget = obj->widget.children.widgets[j];
					
					if (widget->id == child_id)
						return widget->isHighlighted;
				}
			}
		}
	}
	return 0;
}

int ui_enable (void *opaque, const uint8_t child_id)
{
	ui_widget_t *obj = WIDGET(opaque);

	if (obj && !child_id){
		obj->isEnabled = UI_WIDGET_ENABLED;
		return 1;
		
	}else if (!obj && child_id){
		for (int i = 0; i < UI_WIDGETOBJS_TOTAL; i++){
			ui_all_t *obj = (ui_all_t*)widgetObjs[i];
			if (!obj) continue;
			
			if (obj->widget.id == child_id){
				obj->widget.isEnabled = UI_WIDGET_ENABLED;
				return 1;
			}else{
				for (int j = 0; j < obj->widget.children.total; j++){
					ui_widget_t *widget = obj->widget.children.widgets[j]; //CHILD_WIDGET_OBJ(obj,j);
					
					if (widget->id == child_id){
						widget->isEnabled = UI_WIDGET_ENABLED;
						return 1;
					}
				}
			}
		}
	}else{
		for (int i = 0; i < obj->children.total; i++){
			ui_widget_t *widget = CHILD_WIDGET_OBJ(obj,i);
			
			if (widget->id == child_id){
				widget->isEnabled = UI_WIDGET_ENABLED;
				return 1;
			}
		}
	}
	return 0;
}

int ui_disable (void *opaque, const uint8_t child_id)
{
	ui_widget_t *obj = WIDGET(opaque);

	if (obj && !child_id){
		obj->isEnabled = UI_WIDGET_DISABLED;
		return 1;
		
	}else if (!obj && child_id){
		for (int i = 0; i < UI_WIDGETOBJS_TOTAL; i++){
			ui_all_t *obj = (ui_all_t*)widgetObjs[i];
			if (!obj) continue;
			
			if (obj->widget.id == child_id){
				obj->widget.isEnabled = UI_WIDGET_DISABLED;
				return 1;
			}else{
				for (int j = 0; j < obj->widget.children.total; j++){
					ui_widget_t *widget = obj->widget.children.widgets[j]; //CHILD_WIDGET_OBJ(obj,j);
					
					if (widget->id == child_id){
						widget->isEnabled = UI_WIDGET_DISABLED;
						return 1;
					}
				}
			}
		}
	}else{
		for (int i = 0; i < obj->children.total; i++){
			ui_widget_t *widget = CHILD_WIDGET_OBJ(obj,i);
			
			if (widget->id == child_id){
				widget->isEnabled = UI_WIDGET_DISABLED;
				return 1;
			}
		}
	}
	return 0;
}

// FIFO
uint32_t op_push (const uint32_t op_code)
{
	if (opPosition < OP_QUEUE_LENGTH-1){
		opFuncs[opPosition++] = op_code;
		return opPosition;
	}
	return 0;
}

uint32_t op_pop ()
{
	if (opPosition){
		opPosition--;
		uint32_t opFunc = opFuncs[0];
		if (opPosition){
			for (int i = 0; i < OP_QUEUE_LENGTH-1; i++)
				opFuncs[i] = opFuncs[i+1];
		}
		return opFunc;
	}
	return 0;
}

void op_go ()
{
	opState = OP_READY;
}

static inline void op_halt ()
{
	opState = OP_IDLE;
}

uint32_t op_state ()
{
	return opState;
}

static void uiLogs_open ()
{
	ui_disable(NULL, UI_ID_PANEL_MENU);
	ui_disable(NULL, UI_ID_BUTTON_OVERLAYDETAIL);
	ui_disable(NULL, UI_ID_BUTTON_LOGCTRL);
	ui_disable(NULL, UI_ID_BUTTON_ZOOM_IN);
	ui_disable(NULL, UI_ID_BUTTON_ZOOM_OUT);
		
	ui_enable(NULL, UI_ID_PANEL_LOGS);
	ui_enable(NULL, UI_ID_BUTTON_REFRESH);
	ui_enable(NULL, UI_ID_BUTTON_UP);
	ui_enable(NULL, UI_ID_BUTTON_DOWN);
	ui_disable(NULL, UI_ID_BUTTON_LOAD);
	
	memset(&filelist.selected, 0, sizeof(filelist.selected));
	drawPanel(2);
}

static void uiLogs_close ()
{
	ui_disable(NULL, UI_ID_PANEL_LOGS);
		
	ui_enable(NULL, UI_ID_BUTTON_CONFIG);
	ui_enable(NULL, UI_ID_BUTTON_OVERLAYDETAIL);
	ui_enable(NULL, UI_ID_BUTTON_LOGCTRL);
	ui_enable(NULL, UI_ID_BUTTON_ZOOM_IN);
	ui_enable(NULL, UI_ID_BUTTON_ZOOM_OUT);
		
	ui_disable(NULL, UI_ID_BUTTON_REFRESH);
	ui_disable(NULL, UI_ID_BUTTON_UP);
	ui_disable(NULL, UI_ID_BUTTON_DOWN);
	ui_disable(NULL, UI_ID_BUTTON_LOAD);
	
	memset(&filelist.selected, 0, sizeof(filelist.selected));
	drawPanel(1);
}

static void load_import ()
{
	if (!filelist.selected.name[0])
		return;

	fio_setDir(TRACKPTS_DIR);

	char name[64];
	snprintf(name, sizeof(name), "%s.tpts", filelist.selected.name);
	int ct = log_load(name);
	if (ct){
		ui_enableReady(NULL, UI_ID_BUTTON_RESET);
		uiLogs_close();
		//printf(CS("%i trackPoints imported from %s"), ct, name);
	}

	fio_setDir("/");
}

int32_t op_execute (const uint32_t opCode)
{
	switch (opCode){
	case 0:
		op_halt();
		return 0;

	case OP_FUNC_RECEIVER_BAUD:
		if (opSetBaud == UI_ID_BUTTON_BAUD_9600) opSetBaud = 9600;
		else if (opSetBaud == UI_ID_BUTTON_BAUD_19200) opSetBaud = 19200;
		else if (opSetBaud == UI_ID_BUTTON_BAUD_38400) opSetBaud = 38400;
		else if (opSetBaud == UI_ID_BUTTON_BAUD_57600) opSetBaud = 57600;
		else if (opSetBaud == UI_ID_BUTTON_BAUD_115200) opSetBaud = 115200;
		else if (opSetBaud == UI_ID_BUTTON_BAUD_230400) opSetBaud = 230400;
		else if (opSetBaud == UI_ID_BUTTON_BAUD_460800) opSetBaud = 460800;
		else if (opSetBaud == UI_ID_BUTTON_BAUD_921600) opSetBaud = 921600;
		if (opSetBaud > 1000)
			gps_setBaud(opSetBaud);
		return 1;
	
	case OP_FUNC_RECEIVER_RATE:
		gps_setRate(opSetRate);
		return 1;

	case OP_FUNC_LOG_UP:
		if (filelist.renderFrom > 0) filelist.renderFrom--;
		filelist.total = 0;		
		return 1;
		
	case OP_FUNC_LOG_DOWN:
		if (filelist.renderFrom < (filelist.total-FILES_DISPLAY_MAX)+1){
			filelist.renderFrom++;
			filelist.total = 0;
		}		
		return 1;

	case OP_FUNC_LOG_START:
		log_start();
		return 1;
		
	case OP_FUNC_LOG_STOP:
		log_stop();
		return 1;
		
	case OP_FUNC_LOG_OPEN:
		uiLogs_open();
		return 1;

	case OP_FUNC_LOG_CLOSE:
		uiLogs_close();
		return 1;

	case OP_FUNC_LOG_LOAD:
		load_import();
		return 1;
		
	case OP_FUNC_LOG_WRITE:
		log_write();
		return 0;
		
	case OP_FUNC_GPS_TASK:
		gps_task();
		return 0;
		
	case OP_FUNC_ZOOMIN:
		render_zoomIn();
		return 1;
		
	case OP_FUNC_ZOOMOUT:
		render_zoomOut();
		return 1;

	case OP_FUNC_ZOOMRESET:
		render_zoomReset();
		return 1;
		
	case OP_FUNC_LOG_PAUSE:
		if (log_isActive())
			log_pause();
		return 1;
		
	case OP_FUNC_LOG_RESET:
		log_reset();
		log_stop();
		return 1;
		
	case OP_FUNC_MODE:
		if (ui_isEnabled(NULL, UI_ID_PANEL_DISPLAY))
			ui_disable(0, UI_ID_PANEL_DISPLAY);
		else
			ui_enable(0, UI_ID_PANEL_DISPLAY);
		return 1;

	case OP_FUNC_OFF:
		tft_setBacklight(0);
		return 0;

	case OP_FUNC_HOTSTART:
		gps_hotStart();
		return 1;
		
	case OP_FUNC_WARMSTART:
		gps_warmStart();
		return 1;
		
	case OP_FUNC_COLDSTART:
		gps_coldStart();
		return 1;

	case OP_FUNC_FREQ136:
		mpu_setClockFreq(136);
		return 0;
		
	case OP_FUNC_FREQ272:
		mpu_setClockFreq(272);
		return 0;
		
	case OP_FUNC_FREQ528:
		mpu_setClockFreq(528);
		return 0;
		
	case OP_FUNC_FREQ600:
		mpu_setClockFreq(600);
		return 0;
		
	case OP_FUNC_FREQ720:
		mpu_setClockFreq(720);
		return 0;
		
	case OP_FUNC_FREQ816:
		mpu_setClockFreq(816);
		return 0;
		
	case OP_FUNC_REINIT:
		gps_reinit();
		return 1;

	case OP_FUNC_RECONNECT:
		gps_reconnect();
		return 1;

	case OP_FUNC_VERSION:
		gps_printVersions();
		return 1;

	case OP_FUNC_STATUS:
		gps_printStatus();
		return 1;

	case OP_FUNC_REBOOT:
		mpu_reboot();
		return 0;
		
	case OP_FUNC_CONFIG:
		return 0;
	};
	
	return 0;
}

int uiButtons_cb (ui_widget_t *widget, const uint8_t id, const uint32_t flags, const uint32_t msg, const int32_t var1, const int32_t var2)
{
	if (msg == UI_WIDGET_MSG_RENDER){
		ui_draw_button((ui_all_t*)widget, var1, var2);
		return 1;

	}else if (msg != UI_WIDGET_MSG_INPUT){
		return 0;
	}
	
	switch (id){
	case UI_ID_BUTTON_AREAFILL:
		map_setDetail(MAP_RENDER_AREAS, !map_getDetail(MAP_RENDER_AREAS));
		ui_setHighlight(id, !ui_getHighlight(id));
		return 1;
		
	case UI_ID_BUTTON_AREAOUTLINR:
		map_setDetail(MAP_RENDER_AREAS_OUTLINE, !map_getDetail(MAP_RENDER_AREAS_OUTLINE));
		ui_setHighlight(id, !ui_getHighlight(id));
		return 1;
		
	case UI_ID_BUTTON_PATHFILL:
		map_setDetail(MAP_RENDER_PATHS, !map_getDetail(MAP_RENDER_PATHS));
		ui_setHighlight(id, !ui_getHighlight(id));
		return 1;
		
	case UI_ID_BUTTON_FATHLINE:
		map_setDetail(MAP_RENDER_PATHS_LINE, !map_getDetail(MAP_RENDER_PATHS_LINE));
		ui_setHighlight(id, !ui_getHighlight(id));
		return 1;
		
	case UI_ID_BUTTON_TITLEBOUND:
		map_setDetail(MAP_RENDER_TITLE_BOUNDARY, !map_getDetail(MAP_RENDER_TITLE_BOUNDARY));
		ui_setHighlight(id, !ui_getHighlight(id));
		return 1;
		
	case UI_ID_BUTTON_LOAD:
		op_push(OP_FUNC_LOG_LOAD);
		op_go();
		return 1;

	case UI_ID_BUTTON_REFRESH:
		uiLogs_clear();
		return 1;
		
	case UI_ID_BUTTON_UP:
		op_push(OP_FUNC_LOG_UP);
		op_go();
		return 1;

	case UI_ID_BUTTON_DOWN:
		op_push(OP_FUNC_LOG_DOWN);
		op_go();
		return 1;

	case UI_ID_BUTTON_START:
		ui_disable(NULL, UI_ID_BUTTON_START);
		ui_enable(NULL, UI_ID_BUTTON_PAUSE);
		ui_enableReady(NULL, UI_ID_BUTTON_STOP);
		ui_enableReady(NULL, UI_ID_BUTTON_RESET);
		op_push(OP_FUNC_LOG_START);
		op_go();
		return 1;
		
	case UI_ID_BUTTON_STOP:
		ui_enable(NULL, UI_ID_BUTTON_START);
		ui_disable(NULL, UI_ID_BUTTON_PAUSE);
		ui_enableNotReady(NULL, UI_ID_BUTTON_STOP);
		ui_enableReady(NULL, UI_ID_BUTTON_RESET);
		op_push(OP_FUNC_LOG_WRITE);
		op_push(OP_FUNC_LOG_STOP);
		op_push(OP_FUNC_GPS_TASK);
		op_go();
		return 1;
		
	case UI_ID_BUTTON_PAUSE: 
		ui_enable(NULL, UI_ID_BUTTON_START);
		ui_disable(NULL, UI_ID_BUTTON_PAUSE);
		ui_enableReady(NULL, UI_ID_BUTTON_STOP);
		ui_enableReady(NULL, UI_ID_BUTTON_RESET);
		op_push(OP_FUNC_LOG_PAUSE);
		op_go();
		return 1;

	case UI_ID_BUTTON_RESET:
		ui_enable(NULL, UI_ID_BUTTON_START);
		ui_disable(NULL, UI_ID_BUTTON_PAUSE);
		ui_enableNotReady(NULL, UI_ID_BUTTON_STOP);
		ui_enableNotReady(NULL, UI_ID_BUTTON_RESET);

		op_push(OP_FUNC_LOG_RESET);
		op_go();
		return 1;

	case UI_ID_BUTTON_ZOOM_OUT:
		op_push(OP_FUNC_ZOOMOUT);
		op_go();
		return 1;
		
	case UI_ID_BUTTON_ZOOM_IN:
		op_push(OP_FUNC_ZOOMIN);
		op_go();
		return 1;
		
	case UI_ID_BUTTON_OVERLAYDETAIL:
		op_push(OP_FUNC_MODE);
		op_go();
		return 1;

	case UI_ID_BUTTON_DRAW_TEXT:
		map_setDetail(MAP_RENDER_STRINGS, !ui_getHighlight(id));
		ui_setHighlight(id, !ui_getHighlight(id));
		return 1;
		
	case UI_ID_BUTTON_DRAW_SATW:
		map_setDetail(MAP_RENDER_SWORLD, !ui_getHighlight(id));
		ui_setHighlight(id, !ui_getHighlight(id));
		return 1;
		
	case UI_ID_BUTTON_DRAW_AVAIL:
		map_setDetail(MAP_RENDER_SAVAIL, !ui_getHighlight(id));
		ui_setHighlight(id, !ui_getHighlight(id));
		return 1;
		
	case UI_ID_BUTTON_DRAW_LVLS:
		map_setDetail(MAP_RENDER_SLEVELS, !ui_getHighlight(id));
		ui_setHighlight(id, !ui_getHighlight(id));
		return 1;
		
	case UI_ID_BUTTON_DRAW_COMP:
		map_setDetail(MAP_RENDER_COMPASS, !ui_getHighlight(id));
		ui_setHighlight(id, !ui_getHighlight(id));
		return 1;
		
	case UI_ID_BUTTON_DRAW_RULER:
		map_setDetail(MAP_RENDER_MEASURE, !ui_getHighlight(id));
		ui_setHighlight(id, !ui_getHighlight(id));
		return 1;

	case UI_ID_BUTTON_DRAW_LOC:
		map_setDetail(MAP_RENDER_LOCGRAPTHIC, !ui_getHighlight(id));
		ui_setHighlight(id, !ui_getHighlight(id));
		return 1;
		
	case UI_ID_BUTTON_DRAW_ROUTE:
		map_setDetail(MAP_RENDER_TRACKPOINTS, !ui_getHighlight(id));
		ui_setHighlight(id, !ui_getHighlight(id));
		return 1;
		
	case UI_ID_BUTTON_OFF:
		op_push(OP_FUNC_OFF);
		op_go();
		return 1;
	
	case UI_ID_BUTTON_LOGS:
		op_push(OP_FUNC_LOG_OPEN);
		op_go();
		return 1;

	case UI_ID_BUTTON_STYLE1:
	case UI_ID_BUTTON_STYLE2:
	case UI_ID_BUTTON_STYLE3:
		sceneSetColourScheme(id - UI_ID_BUTTON_STYLE1);
		return 1;

	case UI_ID_BUTTON_RATE1:
	case UI_ID_BUTTON_RATE2:
	case UI_ID_BUTTON_RATE3:
	case UI_ID_BUTTON_RATE4:
	case UI_ID_BUTTON_RATE5:
	case UI_ID_BUTTON_RATE6:
	case UI_ID_BUTTON_RATE7:
		ui_setHighlight(UI_ID_BUTTON_RATE1+(opSetRate-35), 0);
		ui_setHighlight(id, 1);
		opSetRate = (id - UI_ID_BUTTON_RATE1) + 35;
		op_push(OP_FUNC_RECEIVER_RATE);
		op_go();
		return 1;

	case UI_ID_BUTTON_BAUD_9600:
	case UI_ID_BUTTON_BAUD_19200:
	case UI_ID_BUTTON_BAUD_38400:
	case UI_ID_BUTTON_BAUD_57600:
	case UI_ID_BUTTON_BAUD_115200:
	case UI_ID_BUTTON_BAUD_230400:
	case UI_ID_BUTTON_BAUD_460800:
	case UI_ID_BUTTON_BAUD_921600:
		opSetBaud = id;
		op_push(OP_FUNC_RECEIVER_BAUD);
		op_go();
		return 1;

	case UI_ID_BUTTON_HOTSTART:
		op_push(OP_FUNC_HOTSTART);
		op_go();
		return 1;

	case UI_ID_BUTTON_MPU_FREQ136:
		op_push(OP_FUNC_FREQ136);
		op_go();
		return 0;
		
	case UI_ID_BUTTON_MPU_FREQ272:
		op_push(OP_FUNC_FREQ272);
		op_go();
		return 0;

	case UI_ID_BUTTON_MPU_FREQ528:
		op_push(OP_FUNC_FREQ528);
		op_go();
		return 0;

	case UI_ID_BUTTON_MPU_FREQ600:
		op_push(OP_FUNC_FREQ600);
		op_go();
		return 0;

	case UI_ID_BUTTON_MPU_FREQ720:
		op_push(OP_FUNC_FREQ720);
		op_go();
		return 0;
		
	case UI_ID_BUTTON_MPU_FREQ816:
		op_push(OP_FUNC_FREQ816);
		op_go();
		return 0;

	case UI_ID_BUTTON_WARMSTART:
		op_push(OP_FUNC_WARMSTART);
		op_go();
		return 1;
		
	case UI_ID_BUTTON_COLDSTART:
		op_push(OP_FUNC_COLDSTART);
		op_go();		
		return 1;

	case UI_ID_BUTTON_REINIT:
		render_signalUpdate();
		op_push(OP_FUNC_REINIT);
		op_go();
		return 1;

	case UI_ID_BUTTON_VERSION:
	 	ui_disable(NULL, UI_ID_PANEL_RECEIVER);
	 	ui_activePanelId = 0;
		op_push(OP_FUNC_VERSION);
		op_go();
		return 1;

	case UI_ID_BUTTON_STATUS:
		ui_disable(NULL, UI_ID_PANEL_RECEIVER);
		op_push(OP_FUNC_STATUS);
		op_go();
		return 1;
		
	case UI_ID_BUTTON_RECONNECT:
		render_signalUpdate();
		op_push(OP_FUNC_RECONNECT);
		op_go();
		return 1;

	case UI_ID_BUTTON_REBOOT:
		op_push(OP_FUNC_REBOOT);
		op_go();
		return 1;

#if (TFT_LOWERPANEL)	
	case UI_ID_BUTTON_LOGCTRL:
		if (ui_activePanelId && (ui_activePanelId != UI_ID_PANEL_LOGCTRL)){
			ui_disable(NULL, ui_activePanelId);
			ui_activePanelId = 0;
		}
		
		if (ui_isEnabled(NULL, UI_ID_PANEL_LOGCTRL)){
			ui_disable(NULL, UI_ID_PANEL_LOGCTRL);
			ui_activePanelId = 0;
		}else{
			ui_activePanelId = UI_ID_PANEL_LOGCTRL;
			ui_enable(NULL, ui_activePanelId);
		}

		return 1;
#endif	
	case UI_ID_BUTTON_CONFIG:	// toggle mapCtrl panel
		ui_disablePanels();
		ui_enable(NULL, UI_ID_PANEL_MENU);
		ui_activePanelId = UI_ID_PANEL_MENU;
		ui_disable(NULL, UI_ID_BUTTON_CONFIG);
		drawPanel(1);
		return 1;
	}
	return 0;
}

static void ui_draw_about (ui_all_t *widget, const int32_t x, const int32_t y)
{
	setBrush(inst.vfont, BRUSH_DISK);
	setGlyphScale(inst.vfont, 1.0f);
	setBrushSize(inst.vfont, 3.0f);
	setBrushQuality(inst.vfont, 2);
	setBrushColour(inst.vfont, COLOUR_PAL_DARKGREY);
	
	char buffer[128];
	int btX = x + 10;
	int btY = y + 24;
	const int pitchY = 34;


	drawString(inst.vfont, CFG_STRING, btX, btY);
	
	sprintf(buffer, "GNSS Receiver:");
	if (RECEIVER_SINGLE == 1)
		strcat(buffer, " Single");
	else
		strcat(buffer, " Dual");

	if (RECEIVER_M10 == 1)
		strcat(buffer, " UBlox M10");
	else
		strcat(buffer, " UBlox M8");
	drawString(inst.vfont, buffer, btX, btY += pitchY);

	sprintf(buffer, "SDCard size: %iGB", SDCARD_SIZE);
	drawString(inst.vfont, buffer, btX, btY += pitchY);
	
	sprintf(buffer, "Clock frequency: %uMhz", (unsigned int)F_CPU_ACTUAL/1000/1000);
	drawString(inst.vfont, buffer, btX, btY += pitchY);

	sprintf(buffer, "ExtMem: %uMB", external_psram_size);
	drawString(inst.vfont, buffer, btX, btY += pitchY);
	
	sprintf(buffer, "CPU temp: %.2fc", InternalTemperature.readTemperatureC());
	drawString(inst.vfont, buffer, btX, btY += pitchY);
	
	sprintf(buffer, "Tiles: %i", tilesCount());
	drawString(inst.vfont, buffer, btX, btY += pitchY);
	
	sprintf(buffer, "Blocks: %i", blocksCount());
	drawString(inst.vfont, buffer, btX, btY += pitchY);
	
	sprintf(buffer, "Tile memory used: %u", tileMemoryUsage());
	drawString(inst.vfont, buffer, btX, btY += pitchY);

	btY += pitchY;

	sprintf(buffer, "Receiver Baud: %i", (int)gps_getBaud());
	drawString(inst.vfont, buffer, btX, btY += pitchY);
	
	sprintf(buffer, "Rx: %i   Tx: %i", receiver_getRx(), receiver_getTx());
	drawString(inst.vfont, buffer, btX, btY += pitchY);
	receiver_resetRxTx();
}

int uiPanel_cb (ui_widget_t *widget, const uint8_t id, const uint32_t flags, const uint32_t msg, const int32_t var1, const int32_t var2)
{
	if (msg == UI_WIDGET_MSG_RENDER){
		ui_draw_panel((ui_all_t*)widget, var1, var2);
		if (id == UI_ID_PANEL_MPU_ABOUT)
			ui_draw_about((ui_all_t*)widget, var1, var2);	

		return 1;
	}
	return 0;
}

static int uiLogs_input (ui_widget_t *widget, const uint8_t id, const uint32_t flags, const int32_t x, const int32_t y)
{
	for (int i = 0; i < FILES_DISPLAY_MAX; i++){
		file_log_t *file = &filelist.files[i];
		
		if (y > file->rect.y-(file->rect.height/2) && y < file->rect.y+(file->rect.height/2)){
			if (x > file->rect.x && x < file->rect.x+file->rect.width){
				if (!strcmp(filelist.selected.name, file->name)){
					memset(&filelist.selected, 0, sizeof(filelist.selected));
					ui_disable(NULL, UI_ID_BUTTON_LOAD);
					drawPanel(2);
				}else{
					filelist.selected = *file;
					ui_enable(NULL, UI_ID_BUTTON_LOAD);
					drawPanel(3);
				}
				break;
			}
		}
	}
	return 1;
}

static File file_open (const char *file)
{
	fio_setDir(TRACKPTS_DIR);
	return SD.open(file);
}

static inline void formatSize (char *buffer, const uint64_t filesize)
{
	const int len = 32;
	if (filesize >= (uint64_t)100*1024*1024*1024)
		snprintf(buffer, len, "%i GB", (int)(filesize/1024/1024/1024));
	else if (filesize >= (uint64_t)1024*1024*1024)
		snprintf(buffer, len, "%.1f GB", filesize/1024.0/1024.0/1024.0);
	else if (filesize >= 100*1024*1024)
		snprintf(buffer, len, "%i MB", (int)(filesize/1024/1024));
	else if (filesize >= 1024*1024)
		snprintf(buffer, len, "%.1f MB", filesize/1024.0/1024.0);
	else if (filesize >= 10*1024)
		snprintf(buffer, len, "%i KB", (int)(filesize/1024));
	else if (filesize >= 1024)
		snprintf(buffer, len, "%.1f KB", filesize/1024.0);
	else
		snprintf(buffer, len, "%i", (int)(filesize));
}

static void uiLogs_draw (ui_all_t *widget, const uint8_t id, const int32_t x, const int32_t y)
{
	char buffer[34];
	int offsetX = x + 10;
	//int offsetY = y + 28;
	
	setBrush(inst.vfont, BRUSH_DISK);
	setGlyphScale(inst.vfont, 1.0f);
	setBrushSize(inst.vfont, 4.0f);
	setBrushQuality(inst.vfont, 2);
	setBrushColour(inst.vfont, COLOUR_PAL_ELITE);
	int dontCheckTwice = 0;
	
	for (int i = 0; i < FILES_DISPLAY_MAX; i++){
		file_log_t *file = &filelist.files[i];
		if (!file->name[0]) break;
	
		setBrushColour(inst.vfont, COLOUR_PAL_DARKGREY);
		formatSize(buffer, file->size);
		drawString(inst.vfont, buffer, offsetX, file->rect.y);

		if (!dontCheckTwice && !strcmp(filelist.selected.name, file->name)){
			dontCheckTwice = 1;
			setBrushSize(inst.vfont, 12.0f);
			setBrushColour(inst.vfont, COLOUR_PAL_WHITE);
			drawString(inst.vfont, file->name, file->rect.x, file->rect.y);

			setBrushSize(inst.vfont, 4.0f);
		}

		setBrushColour(inst.vfont, COLOUR_PAL_ELITE);
		drawString(inst.vfont, file->name, file->rect.x, file->rect.y);

		file->rect.width = inst.vfont->pos.x - file->rect.x;
#if (UI_DRAW_TOUCH_RECTS)
		drawRectangle(file->rect.x, file->rect.y - (file->rect.height/2), file->rect.x+file->rect.width, file->rect.y+(file->rect.height/2), COLOUR_PAL_BLACK);
#endif
	}
}

void uiLogs_clear ()
{
	filelist.total = 0;

	for (int i = 0; i < FILES_DISPLAY_MAX; i++)
		filelist.files[i].name[0] = 0;
}

static int uiLogs_collect (ui_all_t *widget, const uint8_t id, const int32_t x, const int32_t y, const int startAt, const int endAt)
{
	int offsetX = x + 10;
	int offsetY = y + 24;
	int fileIdx = 0;
	int fileCt = 0;


	File root = file_open(TRACKPTS_DIR);
	while (true){
    	File entry = root.openNextFile();
    	if (!entry) break; // no more files
    	if (fileIdx < startAt){
    		fileIdx++;
    		continue;
    	}

    	if (!entry.isDirectory()){
    		const char *name = entry.name();
    		if (name){
    			if (fileCt < FILES_DISPLAY_MAX){
   					filelist.files[fileCt].size = entry.size();

					snprintf(filelist.files[fileCt].name, sizeof(filelist.files[fileCt].name)-1, "%s", name);
					char *found = strrchr(filelist.files[fileCt].name, '.');
					if (found) *found = 0;

					filelist.files[fileCt].rect.x = offsetX + 124;
					filelist.files[fileCt].rect.y = offsetY;
					filelist.files[fileCt].rect.width = 700;
					filelist.files[fileCt].rect.height = 45;
   					fileCt++;
				}
				offsetY += 50;
    		}
    		fileIdx++;
    	}
    	entry.close();
  	}
	fio_setDir("/");
	filelist.total = fileIdx;
	return filelist.total;
}

int uiLogs_cb (ui_widget_t *widget, const uint8_t id, const uint32_t flags, const uint32_t msg, const int32_t var1, const int32_t var2)
{
	if (msg == UI_WIDGET_MSG_RENDER){
		if (widget->type == UI_WIDGET_BUTTON){
			ui_draw_button((ui_all_t*)widget, var1, var2);
			
		}else if (widget->type == UI_WIDGET_PANEL){
			ui_draw_panel((ui_all_t*)widget, var1, var2);
			if (!filelist.total){
				uiLogs_clear();
				uiLogs_collect((ui_all_t*)widget, id, var1, var2, filelist.renderFrom, filelist.renderFrom+8);
			}
			uiLogs_draw((ui_all_t*)widget, id, var1, var2);
		}
		return 1;
		
	}else  if (msg == UI_WIDGET_MSG_INPUT){
		return uiLogs_input(widget, id, flags, var1, var2);
	}
	return 0;
}

FLASHMEM static ui_panel_t *ui_panel_create (const uint8_t ui_id, const uint8_t tButtons, ui_widget_cb_t callback, const uint16_t x, const uint16_t y, const uint16_t width, const uint16_t height)
{
	ui_widget_t **buttonList = (ui_widget_t**)calloc(tButtons, sizeof(ui_button_t*));
	if (!buttonList) return NULL;
	ui_button_t *buttons = (ui_button_t*)calloc(tButtons, sizeof(ui_button_t));
	if (!buttons) return NULL;


	for (int i = 0; i < tButtons; i++)
		buttonList[i] = (ui_widget_t*)&buttons[i];

	ui_panel_t *panel = (ui_panel_t*)calloc(1, sizeof(ui_panel_t));
	if (panel){
		panel->widget.type = UI_WIDGET_PANEL;
		panel->widget.id = ui_id;
		panel->widget.children.total = tButtons;
		panel->widget.children.widgets = buttonList;
		panel->widget.parent = NULL;
		panel->rect.x = x;
		panel->rect.y = y;
		panel->rect.width = width;
		panel->rect.height = height + 10;
		panel->callback.func = callback;
		panel->buttonHeight = 60;
		panel->buttonX = 10;
		panel->buttonY = 15;
		
		panel->widget.isEnabled = 0;
		//ui_disable(panel, 0);
	}
	return panel;
}

FLASHMEM static ui_button_t *ui_panel_addButton (ui_panel_t *panel, const uint8_t id, const uint8_t flags, ui_widget_cb_t callback, const char *text, const uint8_t position)
{
	ui_widget_t **buttonList = panel->widget.children.widgets;
	if (!buttonList) return NULL;

	for (int i = 0; i < panel->widget.children.total; i++){
		ui_button_t *button = (ui_button_t*)buttonList[i];
		if (!button) continue;

		if (!button->widget.id){
			button->widget.id = id;
			button->widget.flags = flags&0xFF;
			button->label.text = text;
			button->rect.y = panel->buttonY + (panel->buttonHeight * (position-1));
			button->callback.func = callback;
						
			button->widget.type = UI_WIDGET_BUTTON;
			button->widget.parent = WIDGET(panel);

			button->label.colour = COLOUR_PAL_DARKGREY;
			button->label.scale = 13;
			button->label.size = 4.0f;
			button->label.quality = 2;

			button->rect.x = panel->buttonX;
			button->rect.width = 0; //0 = auto set at render. panel->rect.width-(button->rect.x*2);
			button->rect.height = 50;
			button->offset.x = 2;
			button->offset.y = 20;

			button->widget.isEnabled = 0;
			return button;
		}
	}
	return NULL;
}

FLASHMEM static ui_button_t *ui_panel_addButtonMenu (ui_panel_t *panel, const uint8_t id, const uint8_t flags, ui_widget_cb_t callback, const char *text, const uint8_t position, const uint8_t menuId)
{
	ui_button_t *button = ui_panel_addButton(panel, id, flags, callback, text, position);
	if (button){
		//button->child.panel = menu;
		button->child.id = menuId;
	}
	return button;
}

static inline int ui_button_getPosition (const uint8_t id, uint16_t *x, uint16_t *y)
{
	ui_button_t *button = (ui_button_t*)ui_getWidget(id);
	if (button){
		*x = button->rect.x;
		*y = button->rect.y;
		return 1;
	}
	return 0;
}

static inline int ui_button_setPosition (const uint8_t id, const uint16_t x, const uint16_t y)
{
	ui_button_t *button = (ui_button_t*)ui_getWidget(id);
	if (button){
		button->rect.x = x;
		button->rect.y = y;
		return 1;
	}
	return 0;
}

FLASHMEM static void ui_panelBuild_receiver_restart ()
{
	ui_panel_t *receiver = ui_panel_create(UI_ID_PANEL_RECEIVER_RESTART, 3, uiPanel_cb, ui_menuColumn, 34, 230, 3*60);
	if (!receiver) return;
	
	widgetObjs[7] = WIDGET(receiver);
	
	ui_panel_addButton(receiver, UI_ID_BUTTON_HOTSTART,  0, uiButtons_cb, "Hotstart",  1);
	ui_panel_addButton(receiver, UI_ID_BUTTON_WARMSTART, 0, uiButtons_cb, "Warmstart", 2);
	ui_panel_addButton(receiver, UI_ID_BUTTON_COLDSTART, 0, uiButtons_cb, "Coldstart", 3);

	
	ui_enable(0, UI_ID_BUTTON_HOTSTART);
	ui_enable(0, UI_ID_BUTTON_WARMSTART);
	ui_enable(0, UI_ID_BUTTON_COLDSTART);
}

FLASHMEM static void ui_panelBuild_receiver_baud ()
{
	const int btHeight = 58;
	const uint16_t height = (8*btHeight) - 10;
	ui_panel_t *receiver = ui_panel_create(UI_ID_PANEL_RECEIVER_BAUD, 8, uiPanel_cb, ui_menuColumn, ((VHEIGHT-height)/2)-5, 180, height);
	if (!receiver) return;
	
	widgetObjs[22] = WIDGET(receiver);

	receiver->buttonHeight = btHeight;
	receiver->buttonY -= 5;
	
	ui_panel_addButton(receiver, UI_ID_BUTTON_BAUD_9600,   0, uiButtons_cb, "9600",   1);
	ui_panel_addButton(receiver, UI_ID_BUTTON_BAUD_19200,  0, uiButtons_cb, "19200",  2);
	ui_panel_addButton(receiver, UI_ID_BUTTON_BAUD_38400,  0, uiButtons_cb, "38400",  3);
	ui_panel_addButton(receiver, UI_ID_BUTTON_BAUD_57600,  0, uiButtons_cb, "57600",  4);
	ui_panel_addButton(receiver, UI_ID_BUTTON_BAUD_115200, 0, uiButtons_cb, "115200", 5);
	ui_panel_addButton(receiver, UI_ID_BUTTON_BAUD_230400, 0, uiButtons_cb, "230400", 6);
	ui_panel_addButton(receiver, UI_ID_BUTTON_BAUD_460800, 0, uiButtons_cb, "460800", 7);
	ui_panel_addButton(receiver, UI_ID_BUTTON_BAUD_921600, 0, uiButtons_cb, "921600", 8);

	ui_enable(0, UI_ID_BUTTON_BAUD_9600);
	ui_enable(0, UI_ID_BUTTON_BAUD_19200);
	ui_enable(0, UI_ID_BUTTON_BAUD_38400);
	ui_enable(0, UI_ID_BUTTON_BAUD_57600);
	ui_enable(0, UI_ID_BUTTON_BAUD_115200);
	ui_enable(0, UI_ID_BUTTON_BAUD_230400);
	ui_enable(0, UI_ID_BUTTON_BAUD_460800);
	ui_enable(0, UI_ID_BUTTON_BAUD_921600);
}

FLASHMEM static void ui_panelBuild_receiver_rate ()
{
	ui_panel_t *rate = ui_panel_create(UI_ID_PANEL_RECEIVER_RATE, 7, uiPanel_cb, ui_menuColumn, (VHEIGHT-(7*60))-20, 110, 7*60);
	if (!rate) return;
	
	widgetObjs[17] = WIDGET(rate);

	ui_panel_addButton(rate, UI_ID_BUTTON_RATE1, 0, uiButtons_cb, " 35 ", 1);
	ui_panel_addButton(rate, UI_ID_BUTTON_RATE2, 0, uiButtons_cb, " 36 ", 2);
	ui_panel_addButton(rate, UI_ID_BUTTON_RATE3, 0, uiButtons_cb, " 37 ", 3);
	ui_panel_addButton(rate, UI_ID_BUTTON_RATE4, 0, uiButtons_cb, " 38 ", 4);
	ui_panel_addButton(rate, UI_ID_BUTTON_RATE5, 0, uiButtons_cb, " 39 ", 5);
	ui_panel_addButton(rate, UI_ID_BUTTON_RATE6, 0, uiButtons_cb, " 40 ", 6);
	ui_panel_addButton(rate, UI_ID_BUTTON_RATE7, 0, uiButtons_cb, " 41 ", 7);

	ui_enable(0, UI_ID_BUTTON_RATE1);
	ui_enable(0, UI_ID_BUTTON_RATE2);
	ui_enable(0, UI_ID_BUTTON_RATE3);
	ui_enable(0, UI_ID_BUTTON_RATE4);
	ui_enable(0, UI_ID_BUTTON_RATE5);
	ui_enable(0, UI_ID_BUTTON_RATE6);
	ui_enable(0, UI_ID_BUTTON_RATE7);
}

FLASHMEM static void ui_panelBuild_receiver_gnss ()
{
	ui_panel_t *panel = ui_panel_create(UI_ID_PANEL_RECEIVER_GNSS, 7, uiPanel_cb, ui_menuColumn, (VHEIGHT-(7*60))-20, 230, 7*60);
	if (!panel) return;
	
	widgetObjs[5] = WIDGET(panel);
	
	ui_panel_addButton(panel, UI_ID_BUTTON_GPS,     0, uiButtons_cb, "GPS",     1);
	ui_panel_addButton(panel, UI_ID_BUTTON_GALILEO, 0, uiButtons_cb, "Galileo", 2);
	ui_panel_addButton(panel, UI_ID_BUTTON_BEIDOU,  0, uiButtons_cb, "BeiDou",  3);
	ui_panel_addButton(panel, UI_ID_BUTTON_GLONASS, 0, uiButtons_cb, "GLONASS", 4);
	ui_panel_addButton(panel, UI_ID_BUTTON_SBAS,    0, uiButtons_cb, "SBAS",    5);
	ui_panel_addButton(panel, UI_ID_BUTTON_QZSS,    0, uiButtons_cb, "QZSS",    6);
	ui_panel_addButton(panel, UI_ID_BUTTON_IMES,    0, uiButtons_cb, "IMES",    7);

	ui_enableNotReady(0, UI_ID_BUTTON_GPS);
	ui_enableNotReady(0, UI_ID_BUTTON_GALILEO);
	ui_enableNotReady(0, UI_ID_BUTTON_BEIDOU);
	ui_enableNotReady(0, UI_ID_BUTTON_GLONASS);
	ui_enableNotReady(0, UI_ID_BUTTON_SBAS);
	ui_enableNotReady(0, UI_ID_BUTTON_QZSS);
	ui_enableNotReady(0, UI_ID_BUTTON_IMES);
}

FLASHMEM static void ui_panelBuild_mpu_about ()
{
	ui_panel_t *panel = ui_panel_create(UI_ID_PANEL_MPU_ABOUT, 1, uiPanel_cb, 100, 20, 620, 440);
	if (!panel) return;
	
	widgetObjs[20] = WIDGET(panel);
	
	ui_panel_addButton(panel, UI_ID_BUTTON_ABOUT_CLOSE, 0, uiButtons_cb, " ", 1);
	ui_disable(0, UI_ID_BUTTON_ABOUT_CLOSE);
}

FLASHMEM static void ui_panelBuild_mpu_freq ()
{
	ui_panel_t *panel = ui_panel_create(UI_ID_PANEL_MPU_FREQ, 6, uiPanel_cb, ui_menuColumn, (VHEIGHT-(6*60))-20, 120, 6*60);
	if (!panel) return;
	
	widgetObjs[19] = WIDGET(panel);
	
	ui_panel_addButton(panel, UI_ID_BUTTON_MPU_FREQ136, 0, uiButtons_cb, "136", 1);
	ui_panel_addButton(panel, UI_ID_BUTTON_MPU_FREQ272, 0, uiButtons_cb, "272", 2);
	ui_panel_addButton(panel, UI_ID_BUTTON_MPU_FREQ528, 0, uiButtons_cb, "528", 3);
	ui_panel_addButton(panel, UI_ID_BUTTON_MPU_FREQ600, 0, uiButtons_cb, "600", 4);
	ui_panel_addButton(panel, UI_ID_BUTTON_MPU_FREQ720, 0, uiButtons_cb, "720", 5);
	ui_panel_addButton(panel, UI_ID_BUTTON_MPU_FREQ816, 0, uiButtons_cb, "816", 6);
	
	ui_enable(0, UI_ID_BUTTON_MPU_FREQ136);
	ui_enable(0, UI_ID_BUTTON_MPU_FREQ272);
	ui_enable(0, UI_ID_BUTTON_MPU_FREQ528);
	ui_enable(0, UI_ID_BUTTON_MPU_FREQ600);
	ui_enable(0, UI_ID_BUTTON_MPU_FREQ720);
	ui_enable(0, UI_ID_BUTTON_MPU_FREQ816);
}

FLASHMEM static void ui_panelBuild_map_style ()
{
	ui_panel_t *panel = ui_panel_create(UI_ID_PANEL_MAP_STYLE, 3, uiPanel_cb, ui_menuColumn, (VHEIGHT-(3*60))-20, 100, 3*60);
	if (!panel) return;
	
	widgetObjs[21] = WIDGET(panel);
	
	ui_panel_addButton(panel, UI_ID_BUTTON_STYLE1, 0, uiButtons_cb, " 1 ", 1);
	ui_panel_addButton(panel, UI_ID_BUTTON_STYLE2, 0, uiButtons_cb, " 2 ", 2);
	ui_panel_addButton(panel, UI_ID_BUTTON_STYLE3, 0, uiButtons_cb, " 3 ", 3);

	ui_enable(0, UI_ID_BUTTON_STYLE1);
	ui_enable(0, UI_ID_BUTTON_STYLE2);
	ui_enable(0, UI_ID_BUTTON_STYLE3);
}

FLASHMEM static void ui_panelBuild_logCtrl ()
{
	ui_panel_t *panel = ui_panel_create(UI_ID_PANEL_LOGCTRL, 5, uiPanel_cb, VWIDTH-250, (VHEIGHT-(4*60))-20, 230, 4*60);
	if (!panel) return;
	
	widgetObjs[9] = WIDGET(panel);

	ui_panel_addButton(panel, UI_ID_BUTTON_START,    0, uiButtons_cb, "Start", 1);
	ui_panel_addButton(panel, UI_ID_BUTTON_PAUSE,    0, uiButtons_cb, "Pause", 1);
	
	ui_panel_addButton(panel, UI_ID_BUTTON_STOP,     0, uiButtons_cb, "Stop",  2);
	ui_panel_addButton(panel, UI_ID_BUTTON_RESET,    0, uiButtons_cb, "Reset", 3);
	ui_panel_addButton(panel, UI_ID_BUTTON_empty,    0, uiButtons_cb, " ",     4);

	uint16_t x = 0;
	uint16_t y = 0;

	ui_button_getPosition(UI_ID_BUTTON_START, &x, &y);
	ui_button_setPosition(UI_ID_BUTTON_PAUSE, x, y);

	ui_enable(0, UI_ID_BUTTON_START);
	ui_disable(0, UI_ID_BUTTON_PAUSE);
	ui_enableNotReady(0, UI_ID_BUTTON_STOP);
	ui_enableNotReady(0, UI_ID_BUTTON_RESET);
}

FLASHMEM static void ui_panelBuild_logs ()
{
	int x = 100;
	if (VWIDTH < 960) x = 60;
	
	int width = 760;
	if (VWIDTH < 960) width = VWIDTH - (60+10);
	
	ui_panel_t *panel = ui_panel_create(UI_ID_PANEL_LOGS, 1, uiLogs_cb, x, 14, width, 440);
	if (!panel) return;
	
	widgetObjs[12] = WIDGET(panel);
	
	ui_panel_addButton(panel, UI_ID_BUTTON_empty, 0, uiButtons_cb, " ", 1);
}


FLASHMEM static void ui_panelBuild_display ()
{
	ui_panel_t *panel = ui_panel_create(UI_ID_PANEL_DISPLAY, 8, uiPanel_cb, ui_menuColumn, 2, 384, (8*59)-7);
	if (!panel) return;
	
	widgetObjs[4] = WIDGET(panel);
	
	panel->buttonY = 10;
	panel->buttonHeight = 59;
	
	ui_panel_addButton(panel, UI_ID_BUTTON_DRAW_SATW,  0, uiButtons_cb, "Satelite World",        1);
	ui_panel_addButton(panel, UI_ID_BUTTON_DRAW_AVAIL, 0, uiButtons_cb, "Satelite Availability", 2);
	ui_panel_addButton(panel, UI_ID_BUTTON_DRAW_LVLS,  0, uiButtons_cb, "Satelite Levels",       3);
	ui_panel_addButton(panel, UI_ID_BUTTON_DRAW_COMP,  0, uiButtons_cb, "Compass",               4);
	ui_panel_addButton(panel, UI_ID_BUTTON_DRAW_RULER, 0, uiButtons_cb, "Measure",               5);
	ui_panel_addButton(panel, UI_ID_BUTTON_DRAW_LOC,   0, uiButtons_cb, "Position Arrow",        6);
	ui_panel_addButton(panel, UI_ID_BUTTON_DRAW_TEXT,  0, uiButtons_cb, "Strings",               7);
	ui_panel_addButton(panel, UI_ID_BUTTON_DRAW_ROUTE, 0, uiButtons_cb, "Route",                 8);
	
	ui_enable(0, UI_ID_BUTTON_DRAW_SATW);
	ui_enable(0, UI_ID_BUTTON_DRAW_AVAIL);
	ui_enable(0, UI_ID_BUTTON_DRAW_LVLS);
	ui_enable(0, UI_ID_BUTTON_DRAW_COMP);
	ui_enable(0, UI_ID_BUTTON_DRAW_RULER);
	ui_enable(0, UI_ID_BUTTON_DRAW_LOC);
	ui_enable(0, UI_ID_BUTTON_DRAW_TEXT);
	ui_enable(0, UI_ID_BUTTON_DRAW_ROUTE);
	
	ui_setHighlight(UI_ID_BUTTON_DRAW_SATW,  1);
	ui_setHighlight(UI_ID_BUTTON_DRAW_AVAIL, 1);
	ui_setHighlight(UI_ID_BUTTON_DRAW_LVLS,  0);
	ui_setHighlight(UI_ID_BUTTON_DRAW_COMP,  0);
	ui_setHighlight(UI_ID_BUTTON_DRAW_RULER, 0);
	ui_setHighlight(UI_ID_BUTTON_DRAW_LOC,   1);
	ui_setHighlight(UI_ID_BUTTON_DRAW_TEXT,  1);
	ui_setHighlight(UI_ID_BUTTON_DRAW_ROUTE, 1);
}


FLASHMEM static void ui_panelBuild_map ()
{
	ui_panel_t *panel = ui_panel_create(UI_ID_PANEL_MAP, 6, uiPanel_cb, ui_menuColumn, (VHEIGHT-(6*60))-20, 340, 6*60);
	if (!panel) return;
	
	widgetObjs[18] = WIDGET(panel);
		
	ui_panel_addButton(panel, UI_ID_BUTTON_AREAFILL,    0, uiButtons_cb, "Area Fill",     1);
	ui_panel_addButton(panel, UI_ID_BUTTON_AREAOUTLINR, 0, uiButtons_cb, "Area Outline",  2);
	ui_panel_addButton(panel, UI_ID_BUTTON_PATHFILL,    0, uiButtons_cb, "Path Fill",     3);
	ui_panel_addButton(panel, UI_ID_BUTTON_FATHLINE,    0, uiButtons_cb, "Path Line",     4);
	ui_panel_addButton(panel, UI_ID_BUTTON_TITLEBOUND,  0, uiButtons_cb, "Tile Boundary", 5);
	ui_panel_addButtonMenu(panel, UI_ID_BUTTON_SCHEME, UI_WIDGET_FLAG_HASPANEL, uiButtons_cb, "Style", 6, UI_ID_PANEL_MAP_STYLE);
	

	ui_enable(0, UI_ID_BUTTON_AREAFILL);
	ui_enable(0, UI_ID_BUTTON_AREAOUTLINR);
	ui_enable(0, UI_ID_BUTTON_PATHFILL);
	ui_enable(0, UI_ID_BUTTON_FATHLINE);
	ui_enable(0, UI_ID_BUTTON_TITLEBOUND);
	ui_enable(0, UI_ID_BUTTON_SCHEME);
	
	ui_setHighlight(UI_ID_BUTTON_AREAFILL, 1);
	ui_setHighlight(UI_ID_BUTTON_PATHFILL, 1);
}


FLASHMEM static void ui_panelBuild_receiver ()
{
	const int btHeight = 58;
	const uint16_t height = (8*btHeight) - 10;
	
	ui_panel_t *receiver = ui_panel_create(UI_ID_PANEL_RECEIVER, 8, uiPanel_cb, ui_menuColumn, ((VHEIGHT-height)/2)-5, 230, height);
	if (!receiver) return;
	
	widgetObjs[2] = WIDGET(receiver);
	
	receiver->buttonHeight = btHeight;
	receiver->buttonY -= 5;
	
	ui_panel_addButtonMenu(receiver, UI_ID_BUTTON_BAUD, UI_WIDGET_FLAG_HASPANEL, uiButtons_cb, "Baud", 1, UI_ID_PANEL_RECEIVER_BAUD);
	ui_panel_addButton(receiver, UI_ID_BUTTON_REINIT,      0, uiButtons_cb, "Reinit",    2);
	ui_panel_addButton(receiver, UI_ID_BUTTON_RECONNECT,   0, uiButtons_cb, "Reconnect", 3);
	ui_panel_addButton(receiver, UI_ID_BUTTON_STATUS,      0, uiButtons_cb, "Status",    4);
	ui_panel_addButton(receiver, UI_ID_BUTTON_VERSION,     0, uiButtons_cb, "Version",   5);
	ui_panel_addButtonMenu(receiver, UI_ID_BUTTON_RECEIVER_RESTART, UI_WIDGET_FLAG_HASPANEL, uiButtons_cb, "Restart", 6, UI_ID_PANEL_RECEIVER_RESTART);
	ui_panel_addButtonMenu(receiver, UI_ID_BUTTON_RATE, UI_WIDGET_FLAG_HASPANEL, uiButtons_cb, "Rate", 7, UI_ID_PANEL_RECEIVER_RATE);
	ui_panel_addButtonMenu(receiver, UI_ID_BUTTON_GNSS, UI_WIDGET_FLAG_HASPANEL, uiButtons_cb, "GNSS", 8, UI_ID_PANEL_RECEIVER_GNSS);
	
	ui_enable(0, UI_ID_BUTTON_BAUD);
	ui_enable(0, UI_ID_BUTTON_RECEIVER_RESTART);
	ui_enable(0, UI_ID_BUTTON_REINIT);
	ui_enable(0, UI_ID_BUTTON_RECONNECT);
	ui_enable(0, UI_ID_BUTTON_STATUS);
	ui_enable(0, UI_ID_BUTTON_VERSION);
	ui_enable(0, UI_ID_BUTTON_RATE);
	ui_enable(0, UI_ID_BUTTON_GNSS);
}

FLASHMEM static void ui_panelBuild_mpu ()
{
	ui_panel_t *panel = ui_panel_create(UI_ID_PANEL_MPU, 3, uiPanel_cb, ui_menuColumn, (VHEIGHT-(3*60))-20, 252, 3*60);
	if (!panel) return;
	
	widgetObjs[3] = WIDGET(panel);
	
	ui_panel_addButton(panel, UI_ID_BUTTON_REBOOT, 0, uiButtons_cb, "Reboot", 1);
	ui_panel_addButtonMenu(panel, UI_ID_BUTTON_MPU_FREQ,   UI_WIDGET_FLAG_HASPANEL, uiButtons_cb, "Frequency", 2, UI_ID_PANEL_MPU_FREQ);
	ui_panel_addButtonMenu(panel, UI_ID_BUTTON_MPU_STATUS, UI_WIDGET_FLAG_HASPOPUP, uiButtons_cb, "About",     3, UI_ID_PANEL_MPU_ABOUT);
	
	ui_enable(0, UI_ID_BUTTON_REBOOT);
	ui_enable(0, UI_ID_BUTTON_MPU_FREQ);
	ui_enable(0, UI_ID_BUTTON_MPU_STATUS);
}

FLASHMEM static void ui_panelBuild_menu ()
{
	int tbuttons = 5;
#if (!TFT_LOWERPANEL)
	tbuttons++;
#endif

	ui_panel_t *panel = ui_panel_create(UI_ID_PANEL_MENU, tbuttons, uiPanel_cb, ui_menuColumn, (VHEIGHT-(tbuttons*60))-20, 230, tbuttons*60);
	if (!panel) return;
	
	widgetObjs[1] = WIDGET(panel);

	ui_panel_addButtonMenu(panel, UI_ID_BUTTON_LOGS,     UI_WIDGET_FLAG_HASPANEL, uiButtons_cb, "Logs",     1, UI_ID_PANEL_LOGS);
	ui_panel_addButtonMenu(panel, UI_ID_BUTTON_DISPLAY,  UI_WIDGET_FLAG_HASPANEL, uiButtons_cb, "Display",  2, UI_ID_PANEL_DISPLAY);
	ui_panel_addButtonMenu(panel, UI_ID_BUTTON_MAP,      UI_WIDGET_FLAG_HASPANEL, uiButtons_cb, "Map",      3, UI_ID_PANEL_MAP);
	ui_panel_addButtonMenu(panel, UI_ID_BUTTON_RECEIVER, UI_WIDGET_FLAG_HASPANEL, uiButtons_cb, "Receiver", 4, UI_ID_PANEL_RECEIVER);
	ui_panel_addButtonMenu(panel, UI_ID_BUTTON_MPU,      UI_WIDGET_FLAG_HASPANEL, uiButtons_cb, "MPU",      5, UI_ID_PANEL_MPU);
	
#if (!TFT_LOWERPANEL)
	ui_panel_addButtonMenu(panel, UI_ID_BUTTON_LOGCTRL,  UI_WIDGET_FLAG_HASPANEL, uiButtons_cb, "Log",      6, UI_ID_PANEL_LOGCTRL);
	ui_enable(0, UI_ID_BUTTON_LOGCTRL);
#endif

	ui_enable(0, UI_ID_BUTTON_LOGS);
	ui_enable(0, UI_ID_BUTTON_DISPLAY);
	ui_enable(0, UI_ID_BUTTON_MAP);
	ui_enable(0, UI_ID_BUTTON_RECEIVER);
	ui_enable(0, UI_ID_BUTTON_MPU);
}

int ui_input (const int32_t x, const int32_t y, const uint32_t flags)
{
	uint32_t handledBy = 0;
	
	int ret = ui_input(widgetObjs, UI_WIDGETOBJS_TOTAL, x, y, &handledBy);
	if (!ret){
		if (ui_activePanelId){
			ui_disable(NULL, ui_activePanelId);
			ui_activePanelId = 0;
		}
		ui_enable(NULL, UI_ID_BUTTON_CONFIG);
	}
	return ret;
}


/*
#######################################################################################################
#######################################################################################################
#######################################################################################################
#######################################################################################################
*/

FLASHMEM void uiBuild ()
{
	
	// Logs >
	ui_panelBuild_logs();
	
	// Display >
	ui_panelBuild_display();

	// Map >
	ui_panelBuild_map_style();
	ui_panelBuild_map();

	// Receiver >
	ui_panelBuild_receiver_rate();
	ui_panelBuild_receiver_restart();
	ui_panelBuild_receiver_baud();
	ui_panelBuild_receiver_gnss();
	ui_panelBuild_receiver();
		
	// MPU > 
	ui_panelBuild_mpu_freq();
	ui_panelBuild_mpu_about();
	ui_panelBuild_mpu();

	// Log >
	ui_panelBuild_logCtrl();
	
	ui_panelBuild_menu();
	
	
	// config toggle button
	ui_button_t *button = &button_config;
	memset(button, 0, sizeof(*button));

	button->widget.type = UI_WIDGET_BUTTON;
	button->widget.id = UI_ID_BUTTON_CONFIG;
	button->widget.children.total = 0;
	button->widget.children.widgets = NULL;
	button->widget.parent = NULL;
	button->label.text = "Config";
	button->label.colour = COLOUR_PAL_REDFUZZ;
	button->label.scale = 13;
	button->label.size = 4.0f;
	button->label.quality = 2;
	button->rect.x = 120;
	button->rect.y = VHEIGHT-52;
	button->rect.width = 140;
	button->rect.height = 70;
	button->offset.x = 8;
	button->offset.y = 22;
	if (TFT_LOWERPANEL){
		button->label.text = NULL;
		button->rect.x = 20;
		button->rect.y = 480;
		button->rect.width = 100;
		button->rect.height = 60;
		button->offset.x = 0;
		button->offset.y = 0;
	}

	
	button->callback.func = uiButtons_cb;
	ui_enable(NULL, UI_ID_BUTTON_CONFIG);
	
	if (!TFT_LOWERPANEL)
		return;

	// overlay detail button
	button = &button_overlayDetail;
	memset(button, 0, sizeof(*button));

	button->widget.type = UI_WIDGET_BUTTON;
	button->widget.id = UI_ID_BUTTON_OVERLAYDETAIL;
	button->widget.children.total = 0;
	button->widget.children.widgets = NULL;
	button->widget.parent = NULL;
	button->label.text = NULL;
	button->rect.x = 440;
	button->rect.y = 480;
	button->rect.width = 80;
	button->rect.height = 60;
	button->offset.x = 0;
	button->offset.y = 0;
	
	button->callback.func = uiButtons_cb;
	ui_enable(NULL, UI_ID_BUTTON_OVERLAYDETAIL);
	
		
	// log Control
	button = &button_logCtrl;
	memset(button, 0, sizeof(*button));

	button->widget.type = UI_WIDGET_BUTTON;
	button->widget.id = UI_ID_BUTTON_LOGCTRL;
	button->widget.children.total = 0;
	button->widget.children.widgets = NULL;
	button->widget.parent = NULL;
	button->label.text = NULL;
	button->rect.x = 840;
	button->rect.y = 480;
	button->rect.width = 80;
	button->rect.height = 60;
	button->offset.x = 0;
	button->offset.y = 0;
	
	button->callback.func = uiButtons_cb;
	ui_enable(NULL, UI_ID_BUTTON_LOGCTRL);
	
	
	// zoom In
	button = &button_zoom[0];
	memset(button, 0, sizeof(*button));

	button->widget.type = UI_WIDGET_BUTTON;
	button->widget.id = UI_ID_BUTTON_ZOOM_IN;
	button->widget.children.total = 0;
	button->widget.children.widgets = NULL;
	button->widget.parent = NULL;
	button->label.text = NULL;
	button->rect.x = 600;
	button->rect.y = 480;
	button->rect.width = 80;
	button->rect.height = 60;
	button->offset.x = 0;
	button->offset.y = 0;
	
	button->callback.func = uiButtons_cb;
	ui_enable(NULL, UI_ID_BUTTON_ZOOM_IN);
	
	// zoom Out
	button = &button_zoom[1];
	memset(button, 0, sizeof(*button));

	button->widget.type = UI_WIDGET_BUTTON;
	button->widget.id = UI_ID_BUTTON_ZOOM_OUT;
	button->widget.children.total = 0;
	button->widget.children.widgets = NULL;
	button->widget.parent = NULL;
	button->label.text = NULL;
	button->rect.x = 710;
	button->rect.y = 480;
	button->rect.width = 80;
	button->rect.height = 60;
	button->offset.x = 0;
	button->offset.y = 0;
	
	button->callback.func = uiButtons_cb;
	ui_enable(NULL, UI_ID_BUTTON_ZOOM_OUT);
	
	// Up
	button = &button_updown[0];
	memset(button, 0, sizeof(*button));

	button->widget.type = UI_WIDGET_BUTTON;
	button->widget.id = UI_ID_BUTTON_UP;
	button->widget.children.total = 0;
	button->widget.children.widgets = NULL;
	button->widget.parent = NULL;
	button->label.text = NULL;
	button->rect.x = 600;
	button->rect.y = 480;
	button->rect.width = 80;
	button->rect.height = 60;
	button->offset.x = 0;
	button->offset.y = 0;
	
	button->callback.func = uiButtons_cb;
	ui_disable(NULL, UI_ID_BUTTON_UP);
	
	// Down
	button = &button_updown[1];
	memset(button, 0, sizeof(*button));

	button->widget.type = UI_WIDGET_BUTTON;
	button->widget.id = UI_ID_BUTTON_DOWN;
	button->widget.children.total = 0;
	button->widget.children.widgets = NULL;
	button->widget.parent = NULL;
	button->label.text = NULL;
	button->rect.x = 710;
	button->rect.y = 480;
	button->rect.width = 80;
	button->rect.height = 60;
	button->offset.x = 0;
	button->offset.y = 0;
	
	button->callback.func = uiButtons_cb;
	ui_disable(NULL, UI_ID_BUTTON_DOWN);

	// Log Refresh
	button = &button_logRefresh;
	memset(button, 0, sizeof(*button));

	button->widget.type = UI_WIDGET_BUTTON;
	button->widget.id = UI_ID_BUTTON_REFRESH;
	button->widget.children.total = 0;
	button->widget.children.widgets = NULL;
	button->widget.parent = NULL;
	button->label.text = NULL;
	button->rect.x = 440;
	button->rect.y = 480;
	button->rect.width = 80;
	button->rect.height = 60;
	button->offset.x = 0;
	button->offset.y = 0;
	
	button->callback.func = uiButtons_cb;
	ui_disable(NULL, UI_ID_BUTTON_REFRESH);
	
	// log Load
	button = &button_logLoad;
	memset(button, 0, sizeof(*button));

	button->widget.type = UI_WIDGET_BUTTON;
	button->widget.id = UI_ID_BUTTON_LOAD;
	button->widget.children.total = 0;
	button->widget.children.widgets = NULL;
	button->widget.parent = NULL;
	button->label.text = NULL;
	button->rect.x = 840;
	button->rect.y = 480;
	button->rect.width = 80;
	button->rect.height = 60;
	button->offset.x = 0;
	button->offset.y = 0;
	
	button->callback.func = uiButtons_cb;
	ui_disable(NULL, UI_ID_BUTTON_LOAD);
}

void ui_init ()
{
	uiLogs_clear();
	uiBuild();
}

void ui_draw (const uint32_t unused1, const uint32_t unused2)
{
	ui_render(widgetObjs, UI_WIDGETOBJS_TOTAL, 0, 0, 0);
}

