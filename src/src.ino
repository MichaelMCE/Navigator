
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU LIBRARY GENERAL PUBLIC LICENSE
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU LIBRARY GENERAL PUBLIC LICENSE for more details.
//
//	You should have received a copy of the GNU Library General Public
//	License along with this library; if not, write to the Free
//	Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.




#include "commonGlue.h"
#if USE_STARTUP_IMAGE
#include "startup_352x320_16.h"
#endif




volatile static int32_t recordSignal = 0;
volatile static int32_t renderSignal = 0xFF;
volatile static int32_t receiverUpdateSignal = 0;
volatile static int32_t appendSignal = 0;
volatile static int32_t tilesLoadSig = 0;
volatile static int serialConnected = 0;


#if (VWIDTH > 480)
DMAMEM uint8_t renderBuffer[VWIDTH * VHEIGHT];
#else
uint8_t renderBuffer[VWIDTH * VHEIGHT];
#endif

uint16_t colourTable[PALETTE_TOTAL];

#if (ENABLE_ENCODERS)
static encodersrd_t encoders;
#endif

#if (ENABLE_TOUCH_FT5216)
extern touchCtx_t touchCtx;
#endif

static IntervalTimer onceSecondTimer;
static IntervalTimer tilesLoadTimer;
static vfont_t vfontContext;
static debugOverlay_t debugStrings;
static gpsdata_t gpsData;

trackRecord_t trackRecord;
extern application_t inst;
extern int gnssReceiver_PassthroughEnabled;



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
	//printf(CS("ui_draw_button: %i %i, %i %i"), (int)x, (int)y, (int)button->offset.x, (int)button->offset.y);
	
	//drawRectangle(x, y, x+button->rect.width-1, y+button->rect.height-1, COLOUR_PAL_BLACK);

	setBrush(inst.vfont, BRUSH_DISK);
	setGlyphScale(inst.vfont, button->label.scale/10.0f);
	setBrushSize(inst.vfont, button->label.size);
	setBrushQuality(inst.vfont, button->label.quality);
	setBrushColour(inst.vfont, button->label.colour);

	drawString(inst.vfont, button->label.text, x+button->offset.x, y+button->offset.y);
	
	//printf(CS("%s"), button->label.text);
}

static void ui_draw_panel (ui_panel_t *panel, const int32_t x, const int32_t y)
{
	//printf(CS("ui_draw_panel: %i %i"), (int)x, (int)y);
	
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
	//printf(CS("ui_render: %p %i, %i %i"), widgets, (int)total, (int)x, (int)y);
	
	for (int i = 0; i < total; i++){
		ui_widget_t *widget = widgets[i];
		//printf(CS("ui_render: obj: %i %i, %i, %i %i"), (int)widget->type, (int)widget->id, (int)widget->isEnabled, (int)x, (int)y);

		if (widget->isEnabled){
			int32_t childX = x;
			int32_t childY = y;
			
			if (widget->type == UI_WIDGET_PANEL){
				//printf(CS("ui_render: panel %i"), widget->id);
				
				ui_panel_t *obj = (ui_panel_t*)widget;
				childX += obj->rect.x;
				childY += obj->rect.y;
				
				//printf(CS("ui_render: panel draw: %i, %i %i"), obj->widget.id, (int)childX, (int)childY);
				ui_draw_panel(obj, childX, childY);
				
			}else if (widget->type == UI_WIDGET_BUTTON){
				//printf(CS("ui_render: button %i"), widget->id);
				
				ui_button_t *obj = (ui_button_t*)widget;
				childX += obj->rect.x;
				childY += obj->rect.y;
				
				//printf(CS("ui_render: button draw: %i, %i %i"), obj->widget.id, (int)childX, (int)childY);
				ui_draw_button(obj, childX, childY);
			}

			if (widget->children.total){
				//printf(CS("ui_render: obj child: %i, %p %i, %i %i"), widget->id, widget->children.widgets, (int)widget->children.total, (int)childX, (int)childY);
				ui_render(widget->children.widgets, widget->children.total, childX, childY, flags);
			}
		}
	}
}

static void uiDraw ()
{
	//printf(CS("\n"));
	ui_render(widgetObjs, UI_WIDGETOBJS_TOTAL, 0, 0, 0);
}

