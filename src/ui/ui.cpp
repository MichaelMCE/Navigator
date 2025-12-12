


#include "../commonGlue.h"





extern trackRecord_t trackRecord;
extern application_t inst;



static ui_button_t button_config;

#define UI_WIDGETOBJS_TOTAL		5
static ui_widget_t *widgetObjs[UI_WIDGETOBJS_TOTAL] = {WIDGET(&button_config)};



static uint32_t opFuncs[OP_QUEUE_LENGTH];	// FIFO
static uint32_t opPosition = 0;
static uint32_t opState = OP_IDLE;




static void ui_draw_button (ui_all_t *widget, const int32_t x, const int32_t y)
{
	ui_button_t *button = (ui_button_t*)widget;

	//drawRectangle(x, y, x+button->rect.width-1, y+button->rect.height-1, COLOUR_PAL_BLACK);

	setBrush(inst.vfont, BRUSH_DISK);
	setGlyphScale(inst.vfont, button->label.scale/10.0f);
	setBrushSize(inst.vfont, button->label.size);
	setBrushQuality(inst.vfont, button->label.quality);
	setBrushColour(inst.vfont, button->label.colour);

	drawString(inst.vfont, button->label.text, x+button->offset.x, y+button->offset.y);
}

static void ui_draw_panel (ui_all_t *widget, const int32_t x, const int32_t y)
{
	ui_panel_t *panel = (ui_panel_t*)widget;
	
	drawRectangleFilled(x+1, y+1, x+panel->rect.width-2, y+panel->rect.height-2, COLOUR_PAL_LIGHTERGREY);
	drawRectangle(x, y, x+panel->rect.width-1, y+panel->rect.height-1, COLOUR_PAL_ORANGE);

	if (1 && trackRecord.date[0]){
		setGlyphScale(inst.vfont, 0.8f);
		setBrushSize(inst.vfont, 1.0f);
		setBrushQuality(inst.vfont, 1);
		setBrushColour(inst.vfont, COLOUR_PAL_BLACK);

		drawString(inst.vfont, trackRecord.date, x+2, y+(panel->rect.height/2));
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

			if (widget->type == UI_WIDGET_PANEL)
				ui_draw_panel(obj, childX, childY);
			else if (widget->type == UI_WIDGET_BUTTON)
				ui_draw_button(obj, childX, childY);

			if (widget->children.total)
				ui_render(widget->children.widgets, widget->children.total, childX, childY, flags);
		}
	}
}

