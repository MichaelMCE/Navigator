


#include "../commonGlue.h"





extern trackRecord_t trackRecord;
extern application_t inst;



static ui_button_t button_config;
static ui_button_t panel_mapCtrl_buttons[6];
static ui_button_t *panelButtons[6] = {&panel_mapCtrl_buttons[0], &panel_mapCtrl_buttons[1], &panel_mapCtrl_buttons[2], &panel_mapCtrl_buttons[3], &panel_mapCtrl_buttons[4], &panel_mapCtrl_buttons[5]};
static ui_panel_t panel_mapCtrl;

#define UI_WIDGETOBJS_TOTAL		2
static ui_widget_t *widgetObjs[UI_WIDGETOBJS_TOTAL] = {WIDGET(&panel_mapCtrl), WIDGET(&button_config)};



static uint32_t opFuncs[OP_QUEUE_LENGTH];	// FIFO
static uint32_t opPosition = 0;
static uint32_t opState = OP_IDLE;






static void ui_draw_button (ui_button_t *button, const int32_t x, const int32_t y)
{
	//drawRectangle(x, y, x+button->rect.width-1, y+button->rect.height-1, COLOUR_PAL_BLACK);

	setBrush(inst.vfont, BRUSH_DISK);
	setGlyphScale(inst.vfont, button->label.scale/10.0f);
	setBrushSize(inst.vfont, button->label.size);
	setBrushQuality(inst.vfont, button->label.quality);
	setBrushColour(inst.vfont, button->label.colour);

	drawString(inst.vfont, button->label.text, x+button->offset.x, y+button->offset.y);
}

static void ui_draw_panel (ui_panel_t *panel, const int32_t x, const int32_t y)
{
	drawRectangleFilled(x+1, y+1, x+panel->rect.width-2, y+panel->rect.height-2, COLOUR_PAL_LIGHTERGREY);
	drawRectangle(x, y, x+panel->rect.width-1, y+panel->rect.height-1, COLOUR_PAL_ORANGE);

	setGlyphScale(inst.vfont, 0.8f);
	setBrushSize(inst.vfont, 1.0f);
	setBrushQuality(inst.vfont, 1);
	setBrushColour(inst.vfont, COLOUR_PAL_BLACK);

	drawString(inst.vfont, trackRecord.date, x+2, y+(panel->rect.height/2)+25);
}

static void ui_render (ui_widget_t **widgets, const uint8_t total, const int32_t x, const int32_t y, const uint32_t flags)
{
	for (int i = 0; i < total; i++){
		ui_widget_t *widget = widgets[i];

		if (widget->isEnabled){
			int32_t childX = x;
			int32_t childY = y;
			
			if (widget->type == UI_WIDGET_PANEL){
				ui_panel_t *obj = (ui_panel_t*)widget;
				childX += obj->rect.x;
				childY += obj->rect.y;
				ui_draw_panel(obj, childX, childY);
				
			}else if (widget->type == UI_WIDGET_BUTTON){
				ui_button_t *obj = (ui_button_t*)widget;
				childX += obj->rect.x;
				childY += obj->rect.y;
				ui_draw_button(obj, childX, childY);
			}

			if (widget->children.total){
				ui_render(widget->children.widgets, widget->children.total, childX, childY, flags);
			}
		}
	}
}