static int ui_input (ui_widget_t **widgets, const uint8_t total, const int32_t x, const int32_t y, const uint32_t flags)
{
	//printf(CS("ui_input: %i %i, %i"), (int)x, (int)y, (int)flags);
	
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
				
				//printf(CS("input button: %i %i %i %i"), childX, childY, childX+obj->rect.width, childY+obj->rect.height);
				
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

int uiInput (const int32_t x, const int32_t y, const uint32_t flags)
{
	printf(CS("\n"));
	int ret = ui_input(widgetObjs, UI_WIDGETOBJS_TOTAL, x, y, flags);
	if (ret)
		render_signalUpdate();
	
	return ret;
}

FLASHMEM int ui_isEnabled (void *opaque, const uint8_t child_id)
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

FLASHMEM int ui_enable (void *opaque, const uint8_t child_id)
{
	ui_widget_t *obj = WIDGET(opaque);

	printf(CS("ui_enable: %i"), child_id);
	
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

FLASHMEM int ui_disable (void *opaque, const uint8_t child_id)
{
	ui_widget_t *obj = WIDGET(opaque);

	printf(CS("ui_disable: %i"), child_id);

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

static inline uint32_t op_pop ()
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

static inline uint32_t op_state ()
{
	return opState;
}

FLASHMEM static inline void op_execute (const uint32_t op_code)
{
	switch (op_code){
	case 0:
		op_halt();
		return;
		
	case OP_FUNC_LOG_START:
		log_start();
		return;
		
	case OP_FUNC_LOG_STOP:
		log_stop();
		return;
		
	case OP_FUNC_LOG_PAUSE:
		if (log_isActive())
			log_pause();
		return;
		
	case OP_FUNC_LOG_RESET:{
		//int isActive = log_isActive();
		log_reset();
		log_stop();
		/*if (!isActive)
			log_stop();
		else
			log_start();*/
		return;
	}
	case OP_FUNC_HOTSTART:
		gps_hotStart();
		return;

	case OP_FUNC_REBOOT:
		mpu_reboot();
		return;
		
	case OP_FUNC_CONFIG:
		return;
	};
}

FLASHMEM int uiButtons_cb (ui_widget_t *widget, const uint8_t id, const uint32_t flags)
{
	printf(CS("uiButtons_cb: id:%i"), id);
	
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

FLASHMEM int uiPanelMapCtrl_cb (ui_widget_t *opaque, const uint8_t id, const uint32_t flags)
{
	printf(CS("uiPanelMapCtrl_cb: id:%i"), id);

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

FLASHMEM void uiBuild ()
{
	ui_panelBuild();
	
	
	// mapCtrl/config toggle button
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

static inline double calcDistM (double lat1, double lon1, double lat2, double lon2)
{
	const double R = 6378137.0;		// Earths radius
	const double pi80 = M_PI / 180.0;
	
	lat1 *= pi80;
	lon1 *= pi80;
	lat2 *= pi80;
	lon2 *= pi80;
	double dlat = fabs(lat2 - lat1);
	double dlon = fabs(lon2 - lon1);
	double a = sin(dlat / 2.0) * sin(dlat / 2.0) + cos(lat1) * cos(lat2) * sin(dlon /2.0) * sin(dlon / 2.0);
	double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
	double d = R * c;

	return d;
}

static inline double calcDistMetersTrkPt (const trackPoint_t *pt1, const trackPoint_t *pt2)
{
	const double lat1 = pt1->location.latitude;
	const double lon1 = pt1->location.longitude;
	const double lat2 = pt2->location.latitude;
	const double lon2 = pt2->location.longitude;

	return calcDistM(lat1, lon1, lat2, lon2);
}

static inline double calcDistKmTrkPt (const trackPoint_t *pt1, const trackPoint_t *pt2)
{
	return calcDistMetersTrkPt(pt1, pt2) / 1000.0;
}

FLASHMEM void mpu_reboot ()
{
    USB1_USBCMD = 0;	// disconnect USB Serial port
    delay(100);			// enough time for USB hubs/ports to detect a disconnect
    SCB_AIRCR = 0x05FA0004;
}

FLASHMEM void addDebugLine (const uint8_t *str)
{
	strncpy((char*)debugStrings.line[debugStrings.totalAdded], (char*)str, DEBUG_LINE_LEN);
	debugStrings.line[debugStrings.totalAdded][DEBUG_LINE_LEN-1] = 0;
	debugStrings.totalAdded++;
	debugStrings.timeAdded = millis();
	debugStrings.ready = 8;
	
	if (debugStrings.totalAdded >= DEBUG_LINES){
		debugStrings.totalAdded = DEBUG_LINES-1;

		for (int i = 0; i < DEBUG_LINES-1; i++)
			memcpy(debugStrings.line[i], debugStrings.line[i+1], DEBUG_LINE_LEN);

		//strncpy((char*)debugStrings.line[DEBUG_LINES-1], (char*)str, DEBUG_LINE_LEN-1);
		//debugStrings.line[DEBUG_LINES-1][DEBUG_LINE_LEN-1] = 0;
	}
	
	// send to console if connected
	cmdSendResponse((char*)str);
}

FLASHMEM void drawDebugStrings (debugOverlay_t *debugLines)
{
	setBrushColour(inst.vfont, COLOUR_PAL_BLACK);
	setGlyphScale(inst.vfont, 0.8);
	setBrushSize(inst.vfont, 1.0);
	
	int y = VHEIGHT - 30;

	for (int i = debugLines->totalAdded-1; i >= 0; i--){
		char *str = (char*)debugLines->line[i];
		drawString(inst.vfont, str, 5, y);
		
		y -= 30;
	}
}

static void drawSatSignalAvailabilitySvId (sat_stats_t *sats, const uint8_t gnssId, const int row, const uint8_t colour)
{

#if (VHEIGHT > 320)
	int satsPerRow = 18;	// svId's per row
	int boxHeight = 16;	
	int boxWidth = 24;
			
	int xStart = boxWidth - 2;
	int hSpace = 4;
	int vSpace = 8;
#else
	int satsPerRow = 18;
	int boxHeight = 12;
	int boxWidth = 16;
	
	int xStart = -20;
	int hSpace = 2;
	int vSpace = 4;
#endif

	int x = 0;
	int y = 0;

	for (int i = 0; i < sats->numSvs; i++){
		if (sats->sv[i].gnssId == gnssId){
			if (sats->sv[i].svId <= satsPerRow){
				x = xStart + (sats->sv[i].svId * boxWidth);
				y = row;
			}else{
				x = xStart + ((sats->sv[i].svId-satsPerRow) * boxWidth);
				y = row + boxHeight + vSpace;
			}
				
			if (sats->sv[i].cno)
				drawRectangleFilled(x, y, x+boxWidth-hSpace, y+boxHeight, colour);
			else
				drawRectangle(x, y, x+boxWidth-hSpace, y+boxHeight, colour);
		}
	}
}

void drawSatSignalAvailability (gpsdata_t *data, sat_stats_t *sats)
{
	if (!inst.rstats.rflags.satAvailability)
		return;
	
#if (VHEIGHT > 320)
	int rowGPS = 190;
	int rowGLO = 240;
	int rowGAL = 290;
	int rowBEI = 340;
#else
	int rowGPS = 160;
	int rowGLO = 195;
	int rowGAL = 230;
	int rowBEI = 265;
#endif


	drawSatSignalAvailabilitySvId(sats, GNSSID_GPS, rowGPS, COLOUR_PAL_DARKGREEN);
	drawSatSignalAvailabilitySvId(sats, GNSSID_GLONASS, rowGLO, COLOUR_PAL_RED);
	drawSatSignalAvailabilitySvId(sats, GNSSID_GALILEO, rowGAL, COLOUR_PAL_BLUE);
	drawSatSignalAvailabilitySvId(sats, GNSSID_BEIDOU, rowBEI, COLOUR_PAL_GOLD);
}

void drawSatSignalLevels (gpsdata_t *data, sat_stats_t *sats)
{
	if (!inst.rstats.rflags.satlevels)
		return;
	
	#define SDISPLAY_MAX 30

#if (VHEIGHT > 320)
	float barScale = 2.0f;
#else
	float barScale = 1.5f;
#endif
	
	int y = VHEIGHT-3;
	int x = 2;
	int barPitch = ((VWIDTH - SDISPLAY_MAX) / SDISPLAY_MAX) - (x*2);
		
	for (int i = 0; i < sats->numSvs; i++){
		if (!sats->sv[i].cno) continue;

		uint8_t colour = COLOUR_PAL_CYAN;
		if (sats->sv[i].gnssId == GNSSID_GPS)
			colour = COLOUR_PAL_DARKGREEN;
		else if (sats->sv[i].gnssId == GNSSID_GLONASS)
			colour = COLOUR_PAL_RED;
		else if (sats->sv[i].gnssId == GNSSID_GALILEO)
			colour = COLOUR_PAL_BLUE;
		else if (sats->sv[i].gnssId == GNSSID_BEIDOU)
			colour = COLOUR_PAL_GOLD;
		else if (sats->sv[i].gnssId == GNSSID_QZSS)
			colour = COLOUR_PAL_CHERRYBLOSSOM;
	
		drawRectangleFilled(x, y-((float)sats->sv[i].cno*barScale), x+barPitch-1, y, colour);
		x += barPitch + 2;
	}
}

void drawLocationHeading (const float cx, const float cy, float radius, gpsdata_t *data, sat_stats_t *sats)
{
	const float heading = data->misc.heading;
	const float length = 20.0;
	
	radius += 1.0;
	drawFillArc(cx, cy, radius, 6.0, heading-length, heading+length, COLOUR_PAL_HOMER);
}

void drawSatWorld (gpsdata_t *data, sat_stats_t *sats)
{
	if (!inst.rstats.rflags.satWorld)
		return;
	
	float radius = 105.0f;
	float multiplier = 1.0f;
	
#if (VHEIGHT > 320)
	multiplier = 1.7f;
	radius *= multiplier;
#endif
	
	float cx = (VWIDTH  - radius) - 3.0f;
	float cy = (VHEIGHT - radius) - 3.0f;
	
	for (float r = 2.0f; r <= radius; r += 25.0f)
		drawCircle(cx, cy, r, COLOUR_PAL_REDISH);
		
	for (int i = 0; i < sats->numSvs; i++){
		if (!sats->sv[i].svId) continue;

		uint8_t colour = COLOUR_PAL_MAGENTA;
		if (sats->sv[i].gnssId == GNSSID_QZSS){
			colour = COLOUR_PAL_CHERRYBLOSSOM;
		}else if (sats->sv[i].gnssId == GNSSID_SBAS){
			colour = COLOUR_PAL_CYAN;
		}else if (sats->sv[i].flags&SAT_FLAGS_SVUSED){
			if (sats->sv[i].gnssId == GNSSID_GPS)
				colour = COLOUR_PAL_DARKGREEN;
			else if (sats->sv[i].gnssId == GNSSID_GLONASS)
				colour = COLOUR_PAL_RED;
			else if (sats->sv[i].gnssId == GNSSID_GALILEO)
				colour = COLOUR_PAL_BLUE;
			else if (sats->sv[i].gnssId == GNSSID_BEIDOU)
				colour = COLOUR_PAL_GOLD;
		}

		if (sats->sv[i].elev >= 0){
			float elv = (90.0f - (float)sats->sv[i].elev) * multiplier;
			float az = (float)sats->sv[i].azim - 90.0f;
		
			int x = cx + (elv * cosDegrees(az));
			int y = cy + (elv * sinDegrees(az));
			drawCircleFilled(x, y, 5.0f * multiplier, colour);
		}
	}
	
	drawLocationHeading(cx, cy, radius, data, sats);
}

void getDateTime (dategps_t *date, timegps_t *time)
{
	gpsdata_t *data = &gpsData;
	
	date_adjustTime4BST(data);
	
	time->hour = data->time.hour;
	time->min = data->time.min;
	time->sec = data->time.sec;
	
	date->day = data->date.day;
	date->month = data->date.month;
	date->year = data->date.year;
	
	//printf(CS(" %i %i %i"), date->day, date->month, date->year);
	//printf(CS(" %i %i %i"), time->hour, time->min, time->sec);
}

static inline void drawStrings (gpsdata_t *data, sat_stats_t *sats)
{
	char tbuffer[64];


	setBrush(inst.vfont, BRUSH_DISK);
	setGlyphScale(inst.vfont, 0.8);
	setBrushSize(inst.vfont, 2.0);
	setBrushQuality(inst.vfont, 2);
	setBrushColour(inst.vfont, COLOUR_PAL_BLACK);
	
	date_adjustTime4BST(data);
	snprintf(tbuffer, sizeof(tbuffer), "Time: %.02i:%.02i:%.02i", data->time.hour, data->time.min, data->time.sec);
	drawString(inst.vfont, tbuffer, 5, 20);
	
	snprintf(tbuffer, sizeof(tbuffer), "Date: %.02i.%.02i.%i", data->date.day, data->date.month, data->date.year-2000);
	drawString(inst.vfont, tbuffer, 5, 45);

	snprintf(tbuffer, sizeof(tbuffer), "Sats: %i/%i", data->fix.sats, sats->numSvs);
	drawString(inst.vfont, tbuffer, VWIDTH-156, 20);
	
	snprintf(tbuffer, sizeof(tbuffer), "Fix: %s", getFixName(data->fix.type));
	drawString(inst.vfont, tbuffer, VWIDTH-156, 45);

	if (data->fix.type){
		snprintf(tbuffer, sizeof(tbuffer), "3D: %.2f", data->fix.pAcc/100.0f);
		drawString(inst.vfont, tbuffer, VWIDTH-156, 70);
		
		snprintf(tbuffer, sizeof(tbuffer), "2D: %.2f", data->fix.hAcc/100.0f);
		drawString(inst.vfont, tbuffer, VWIDTH-156, 95);
	}else{
		drawString(inst.vfont, "3D: 0.0", VWIDTH-156, 70);
		drawString(inst.vfont, "2D: 0.0", VWIDTH-156, 95);
	}
	
	snprintf(tbuffer, sizeof(tbuffer), "Longitude:%.8f", data->navAvg.longitude);
	drawString(inst.vfont, tbuffer, 5, 70);

	snprintf(tbuffer, sizeof(tbuffer), "Latitude:  %.8f", data->navAvg.latitude); 
	drawString(inst.vfont, tbuffer, 5, 95);
		
	snprintf(tbuffer, sizeof(tbuffer), "Altitude: %.1f", data->navAvg.altitude);
	drawString(inst.vfont, tbuffer, 5, 120);

	snprintf(tbuffer, sizeof(tbuffer), "H: %.2f, V: %.2f", data->dop.horizontal/100.0f, data->dop.vertical/100.0f);
	drawString(inst.vfont, tbuffer, 5, 145);
	
#if (VHEIGHT > 320)
	snprintf(tbuffer, sizeof(tbuffer), "P: %.2f, G: %.2f", data->dop.position/100.0f, data->dop.geometric/100.0f);
	drawString(inst.vfont, tbuffer, 5, 170);	
#endif

	if (inst.renderFlags == 4) return;

	if (inst.runLog.enabled){
		snprintf(tbuffer, sizeof(tbuffer), "%i", (int)inst.runLog.idx);
		drawString(inst.vfont, tbuffer, VWIDTH-400, VHEIGHT-15);
	}else{
		snprintf(tbuffer, sizeof(tbuffer), "%.0fm", inst.distance);
		drawString(inst.vfont, tbuffer, VWIDTH-480, VHEIGHT-15);
	}

	snprintf(tbuffer, sizeof(tbuffer), "%i", (int)trackRecord.marker-(int)trackRecord.lastFrom);
	drawString(inst.vfont, tbuffer, VWIDTH-300, VHEIGHT-15);

	snprintf(tbuffer, sizeof(tbuffer), "%i", (int)trackRecord.marker);
	drawString(inst.vfont, tbuffer, VWIDTH-200, VHEIGHT-15);

	if (data->misc.distance >= 2000)
		snprintf(tbuffer, sizeof(tbuffer), "%.2fKm", data->misc.distance/1000.0f);
	else
		snprintf(tbuffer, sizeof(tbuffer), "%um", (unsigned int)data->misc.distance);	
	drawString(inst.vfont, tbuffer, VWIDTH-108, VHEIGHT-15);
}

void drawSpeed (gpsdata_t *data)
{
	char tbuffer[8];
	int x = (VWIDTH/2);
	int y = 0;


#if (VHEIGHT <= 320)
	x += 10;
	setGlyphScale(inst.vfont, 1.9);
#else
	setGlyphScale(inst.vfont, 2.0);
#endif

	setBrushColour(inst.vfont, COLOUR_PAL_MAROON);
	setBrushSize(inst.vfont, 4.0);
		
	//if (data->misc.speed > 1.0)
	snprintf(tbuffer, sizeof(tbuffer), "%.1f", data->misc.speed);
	

	box_t box = {0};
	getStringMetrics(inst.vfont, tbuffer, &box);
	x -= box.x2 / 2;
	y += box.y2 + abs(box.y1) - 10;
	drawString(inst.vfont, tbuffer, x, y);
}

static inline void drawLogStatus (int x, int y, int boxDepth)
{
	if (!trackRecord.acquDisabled)
		drawRectangleFilled(x+1, y+1, x+boxDepth-1, y+boxDepth-1, COLOUR_PAL_DARKGREEN);
	drawRectangle(x, y, x+boxDepth, y+boxDepth, COLOUR_PAL_DARKGREY);

	x += 36;
	if (!trackRecord.writeDisabled)
		drawRectangleFilled(x+1, y+1, x+boxDepth-1, y+boxDepth-1, COLOUR_PAL_DARKGREEN);
	drawRectangle(x, y, x+boxDepth, y+boxDepth, COLOUR_PAL_DARKGREY);
}

void drawPanel (gpsdata_t *data)
{
	sat_stats_t *sats = getSats();
	
	uint32_t t0 = millis();
	drawStrings(data, sats);
	inst.rstats.rtime.strings = millis() - t0;
	
	if (sats->numSvs){
		if (inst.renderFlags == 0)
			drawSatSignalLevels(data, sats);
		if (inst.renderFlags == 0 || inst.renderFlags == 1)
			drawSatSignalAvailability(data, sats);
		if (inst.renderFlags == 0 || inst.renderFlags == 1 || inst.renderFlags == 2)
			drawSatWorld(data, sats);
		if (inst.renderFlags != 4 && !inst.runLog.enabled)
			drawSpeed(data);
	}
	
	if (!inst.rstats.rflags.satlevels || inst.renderFlags != 0){	// dont overwrite sat levels
		//if (serialConnected)
			drawLogStatus(8, VHEIGHT - 28, 20);
	}
}

static inline void frameClear ()
{
	memset(renderBuffer, COLOUR_PAL_CREAM, sizeof(renderBuffer));
}

static inline uint16_t paletteGet16 (const uint8_t idx)
{
	return colourTable[idx];
}

void displayUpdate ()
{
	const uint32_t t0 = millis();

#if USE_STRIP_RENDERER
	
	uint8_t *pixels8 = renderBuffer;
	uint16_t *stripAddress = (uint16_t*)tft_getBuffer();

	for (int y1 = 0; y1 < VHEIGHT; y1 += STRIP_RENDERER_HEIGHT){
		uint16_t *pixels16 = stripAddress;
		
		for (int y = 0; y < STRIP_RENDERER_HEIGHT; y++){
			for (int x = 0; x < VWIDTH; x++){
				*pixels16 = paletteGet16(*pixels8);
				pixels16++;
				pixels8++;
			}
		}
		tft_update_area(0, y1, VWIDTH-1, y1+STRIP_RENDERER_HEIGHT-1);
	}
#else

	uint8_t *pixels8 = renderBuffer;
	uint16_t *pixels16 = (uint16_t*)tft_getBuffer();
	
	for (int y = 0; y < VHEIGHT; y++){
		for (int x = 0; x < VWIDTH; x++){
			*pixels16 = paletteGet16(*pixels8);
			pixels16++;
			pixels8++;
		}
	}

	tft_update();
#endif
	const uint32_t t1 = millis();
	inst.rstats.rtime.display = (t1 - t0);
}

FLASHMEM static void setStartupImage ()
{
#if USE_STARTUP_IMAGE
	const int img_w = 352;
	const int img_h = 320;
	
	int x1 = (TFT_WIDTH - img_w) / 2;
	if (x1 < 0) x1 = 0;
	int y1 = (TFT_HEIGHT - img_h) / 2;
	if (y1 < 0) y1 = 0;

	int x2 = x1 + img_w-1;
	if (x2 > TFT_WIDTH-1) x2 = TFT_WIDTH-1;
	int y2 = y1 + img_h-1;
	if (y2 > TFT_HEIGHT-1) y2 = TFT_HEIGHT-1;

	tft_update_array((uint16_t*)frame352x320, x1, y1, x2, y2);
#endif
}

FLASHMEM static void init_display ()
{
	tft_init();
	palette_init();
	frameClear();
	displayUpdate();
	tft_setBacklight(TFT_INTENSITY);
	displayUpdate();
	setStartupImage();
}

static inline void appendTrackPoint (trackRecord_t *trackRecord, const gpsdata_t *gpsData)
{
	if (gpsData->fix.type == PVT_FIXTYPE_NOFIX || !trackRecord->firstFix)
		return;
	if (trackRecord->acquDisabled)
		return;

	trackPoint_t *tpt = &trackRecord->trackPoints[trackRecord->marker];
	
	tpt->iTow = gpsData->iTow;
	tpt->location.longitude = gpsData->navAvg.longitude;
	tpt->location.latitude  = gpsData->navAvg.latitude;
	tpt->location.altitude  = gpsData->navAvg.altitude;
	tpt->heading = gpsData->misc.heading * 100.0f;
	tpt->speed = gpsData->misc.speed * 100.0f;
	

	if (++trackRecord->marker >= TRACKPTS_MAX){
		trackRecord->marker = 0;
		trackRecord->lastFrom = 0;
	}

	inst.rstats.trkptsTotal = trackRecord->marker;
	inst.rstats.trkptsToWrite = trackRecord->marker - (int)trackRecord->lastFrom;
}

static inline void recordCreatePathname (trackRecord_t *trackRecord, gpsdata_t *gps)
{
	date_formatDateTime(gps, trackRecord->date, sizeof(trackRecord->date));
	snprintf(trackRecord->filename, sizeof(trackRecord->filename), TRACKPTS_DIR"%s.tpts", trackRecord->date);
}

void msgPostMed (const gpsdata_t *const opaque, const intptr_t unused)
{
	gpsData = *opaque;

#if 0
	gpsData.navAvg.latitude = MY_LAT;
	gpsData.navAvg.longitude = MY_LON;
	gpsData.navAvg.altitude = MY_ALT;
#endif

	if (!trackRecord.firstFix){
		inst.lastFix.latitude = MY_LAT;
		inst.lastFix.longitude = MY_LON;
		inst.lastFix.altitude = MY_ALT;
	}

	if (!inst.assistNowAutoLoad)
		inst.assistNowAutoLoad = gpsData.dateConfirmed && gpsData.timeConfirmed;
		
	if (opaque->fix.type == PVT_FIXTYPE_NOFIX)
		return;
	else
		inst.lastFix = gpsData.navAvg;

 	if (trackRecord.acquDisabled)
 		return;

	// begin recording from first fix
	if (!trackRecord.firstFix && trackRecord.recordActive){
		trackRecord.firstFix = 1;
		
		date_adjustTime4BST(&gpsData);
		recordCreatePathname(&trackRecord, &gpsData);
		printf(CS("FirstFix. Filename: %s"), trackRecord.filename);
		log_start();
		
		dategps_t date;
		timegps_t time;
		getDateTime(&date, &time);
		gps_resetOdo();
	}

	appendSignal = 1;
}

void render_loadTiles ()
{
	inst.loadTiles = 200;
}

void render_signalUpdate ()
{
	renderSignal = 0xFF;
}

void ISR_tilesLoad_sig ()
{
	tilesLoadSig = 0xFF;
}

void ISR_onceSecond_sig ()
{
	inst.rstats.nothingCountSecond = inst.rstats.nothingCount;
	inst.rstats.nothingCount = 0;
	
	renderSignal = 0xFF;
	recordSignal++;

	serialConnected = Serial.dtr();
	inst.heartbeatPulse = 1 && serialConnected;

	receiverUpdateSignal = 0xFF;
}

FLASHMEM void init_vfont ()
{
	vfont_t *vfont = &vfontContext;

	vfont_init(vfont);
	setFont(vfont, &futural);
	setBrush(vfont, BRUSH_DISK);
	setBrushStep(vfont, 1.0);
	setBrushSize(vfont, 1.0);
	setAspect(vfont, 1.0, 1.0);
	setBrushColour(vfont, COLOUR_PAL_BLACK);
	setGlyphScale(vfont, 0.8);
}

static void drawMap (const pos_rec_t *loc, const float heading)
{
	uint32_t t0 = micros();
	map_render(&trackRecord, loc, heading, MAP_RENDER_VIEWPORT);
	uint32_t t1 = micros();
	inst.rstats.rtime.map = (t1 - t0)/1000.0f;

	//map_render(&trackRecord, loc, heading, MAP_RENDER_POI);
	inst.rstats.rtime.poi = 0;//(micros() - t1)/1000.0f;

	t1 = micros();
	map_render(&trackRecord, loc, heading, MAP_RENDER_TRACKPOINTS);
	inst.rstats.rtime.trkpts = (micros() - t1)/1000.0f;

	if (inst.renderFlags == 4)
		map_render(&trackRecord, loc, heading, MAP_RENDER_LOCGRAPTHIC | MAP_RENDER_COMPASS | MAP_RENDER_OVERLAY);
	else
		map_render(&trackRecord, loc, heading, MAP_RENDER_LOCGRAPTHIC);
}

void drawScreen (gpsdata_t *data)
{
	if (!inst.runLog.enabled){
		//if (!trackRecord.firstFix)
		//	drawMap(&data->navAvg, data->misc.heading);
		//else
			drawMap(&inst.lastFix, data->misc.heading);

	}else{
		if (trackRecord.marker){
			trackPoint_t *trkPt = &trackRecord.trackPoints[inst.runLog.idx];

			data->navAvg = trkPt->location;
			data->iTow = trkPt->iTow;
			data->misc.speed = trkPt->speed;
			data->misc.heading = trkPt->heading/100.0f;

			drawMap(&trkPt->location, data->misc.heading);
			//printf(CS("%i: %f %f"), (int)tpIdx, trkPt->location.longitude, trkPt->location.latitude);
		
			if (!inst.runLog.pause)
				inst.runLog.idx += inst.runLog.step;
			if (inst.runLog.idx >= trackRecord.marker-1)
				inst.runLog.idx = 0;
		}
	}
	
	if (inst.renderFlags == 5){
		inst.rstats.rtime.strings = 0;
		return;
	}

	if (debugStrings.ready){
		debugStrings.ready--;
		drawDebugStrings(&debugStrings);
	}else{
		drawPanel(&gpsData);
	}
}

FLASHMEM void init_debugStrings ()
{
	memset(&debugStrings, 0, sizeof(debugStrings));
	debugStrings.ready = 3;
}


FLASHMEM void init_record ()
{
	memset(&trackRecord, 0, sizeof(trackRecord));
}

FLASHMEM void init_isrTimers ()
{
	// render update timer. Set to once per second
	onceSecondTimer.begin(ISR_onceSecond_sig, 1*990*1000);		// in microseconds
	onceSecondTimer.priority(180);

	tilesLoadTimer.begin(ISR_tilesLoad_sig, 1*125*1000);		// in microseconds
	tilesLoadTimer.priority(210);

#if ENABLE_TOUCH_FT5216	
	touch_startTimer();
#endif

}

FLASHMEM void setup ()
{
	
#if ENABLE_MTP
	mtp_init();
#endif
		
	Serial.begin(SERIAL_RATE);

	init_display();
	fio_init();

#if ENABLE_MTP
	while (1)
		mtp_task();
	return;
#endif

	cmd_init();
	init_vfont();
	init_debugStrings();
	gps_init();
	init_isrTimers();
#if ENABLE_TOUCH_FT5216
	touch_init();
#endif
	init_record();

	map_init(&vfontContext);
	fpRecord_init(&trackRecord);

#if ENABLE_ENCODERS
	encoders_init();
#endif

	uiBuild();

	if (MPU_CLOCK_FREQ > 60)
		mpu_setClockFreq(MPU_CLOCK_FREQ);

	inst.renderFlags = 1;		// don't draw sat availability. do show log status
	log_stop();
}

#if ENABLE_ENCODERS
void doEncoders (encodersrd_t *encoders)
{
	if (encoders->encoder[2].positionChange != 0){
		float zoomlevel = sceneGetZoom(&inst);
		if (encoders->encoder[2].positionChange > 0)
			zoomlevel += (zoomlevel * 0.1f);
		else
			zoomlevel -= (zoomlevel * 0.1f);

		if (zoomlevel < SCENE_ZOOM_MIN) zoomlevel = SCENE_ZOOM_MIN;
		else if (zoomlevel > SCENE_ZOOM_MAX) zoomlevel = SCENE_ZOOM_MAX;

		sceneSetZoom(&inst, zoomlevel);
		sceneResetViewport(&inst);
		render_loadTiles();
		renderSignal = 1;
	}

	if (encoders->encoder[2].buttonPress){
		sceneSetZoom(&inst, SCENE_ZOOM);
		sceneResetViewport(&inst);
		sceneLoadTiles(&inst);
		renderSignal = 1;
	}
	
	if (encoders->encoder[1].positionChange != 0){
		if (inst.runLog.enabled){
			if (encoders->encoder[1].positionChange > 0)
				log_runAdvance(25);
			else
				log_runAdvance(-25);
			renderSignal = 1;
		}
	}
	
	if (encoders->encoder[1].buttonPress){
		
		if (!inst.runLog.enabled){
			log_runStart();
			log_runPause();
		}else{
			log_runStop();
		}
		renderSignal = 1;
	}

	if (encoders->encoder[0].positionChange != 0){
		uint8_t level = tft_getBacklight();
		
		if (encoders->encoder[0].positionChange > 0){
			level = (level + 5) & 0xFF;
			if (level < 5) level = 0;
			tft_setBacklight(level);
		}else{
			level = (level - 5) & 0xFF;
			if (level < 5) level = 0;
			tft_setBacklight(level);
		}
		//printf(CS("Backlight: %i"), (int)level);
	}

	if (encoders->encoder[0].buttonPress){
		//printf(CS("GPS Passthrough %i"), (!gnssReceiver_PassthroughEnabled)&0x01);
		cmdSendResponse("");
		Serial.flush();

		if (gnssReceiver_PassthroughEnabled == 0)
			gnssReceiver_PassthroughEnabled = 1;
		else
			gnssReceiver_PassthroughEnabled = 0;
		renderSignal = 1;
	}
}
#endif

void printCmdStats (runState_t *stats)
{
	cmdSendResponse("");
	printf(CS("zoom:%.0f, temp:%.1f, nothing:%llu, update:%.1f"), sceneGetZoom(&inst), InternalTemperature.readTemperatureC(), stats->nothingCountSecond, inst.rstats.rtime.display);
	printf(CS("map:%.2f, strings:%i, poi:%.2f, route:%.2f"), stats->rtime.map, stats->rtime.strings, stats->rtime.poi, stats->rtime.trkpts);
	//printf(CS("trkpt total:%i, toWrite:%i, epoch:%i"), stats->trkptsTotal, stats->trkptsToWrite, gpsData.rates.epochPerRead);
	printf(CS("epoch:%i, rx:%i"), gpsData.rates.epochPerRead, receiver_getRx());
	receiver_resetRxTx();
}

FASTRUN void mpu_setClockFreq (const uint32_t freqMhz)
{
	if (freqMhz >= 24 && freqMhz <= 960)
		set_arm_clock(freqMhz*1000*1000);
}

FASTRUN void loop ()
{
#if ENABLE_ENCODERS
	if (encoders_isReady()){
		encoders_read(&encoders);
		doEncoders(&encoders);
	}
#endif

	gps_task();

	if (gnssReceiver_PassthroughEnabled){
		gps_task();
		
#if ENABLE_TOUCH_FT5216
		if (touchCtx.tready){		// touch panel to disengage passthrough
			touch_task(&touchCtx);
			touchCtx.tready = 0;
		}
#endif
		return;
	}

	if (receiverUpdateSignal){
		receiverUpdateSignal = 0;
		gps_requestUpdate();
	}

	if (appendSignal){
		appendSignal = 0;
		if (trackRecord.recordActive /*&& inst.renderFlags != 4*/)
			appendTrackPoint(&trackRecord, &gpsData);
	}

	gps_task();

#if ENABLE_TOUCH_FT5216
	if (touchCtx.tready){
		touch_task(&touchCtx);
		touchCtx.tready = 0;
	}
#endif

	if (renderSignal){
		if (!inst.runLog.enabled || inst.runLog.pause)
			renderSignal = 0;
		frameClear();
		drawScreen(&gpsData);
		uiDraw();
		displayUpdate();
		gps_task();

		if (inst.rstats.rflags.console && serialConnected)
			printCmdStats(&inst.rstats);

		// load auto.ubx once we have a valid date, but not too quickly as not always taken
		// perform once per boot only
		if (inst.assistNowAutoLoad == 1){
			if (recordSignal > 15){
				inst.assistNowAutoLoad = 2;
				gps_loadOfflineAssist(0);
			}
		}

		if (0 && inst.freeTiles){
			inst.freeTiles = 0;
			//tilesUnload(inst.renderPassCt);
		}
	}

#if ENABLE_TOUCH_FT5216
	if (touchCtx.tready){
		touch_task(&touchCtx);
		touchCtx.tready = 0;
	}
#endif

	if (tilesLoadSig){
		tilesLoadSig = 0;
		if (inst.loadTiles){
			inst.loadTiles--;
			if (sceneLoadTiles(&inst))
				gps_task();
		}
	}

	if (trackRecord.recordActive){
		if (recordSignal > 60){
			recordSignal = 0;
			if (inst.renderFlags != 4 && !trackRecord.writeDisabled){		// safe mode. don't write whilst compass is displayed.
				fpRecord_appendLog(&trackRecord);
			}
			gps_task();
		}
	}

	if (inst.cmdTaskRunMode){
		inst.cmdTaskRunMode = cmd_task(inst.heartbeatPulse);
		inst.heartbeatPulse = 0;
	}

	if (op_state() == OP_READY){
		op_execute(op_pop());
	}

	inst.rstats.nothingCount++;
}