static int ui_input (ui_widget_t **widgets, const uint8_t total, const int32_t x, const int32_t y, const uint32_t flags)
{
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
						ret += obj->callback.func(widget, widget->id, flags);
					}
				}
			}else if (widget->type == UI_WIDGET_BUTTON){
				ui_all_t *obj = (ui_all_t*)widget;
				localX = obj->rect.x;
				localY = obj->rect.y;

				if (childX >= obj->rect.x && childY >= obj->rect.y){
					if (childX < obj->rect.x+obj->rect.width && childY < obj->rect.y+obj->rect.height){
						if (obj->callback.func(widget, widget->id, flags)){
							ret++;
							break;
						}
					}
				}
			}

			if (widget->children.total){
				ui_widget_t *objs[widget->children.total];
				for (int j = 0; j < widget->children.total; j++)
					objs[j] = CHILD_WIDGET_OBJ(widget,j);

				ret += ui_input(objs, widget->children.total, childX-localX, childY-localY, flags);
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

	if (!child_id && obj){
		obj->isEnabled = UI_WIDGET_DISABLED;
		return 1;
		
	}else if (obj == NULL && child_id){
		for (int i = 0; i < UI_WIDGETOBJS_TOTAL; i++){
			obj = widgetObjs[i];
			if (obj->id == child_id){
				obj->isEnabled = UI_WIDGET_DISABLED;
				return 1;
			}else{
				for (int j = 0; j < obj->children.total; j++){
					ui_widget_t *widget = CHILD_WIDGET_OBJ(obj,j);
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
static uint32_t op_push (const uint32_t op_code)
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

static inline void op_go ()
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

int32_t op_execute (const uint32_t opCode)
{
	switch (opCode){
	case 0:
		op_halt();
		return 0;
		
	case OP_FUNC_LOG_START:
		log_start();
		return 1;
		
	case OP_FUNC_LOG_STOP:
		log_stop();
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
		render_cycleMode();
		return 1;

	case OP_FUNC_OFF:
		tft_setBacklight(0);
		return 0;

	case OP_FUNC_HOTSTART:
		gps_hotStart();
		return 0;
		
	case OP_FUNC_WARMSTART:
		gps_warmStart();
		return 0;
		
	case OP_FUNC_COLDSTART:
		gps_coldStart();
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
		return 0;

	case OP_FUNC_RECONNECT:
		gps_reconnect();
		return 0;

	case OP_FUNC_REBOOT:
		mpu_reboot();
		return 0;
		
	case OP_FUNC_CONFIG:
		return 0;
	};
	
	return 0;
}

int uiButtons_cb (ui_widget_t *widget, const uint8_t id, const uint32_t flags)
{
	switch (id){
	case UI_ID_BUTTON_START:
		ui_disable(NULL, UI_ID_BUTTON_START);
		ui_enable(NULL, UI_ID_BUTTON_PAUSE);
		ui_enable(NULL, UI_ID_BUTTON_STOP);
		ui_enable(NULL, UI_ID_BUTTON_RESET);
		op_push(OP_FUNC_LOG_START);
		op_go();
		//render_signalUpdate();
		return 1;
		
	case UI_ID_BUTTON_STOP:
		ui_enable(NULL, UI_ID_BUTTON_START);
		ui_disable(NULL, UI_ID_BUTTON_PAUSE);
		ui_disable(NULL, UI_ID_BUTTON_STOP);
		op_push(OP_FUNC_LOG_STOP);
		op_go();
		//render_signalUpdate();
		return 1;
		
	case UI_ID_BUTTON_PAUSE: 
		ui_enable(NULL, UI_ID_BUTTON_START);
		ui_disable(NULL, UI_ID_BUTTON_PAUSE);
		ui_enable(NULL, UI_ID_BUTTON_STOP);
		op_push(OP_FUNC_LOG_PAUSE);
		op_go();
		//render_signalUpdate();
		return 1;

	case UI_ID_BUTTON_RESET:
		ui_enable(NULL, UI_ID_BUTTON_START);
		ui_disable(NULL, UI_ID_BUTTON_PAUSE);
		ui_disable(NULL, UI_ID_BUTTON_STOP);
		ui_disable(NULL, UI_ID_BUTTON_RESET);

		op_push(OP_FUNC_LOG_RESET);
		op_go();
		return 1;

	case UI_ID_BUTTON_MODE:
		op_push(OP_FUNC_MODE);
		op_go();
		return 1;

	case UI_ID_BUTTON_OFF:
		op_push(OP_FUNC_OFF);
		op_go();
		return 1;

	case UI_ID_BUTTON_DISPLAY:
		ui_disable(NULL, UI_ID_PANEL_MAPCTRL);
		ui_enable(NULL, UI_ID_PANEL_DISPLAY);
		return 1;
		
	case UI_ID_BUTTON_RECEIVER:
		ui_disable(NULL, UI_ID_PANEL_MAPCTRL);
		ui_enable(NULL, UI_ID_PANEL_RECEIVER);
		return 1;

	case UI_ID_BUTTON_MPU:
		ui_disable(NULL, UI_ID_PANEL_MAPCTRL);
		ui_enable(NULL, UI_ID_PANEL_MPU);
		return 1;

	case UI_ID_BUTTON_HOTSTART:
		op_push(OP_FUNC_HOTSTART);
		op_go();
		return 1;

	case UI_ID_BUTTON_FREQ528:
		op_push(OP_FUNC_FREQ528);
		op_go();
		return 0;

	case UI_ID_BUTTON_FREQ600:
		op_push(OP_FUNC_FREQ600);
		op_go();
		return 0;

	case UI_ID_BUTTON_FREQ720:
		op_push(OP_FUNC_FREQ720);
		op_go();
		return 0;
		
	case UI_ID_BUTTON_FREQ816:
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
		op_push(OP_FUNC_REINIT);
		op_go();
		return 1;
		
	case UI_ID_BUTTON_RECONNECT:
		op_push(OP_FUNC_RECONNECT);
		op_go();
		return 1;

	case UI_ID_BUTTON_REBOOT:
		op_push(OP_FUNC_REBOOT);
		op_go();
		return 1;
	
	case UI_ID_BUTTON_CONFIG:	// toggle mapCtrl panel
		if (ui_isEnabled(NULL, UI_ID_PANEL_MAPCTRL)  || 
			ui_isEnabled(NULL, UI_ID_PANEL_RECEIVER) || 
			ui_isEnabled(NULL, UI_ID_PANEL_DISPLAY)  || 
			ui_isEnabled(NULL, UI_ID_PANEL_MPU))
		  {
				
			ui_disable(NULL, UI_ID_PANEL_MAPCTRL);
			ui_disable(NULL, UI_ID_PANEL_RECEIVER);
			ui_disable(NULL, UI_ID_PANEL_MPU);
			ui_disable(NULL, UI_ID_PANEL_DISPLAY);
		}else{
			ui_enable(NULL, UI_ID_PANEL_MAPCTRL);
		}
		return 1;
	}
	return 0;
}

int uiPanel_cb (ui_widget_t *opaque, const uint8_t id, const uint32_t flags)
{
	//printf(CS("uiPanelMapCtrl_cb: id:%i"), id);

	/*ui_panel_t *panel = (ui_panel_t*)opaque;

	switch (id){
		
	}*/
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

		panel->widget.isEnabled = 0;
		//ui_disable(panel, 0);
	}
	return panel;
}

FLASHMEM static ui_button_t *ui_panel_addButton (ui_panel_t *panel, const uint8_t id, ui_widget_cb_t callback, const char *text, const uint8_t position)
{
	ui_widget_t **buttonList = panel->widget.children.widgets;
	if (!buttonList) return NULL;

	for (int i = 0; i < panel->widget.children.total; i++){
		ui_button_t *button = (ui_button_t*)buttonList[i];
		if (!button) continue;

		if (!button->widget.id){
			button->widget.id = id;
			button->label.text = text;
			button->rect.y = 10 + (50 * (position-1));
			button->callback.func = callback;
						
			button->widget.type = UI_WIDGET_BUTTON;
			button->widget.parent = WIDGET(panel);

			button->label.colour = COLOUR_PAL_DARKGREY;
			button->label.scale = 13;
			button->label.size = 4.0f;
			button->label.quality = 2;

			button->rect.x = 10;
			button->rect.width = panel->rect.width-(button->rect.x*2);
			button->rect.height = 40;
			button->offset.x = 2;
			button->offset.y = 20;

			button->widget.isEnabled = 0;
			return button;
		}
	}
	return NULL;
}

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

FLASHMEM static void ui_receiverPanelBuild ()
{
	ui_panel_t *receiver = ui_panel_create(UI_ID_PANEL_RECEIVER, 5, uiPanel_cb, 60, 34, 230, 5*50);
	if (!receiver) return;
	
	widgetObjs[2] = WIDGET(receiver);
	
	ui_panel_addButton(receiver, UI_ID_BUTTON_HOTSTART,  uiButtons_cb, "Hotstart",  1);
	ui_panel_addButton(receiver, UI_ID_BUTTON_WARMSTART, uiButtons_cb, "Warmstart", 2);
	ui_panel_addButton(receiver, UI_ID_BUTTON_COLDSTART, uiButtons_cb, "Coldstart", 3);
	ui_panel_addButton(receiver, UI_ID_BUTTON_REINIT,    uiButtons_cb, "Reinit",    4);
	ui_panel_addButton(receiver, UI_ID_BUTTON_RECONNECT, uiButtons_cb, "Reconnect", 5);
	
	ui_enable(0, UI_ID_BUTTON_HOTSTART);
	ui_enable(0, UI_ID_BUTTON_WARMSTART);
	ui_enable(0, UI_ID_BUTTON_COLDSTART);
	ui_enable(0, UI_ID_BUTTON_REINIT);
	ui_enable(0, UI_ID_BUTTON_RECONNECT);
}

FLASHMEM static void ui_mpuPanelBuild ()
{
	ui_panel_t *panel = ui_panel_create(UI_ID_PANEL_MPU, 6, uiPanel_cb, 80, 60, 230, 6*50);
	if (!panel) return;
	
	widgetObjs[3] = WIDGET(panel);
	
	ui_panel_addButton(panel, UI_ID_BUTTON_REBOOT,  uiButtons_cb, "Reboot",    1);
	ui_panel_addButton(panel, UI_ID_BUTTON_empty,   uiButtons_cb, " ",         2);
	ui_panel_addButton(panel, UI_ID_BUTTON_FREQ528, uiButtons_cb, "Freq: 528", 3);
	ui_panel_addButton(panel, UI_ID_BUTTON_FREQ600, uiButtons_cb, "Freq: 600", 4);
	ui_panel_addButton(panel, UI_ID_BUTTON_FREQ720, uiButtons_cb, "Freq: 720", 5);
	ui_panel_addButton(panel, UI_ID_BUTTON_FREQ816, uiButtons_cb, "Freq: 816", 6);
	
	ui_enable(0, UI_ID_BUTTON_REBOOT);
	ui_enable(0, UI_ID_BUTTON_FREQ528);
	ui_enable(0, UI_ID_BUTTON_FREQ600);
	ui_enable(0, UI_ID_BUTTON_FREQ720);
	ui_enable(0, UI_ID_BUTTON_FREQ816);
}

FLASHMEM static void ui_displayPanelBuild ()
{
	ui_panel_t *panel = ui_panel_create(UI_ID_PANEL_DISPLAY, 2, uiPanel_cb, 50, 70, 160, 2*50);
	if (!panel) return;
	
	widgetObjs[4] = WIDGET(panel);
	
	ui_panel_addButton(panel, UI_ID_BUTTON_MODE, uiButtons_cb, "Mode", 1);
	ui_panel_addButton(panel, UI_ID_BUTTON_OFF,  uiButtons_cb, "Off",  2);
	
	ui_enable(0, UI_ID_BUTTON_MODE);
	ui_enable(0, UI_ID_BUTTON_OFF);
}

FLASHMEM static void ui_ctrlPanelBuild ()
{
	ui_panel_t *panel = ui_panel_create(UI_ID_PANEL_MAPCTRL, 8, uiPanel_cb, 30, 34, 230, 7*50);
	if (!panel) return;
	
	widgetObjs[1] = WIDGET(panel);

	ui_panel_addButton(panel, UI_ID_BUTTON_START,    uiButtons_cb, "Start",    1);
	ui_panel_addButton(panel, UI_ID_BUTTON_PAUSE,    uiButtons_cb, "Pause",    1);
	ui_panel_addButton(panel, UI_ID_BUTTON_STOP,     uiButtons_cb, "Stop",     2);
	ui_panel_addButton(panel, UI_ID_BUTTON_RESET,    uiButtons_cb, "Reset",    3);
	ui_panel_addButton(panel, UI_ID_BUTTON_empty,    uiButtons_cb, " ",        4);
	ui_panel_addButton(panel, UI_ID_BUTTON_DISPLAY,  uiButtons_cb, "Display",  5);
	ui_panel_addButton(panel, UI_ID_BUTTON_RECEIVER, uiButtons_cb, "Receiver", 6);
	ui_panel_addButton(panel, UI_ID_BUTTON_MPU,      uiButtons_cb, "MPU",      7);

	uint16_t x = 0;
	uint16_t y = 0;

	ui_button_getPosition(UI_ID_BUTTON_START, &x, &y);
	ui_button_setPosition(UI_ID_BUTTON_PAUSE, x, y);

	ui_enable(0, UI_ID_BUTTON_START);
	ui_enable(0, UI_ID_BUTTON_DISPLAY);
	ui_enable(0, UI_ID_BUTTON_RECEIVER);
	ui_enable(0, UI_ID_BUTTON_MPU);
}

void ui_draw (const uint32_t unused1, const uint32_t unused2)
{
	ui_render(widgetObjs, UI_WIDGETOBJS_TOTAL, 0, 0, 0);
}

int ui_input (const int32_t x, const int32_t y, const uint32_t flags)
{
	int ret = ui_input(widgetObjs, UI_WIDGETOBJS_TOTAL, x, y, flags);
	if (!ret){
		if (ui_isEnabled(NULL, UI_ID_PANEL_MAPCTRL)){
			ui_disable(NULL, UI_ID_PANEL_MAPCTRL);
			return 1;

		}else if (ui_isEnabled(NULL, UI_ID_PANEL_RECEIVER)){
			ui_disable(NULL, UI_ID_PANEL_RECEIVER);
			return 1;

		}else if (ui_isEnabled(NULL, UI_ID_PANEL_MPU)){
			ui_disable(NULL, UI_ID_PANEL_MPU);
			return 1;

		}else if (ui_isEnabled(NULL, UI_ID_PANEL_DISPLAY)){
			ui_disable(NULL, UI_ID_PANEL_DISPLAY);
			return 1;
		}
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
	
	ui_ctrlPanelBuild();
	ui_receiverPanelBuild();
	ui_mpuPanelBuild();
	ui_displayPanelBuild();

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
	button->rect.x = 50;
	button->rect.y = VHEIGHT-60;
	button->rect.width = 130;
	button->rect.height = 50;
	button->offset.x = 8;
	button->offset.y = 22;
	
	button->callback.func = uiButtons_cb;
	ui_enable(NULL, UI_ID_BUTTON_CONFIG);
}

void ui_init ()
{

	uiBuild();
}