static int ui_input (ui_widget_t **widgets, const uint8_t total, const int32_t x, const int32_t y, const uint32_t flags)
{
	int ret = 0;
	
	for (int i = 0; i < total; i++){
		ui_widget_t *widget = widgets[i];
		if (widget->isEnabled){
			int32_t childX = x;
			int32_t childY = y;
			
			int32_t localX = 0;
			int32_t localY = 0;
			
			if (widget->type == UI_WIDGET_PANEL){
				ui_panel_t *obj = (ui_panel_t*)widget;
				localX = obj->rect.x;
				localY = obj->rect.y;
				
				if (childX >= obj->rect.x && childY >= obj->rect.y){
					if (childX < obj->rect.x+obj->rect.width && childY < obj->rect.y+obj->rect.height){
						ret += obj->callback.func(widget, widget->id, flags);
					}
				}
				
			}else if (widget->type == UI_WIDGET_BUTTON){
				ui_button_t *obj = (ui_button_t*)widget;
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

	
	if (!child_id){
		return obj->isEnabled;
		
	}else if (obj == NULL && child_id){
		for (int i = 0; i < UI_WIDGETOBJS_TOTAL; i++){
			obj = widgetObjs[i];
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
	
	if (!child_id){
		obj->isEnabled = UI_WIDGET_ENABLED;
		return 1;
		
	}else if (obj == NULL && child_id){
		for (int i = 0; i < UI_WIDGETOBJS_TOTAL; i++){
			obj = widgetObjs[i];
			if (obj->id == child_id){
				obj->isEnabled = UI_WIDGET_ENABLED;
				return 1;
			}else{
				for (int j = 0; j < obj->children.total; j++){
					ui_widget_t *widget = CHILD_WIDGET_OBJ(obj,j);
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

	if (!child_id){
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

	case OP_FUNC_HOTSTART:
		gps_hotStart();
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
		render_signalUpdate();
		return 1;
		
	case UI_ID_BUTTON_STOP:
		ui_enable(NULL, UI_ID_BUTTON_START);
		ui_disable(NULL, UI_ID_BUTTON_PAUSE);
		ui_disable(NULL, UI_ID_BUTTON_STOP);
		op_push(OP_FUNC_LOG_STOP);
		op_go();
		render_signalUpdate();
		return 1;
		
	case UI_ID_BUTTON_PAUSE: 
		ui_enable(NULL, UI_ID_BUTTON_START);
		ui_disable(NULL, UI_ID_BUTTON_PAUSE);
		ui_enable(NULL, UI_ID_BUTTON_STOP);
		op_push(OP_FUNC_LOG_PAUSE);
		op_go();
		render_signalUpdate();
		return 1;

	case UI_ID_BUTTON_RESET:
		ui_enable(NULL, UI_ID_BUTTON_START);
		ui_disable(NULL, UI_ID_BUTTON_PAUSE);
		ui_disable(NULL, UI_ID_BUTTON_STOP);
		ui_disable(NULL, UI_ID_BUTTON_RESET);

		op_push(OP_FUNC_LOG_RESET);
		op_go();
		render_signalUpdate();
		return 1;
		
	case UI_ID_BUTTON_HOTSTART:
		op_push(OP_FUNC_HOTSTART);
		op_go();
		render_signalUpdate();
		return 1;

	case UI_ID_BUTTON_REBOOT:
		op_push(OP_FUNC_REBOOT);
		op_go();
		render_signalUpdate();
		return 1;
	
	case UI_ID_BUTTON_CONFIG:	// toggle mapCtrl panel
		if (ui_isEnabled(NULL, UI_ID_PANEL_MAPCTRL)){
			ui_disable(NULL, UI_ID_PANEL_MAPCTRL);
			//inst.renderFlags = 1;
		}else{
			ui_enable(NULL, UI_ID_PANEL_MAPCTRL);
			//inst.renderFlags = 5;
		}
		//op_push(OP_FUNC_CONFIG);
		//op_go();
		render_signalUpdate();
		return 1;
	}
	return 0;
}

int uiPanelMapCtrl_cb (ui_widget_t *opaque, const uint8_t id, const uint32_t flags)
{
	//printf(CS("uiPanelMapCtrl_cb: id:%i"), id);

	/*ui_panel_t *panel = (ui_panel_t*)opaque;

	switch (id){
		
	}*/
	return 0;
}

FLASHMEM void ui_panelBuild ()
{

	ui_panel_t *panel = &panel_mapCtrl;
	memset(panel, 0, sizeof(*panel));
	
	panel->widget.type = UI_WIDGET_PANEL;
	panel->widget.id = UI_ID_PANEL_MAPCTRL;
	panel->widget.children.total = 6;
	panel->widget.children.widgets = (ui_widget_t**)panelButtons;
	panel->widget.parent = NULL;
	panel->rect.x = 30;
	panel->rect.y = 100;
	panel->rect.width = 230;
	panel->rect.height = 300;
	panel->callback.func = uiPanelMapCtrl_cb;
	ui_disable(panel, 0);

	// Start log recording
	ui_button_t *button = CHILD_WIDGET_BUTTON(panel,0);
	button->widget.type = UI_WIDGET_BUTTON;
	button->widget.id = UI_ID_BUTTON_START;
	button->widget.parent = WIDGET(panel);
	button->label.text = "Start";
	button->label.colour = COLOUR_PAL_CREAM;
	button->label.scale = 13;
	button->label.size = 4.0f;
	button->label.quality = 2;
	button->rect.x = 10;
	button->rect.y = 10;
	button->rect.width = 120;
	button->rect.height = 40;
	button->offset.x = 2;
	button->offset.y = 20;
	button->callback.func = uiButtons_cb;
	ui_enable(button, 0);
	
	// Pause recording
	button = CHILD_WIDGET_BUTTON(panel,2);
	button->widget.type = UI_WIDGET_BUTTON;
	button->widget.id = UI_ID_BUTTON_PAUSE;
	button->widget.parent = WIDGET(panel);
	button->label.text = "Pause";
	button->label.colour = COLOUR_PAL_CREAM;
	button->label.scale = 13;
	button->label.size = 4.0f;
	button->label.quality = 2;
	button->rect.x = 10;
	button->rect.y = 10;
	button->rect.width = 130;
	button->rect.height = 40;
	button->offset.x = 2;
	button->offset.y = 20;
	button->callback.func = uiButtons_cb;
	ui_disable(button, 0);
	
	// Stop recording
	button = CHILD_WIDGET_BUTTON(panel,1);
	button->widget.type = UI_WIDGET_BUTTON;
	button->widget.id = UI_ID_BUTTON_STOP;
	button->widget.parent = WIDGET(panel);
	button->label.text = "Stop";
	button->label.colour = COLOUR_PAL_CREAM;
	button->label.scale = 13;
	button->label.size = 4.0f;
	button->label.quality = 2;
	button->rect.x = 10;
	button->rect.y = 60;
	button->rect.width = 100;
	button->rect.height = 40;
	button->offset.x = 2;
	button->offset.y = 20;
	button->callback.func = uiButtons_cb;
	ui_disable(button, 0);

	// Reset recording
	button = CHILD_WIDGET_BUTTON(panel,3);
	button->widget.type = UI_WIDGET_BUTTON;
	button->widget.id = UI_ID_BUTTON_RESET;
	button->widget.parent = WIDGET(panel);
	button->label.text = "Reset";
	button->label.colour = COLOUR_PAL_CREAM;
	button->label.scale = 13;
	button->label.size = 4.0f;
	button->label.quality = 2;
	button->rect.x = 10;
	button->rect.y = 110;
	button->rect.width = 130;
	button->rect.height = 40;
	button->offset.x = 2;
	button->offset.y = 20;
	button->callback.func = uiButtons_cb;
	ui_disable(button, 0);


	// Hotstart receiver
	button = CHILD_WIDGET_BUTTON(panel,4);
	button->widget.type = UI_WIDGET_BUTTON;
	button->widget.id = UI_ID_BUTTON_HOTSTART;
	button->widget.parent = WIDGET(panel);
	button->label.text = "Hotstart";
	button->label.colour = COLOUR_PAL_CREAM;
	button->label.scale = 15;
	button->label.size = 4.0f;
	button->label.quality = 2;
	button->rect.x = 10;
	button->rect.y = 200;
	button->rect.width = 190;
	button->rect.height = 40;
	button->offset.x = 2;
	button->offset.y = 20;
	button->callback.func = uiButtons_cb;
	ui_enable(button, 0);


	// Reboot MPU
	button = CHILD_WIDGET_BUTTON(panel,5);
	button->widget.type = UI_WIDGET_BUTTON;
	button->widget.id = UI_ID_BUTTON_REBOOT;
	button->widget.parent = WIDGET(panel);
	button->label.text = "Reboot";
	button->label.colour = COLOUR_PAL_CREAM;
	button->label.scale = 15;
	button->label.size = 4.0f;
	button->label.quality = 2;
	button->rect.x = 10;
	button->rect.y = 250;
	button->rect.width = 165;
	button->rect.height = 40;
	button->offset.x = 2;
	button->offset.y = 20;
	
	button->callback.func = uiButtons_cb;
	ui_enable(button, 0);
}

void ui_draw (const uint32_t unused1, const uint32_t unused2)
{
	ui_render(widgetObjs, UI_WIDGETOBJS_TOTAL, 0, 0, 0);
}

int ui_input (const int32_t x, const int32_t y, const uint32_t flags)
{
	return ui_input(widgetObjs, UI_WIDGETOBJS_TOTAL, x, y, flags);
}

/*
#######################################################################################################
#######################################################################################################
#######################################################################################################
#######################################################################################################
*/

FLASHMEM void uiBuild ()
{
	ui_panelBuild();
	
	
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


