
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
#if TFT_LOWERPANEL
#include "lowerpane_960x60_16.h"
#endif



volatile static int32_t recordSignal = 0;
volatile static int32_t renderSignal = 0xFF;
volatile static int32_t receiverUpdateSignal = 0;
volatile static int32_t appendSignal = 0;
volatile static int32_t tilesLoadSig = 0;
volatile static int serialConnected = 0;

static volatile int zoomMultiplier = 1;
static volatile int spectrumXMarker = 56;


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

extern trackRecord_t trackRecord;
extern application_t inst;
extern int gnssReceiver_PassthroughEnabled;
extern touch_t touchDebug;
extern touchCtx_t touchCtx;



uint32_t log_getLastiTow ()
{
	return gpsData.iTow;
}

pos_rec_t log_getLastPosition ()
{
	return gpsData.navAvg;
}

dategps_t log_getLastDate ()
{
	return gpsData.date;
}

timegps_t log_getLastTime ()
{
	return gpsData.time;
}

void date_getAdjustedTime (gpsdata_t *data, dategps_t *date, timegps_t *time)
{
	if (data == NULL) data = &gpsData;

	date_adjustTime4BST(data);
	
	time->hour = data->time.hour;
	time->min = data->time.min;
	time->sec = data->time.sec;
	
	date->day = data->date.day;
	date->month = data->date.month;
	date->year = data->date.year;
}

static inline uint16_t paletteGet16 (const uint8_t idx)
{
	return colourTable[idx];
}

static inline double trkPt_calcDistMetersPosition (double lat1, double lon1, double lat2, double lon2)
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

static inline double trkPt_calcDistMeters (const trackPoint_t *pt1, const trackPoint_t *pt2)
{
	const double lat1 = pt1->location.latitude;
	const double lon1 = pt1->location.longitude;
	const double lat2 = pt2->location.latitude;
	const double lon2 = pt2->location.longitude;

	return trkPt_calcDistMetersPosition(lat1, lon1, lat2, lon2);
}

static inline double trkPt_calcDistKm (const trackPoint_t *pt1, const trackPoint_t *pt2)
{
	return trkPt_calcDistMeters(pt1, pt2) / 1000.0;
}

FLASHMEM void mpu_reboot ()
{
    USB1_USBCMD = 0;	// disconnect USB Serial port
    delay(100);			// enough time for USB hubs/ports to detect a disconnect
    SCB_AIRCR = 0x05FA0004;
}

void mpu_updateMPUFreqMenu (const uint32_t freqMhz)
{
	for (int i = UI_ID_BUTTON_MPU_FREQ136; i <= UI_ID_BUTTON_MPU_FREQ816; i++)
		ui_setHighlight(i, 0);

	switch (freqMhz){
	case 136:
		ui_setHighlight(UI_ID_BUTTON_MPU_FREQ136, 1);
		return;
	case 272:
		ui_setHighlight(UI_ID_BUTTON_MPU_FREQ272, 1);
		return;
	case 528:
		ui_setHighlight(UI_ID_BUTTON_MPU_FREQ528, 1);
		return;
	case 600:
		ui_setHighlight(UI_ID_BUTTON_MPU_FREQ600, 1);
		return;
	case 720:
		ui_setHighlight(UI_ID_BUTTON_MPU_FREQ720, 1);
		return;
	case 816:
		ui_setHighlight(UI_ID_BUTTON_MPU_FREQ816, 1);
		return;
	}
}

FLASHMEM void mpu_setClockFreq (const uint32_t freqMhz)
{
	if (freqMhz >= 24 && freqMhz <= 960){
		set_arm_clock(freqMhz*1000*1000);
		mpu_updateMPUFreqMenu(freqMhz);
	}
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

static void drawSatSignalAvailability (gpsdata_t *data, sat_stats_t *sats)
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

static void drawSatSignalLevels (gpsdata_t *data, sat_stats_t *sats)
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

static void drawSatWorldHeading (const float cx, const float cy, float radius, gpsdata_t *data, sat_stats_t *sats)
{
	const float heading = data->misc.heading;
	const float length = 20.0;
	
	radius += 1.0;
	drawFillArc(cx, cy, radius, 6.0, heading-length, heading+length, COLOUR_PAL_HOMER);
}

static void drawSatWorld (gpsdata_t *data, sat_stats_t *sats)
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
	
	drawSatWorldHeading(cx, cy, radius, data, sats);
}

static inline void drawMapOverlayStrings (gpsdata_t *data, sat_stats_t *sats)
{
	if (!inst.rstats.rflags.strings)
		return;
		
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

	snprintf(tbuffer, sizeof(tbuffer), "Sats: %i/%i", data->fix.sats, (int)sats->numSvs);
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
	//snprintf(tbuffer, sizeof(tbuffer), "P: %.2f, G: %.2f", data->dop.position/100.0f, data->dop.geometric/100.0f);
#if (RECEIVER_SINGLE)
	snprintf(tbuffer, sizeof(tbuffer), "msSS: %i", (int)((sats->status_msSS[0]/1000.0f)+0.5f));
#else
	snprintf(tbuffer, sizeof(tbuffer), "msSS A: %i, B: %i", (int)((sats->status_msSS[0]/1000.0f)+0.5f), (int)((sats->status_msSS[1]/1000.0f)+0.5f));
#endif
	drawString(inst.vfont, tbuffer, 5, 170);
	
#endif

#if UI_DRAW_TOUCH_POSITION
	snprintf(tbuffer, sizeof(tbuffer), "%i,%i", (int)touchDebug.points[0].x, (int)touchDebug.points[0].y);
	drawString(inst.vfont, tbuffer, 200, VHEIGHT-15);
#endif

	if (log_runStatus()){
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

static void drawMapOverlaySpeed (gpsdata_t *data)
{
	if (!inst.rstats.rflags.strings)
		return;
		
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

	x += 36;
	if (isSerialConsoleConnected())
		drawRectangleFilled(x+1, y+1, x+boxDepth-1, y+boxDepth-1, COLOUR_PAL_DARKGREEN);
	drawRectangle(x, y, x+boxDepth, y+boxDepth, COLOUR_PAL_DARKGREY);

	x += 36;
	if (powersaveIsEnabled())
		drawRectangleFilled(x+1, y+1, x+boxDepth-1, y+boxDepth-1, COLOUR_PAL_DARKGREEN);
	drawRectangle(x, y, x+boxDepth, y+boxDepth, COLOUR_PAL_DARKGREY);
}

static void drawMapOverlay (gpsdata_t *data)
{
	sat_stats_t *sats = getSats();
	
	uint32_t t0 = millis();
	drawMapOverlayStrings(data, sats);
	inst.rstats.rtime.strings = millis() - t0;
	
	if (sats->numSvs){
		if (inst.rstats.rflags.satlevels)
			drawSatSignalLevels(data, sats);
			
		if (inst.rstats.rflags.satAvailability)
			drawSatSignalAvailability(data, sats);
			
		if (inst.rstats.rflags.satWorld)
			drawSatWorld(data, sats);
			
		if (inst.rstats.rflags.strings && !log_runStatus())
			drawMapOverlaySpeed(data);
	}
	
	if (!inst.rstats.rflags.satlevels)
		drawLogStatus(8, VHEIGHT - 28, 20);
}

static inline void frameClear ()
{
	memset(renderBuffer, COLOUR_PAL_CREAM, sizeof(renderBuffer));
}

static inline void frameSend ()
{
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
}

FLASHMEM static inline void frameSendClock ()
{
	memset(renderBuffer, COLOUR_PAL_BLACK, sizeof(renderBuffer));
	
	char buffer[2][8];
	date_adjustTime4BST(&gpsData);
	snprintf(buffer[0], sizeof(buffer[0]), "%.02i:%.02i", gpsData.time.hour, gpsData.time.min);
	snprintf(buffer[1], sizeof(buffer[1]), "%i", gpsData.time.sec);
	
	clock_render(buffer[0], buffer[1]);
	frameSend();	
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

#if (TFT_LOWERPANEL)
	// recenter image
	y1 += 30;
	y2 += 30;
#endif

	tft_update_array((uint16_t*)frame352x320, x1, y1, x2, y2);
#endif
}

static inline void drawPanelBlank ()
{
#if TFT_LOWERPANEL
	const int height = 60;
	const int y1 = 480;
	memset(renderBuffer, COLOUR_PAL_BLACK, sizeof(renderBuffer));
	tft_update_array((uint16_t*)renderBuffer, 0, y1, VWIDTH-1, y1+height-1);
#endif
}

void drawPanel (const uint8_t which)
{
#if (TFT_LOWERPANEL)
	static uint8_t currentPanel = 255;
	
	if (currentPanel == which)
		return;

	switch (which){
	case 0: tft_update_array((uint16_t*)blank960x60, 0, 480, 960-1, 540-1); return;
	case 1: tft_update_array((uint16_t*)panel960x60_map, 0, 480, 960-1, 540-1); return;
	case 2: tft_update_array((uint16_t*)panel960x60_log_importfaded, 0, 480, 960-1, 540-1); return;
	case 3: tft_update_array((uint16_t*)panel960x60_log_import, 0, 480, 960-1, 540-1); return;
	case 4: drawPanelBlank(); return;
	};
#endif
}

uint16_t *drawPanel_getPixels (const uint8_t which)
{
#if (TFT_LOWERPANEL)
	switch (which){
	case 0: return (uint16_t*)blank960x60;
	case 1: return (uint16_t*)panel960x60_map;
	case 2: return (uint16_t*)panel960x60_log_importfaded;
	case 3: return (uint16_t*)panel960x60_log_import;
	case 4: return (uint16_t*)blank960x60;	// should throw an error should one attempt to write here. Don't write here.
	};
#endif
	return NULL;
}

FLASHMEM static void init_display ()
{
	tft_init();
	palette_init();
	frameClear();
	frameSend();
	tft_setBacklight(TFT_INTENSITY);
	frameSend();
	setStartupImage();
	drawPanel(0);
}

static inline void trkPt_trackRecordAppend (trackRecord_t *trackRecord, const gpsdata_t *gpsData)
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

static void trkPt_trackRecordCreatePathname (trackRecord_t *trackRecord, gpsdata_t *gps)
{
	date_formatDateTime(gps, trackRecord->date, sizeof(trackRecord->date));
	snprintf(trackRecord->filename, sizeof(trackRecord->filename), TRACKPTS_DIR"%s.tpts", trackRecord->date);
}

void receiver_cb (const gpsdata_t *const opaque, const intptr_t unused)
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
	if (trackRecord.recordActive){
		if (!trackRecord.firstFix){
			trackRecord.firstFix = 1;
		
			date_adjustTime4BST(&gpsData);
			trkPt_trackRecordCreatePathname(&trackRecord, &gpsData);
			//printf(CS("FirstFix. Filename: %s"), trackRecord.filename);
			log_start();
		
			dategps_t date;
			timegps_t time;
			date_getAdjustedTime(&gpsData, &date, &time);
			gps_resetOdo();
		}
		appendSignal = 1;
	}
}

void render_signalTiles ()
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
	inst.rstats.oneSecondCounter++;
		
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

	map_render(&trackRecord, loc, heading, MAP_RENDER_LOCGRAPTHIC | MAP_RENDER_COMPASS | MAP_RENDER_MEASURE);
}

static void drawCompose (gpsdata_t *data)
{
	if (!log_runStatus()){
#if 0
		if (!trackRecord.firstFix)
			drawMap(&data->navAvg, data->misc.heading);
#else
			drawMap(&inst.lastFix, data->misc.heading);
#endif
	}else{
		if (trackRecord.marker){
			trackPoint_t *trkPt = &trackRecord.trackPoints[inst.runLog.idx];

			data->navAvg = trkPt->location;
			data->iTow = trkPt->iTow;
			data->misc.speed = trkPt->speed;
			data->misc.heading = trkPt->heading/100.0f;

			drawMap(&trkPt->location, data->misc.heading);

			if (!log_stateIsPaused())
				inst.runLog.idx += inst.runLog.step;
			if (inst.runLog.idx >= trackRecord.marker-1)
				inst.runLog.idx = 0;
		}
	}

	if (debugStrings.ready){
		debugStrings.ready--;
		drawDebugStrings(&debugStrings);
	}else{
		if (!ui_isEnabled(NULL, UI_ID_PANEL_LOGS))	// use a flag instead
			drawMapOverlay(&gpsData);
	}
}

static inline void frameCompose ()
{
	drawCompose(&gpsData);
	uiDraw();
	
#if 0
	const uint32_t total = touchCtx.touchDragTotal;
	if (touchCtx.touchDragTotal){
		for (uint32_t i = 0; i < total; i++)
			drawCircleFilled(touchCtx.touchCord[i].x, touchCtx.touchCord[i].y, 10.0f, COLOUR_PAL_BROWN);
	}
	
	drawCircleFilled(touchCtx.touchCord[0].x, touchCtx.touchCord[0].y, 10.0f, COLOUR_PAL_GREEN);
	drawCircleFilled(touchCtx.touchCord[total-1].x, touchCtx.touchCord[total-1].y, 10.0f, COLOUR_PAL_RED);
	//drawLine(touchCtx.touchCord[0].x, touchCtx.touchCord[0].y, touchCtx.touchCord[total-1].x, touchCtx.touchCord[total-1].y, COLOUR_PAL_ORANGE);
#endif
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
	onceSecondTimer.begin(ISR_onceSecond_sig, 1*990*1000);	// in microseconds
	onceSecondTimer.priority(180);

	tilesLoadTimer.begin(ISR_tilesLoad_sig, 1*50*1000);		// in microseconds
	tilesLoadTimer.priority(210);
}

static inline void uiDraw ()
{
	ui_draw(0, 0);
}

int uiInput (const int32_t x, const int32_t y, const uint32_t flags)
{
	int ret = ui_input(x, y, flags);
	if (ret)
		render_signalUpdate();
	return ret;
}

FLASHMEM void setup ()
{

#if (ENABLE_MTP)
	mtp_init();
#endif
		
	Serial.begin(SERIAL_RATE);

	if (MPU_CLOCK_FREQ > 60)
		mpu_setClockFreq(MPU_POWERSAVE_FREQ);

	init_display();
	fio_init();

#if (ENABLE_MTP)
	while (1)
		mtp_task();
	return;
#endif

	ui_init();
	gps_init();
	cmd_init();
	init_vfont();
	init_debugStrings();
	init_isrTimers();
#if (ENABLE_TOUCH_FT5216)
	touch_init();
#endif
	init_record();
	
	if (MPU_CLOCK_FREQ > 60)
		mpu_setClockFreq(MPU_CLOCK_FREQ);

	powersave_init();
	map_init(&vfontContext);
	fpRecord_init(&trackRecord);
	clock_init();

#if (ENABLE_ENCODERS)
	encoders_init();
#endif

	log_stop();
	page_set(RENDER_PAGE_MAP);
	drawPanel(1);
}

FLASHMEM void render_zoomIn ()
{
	float zoomlevel = sceneGetZoom(&inst);
	zoomlevel -= (zoomlevel * (0.1f*(float)zoomMultiplier));
	zoomMultiplier = 1;

	if (zoomlevel < SCENE_ZOOM_MIN) zoomlevel = SCENE_ZOOM_MIN;
	else if (zoomlevel > SCENE_ZOOM_MAX) zoomlevel = SCENE_ZOOM_MAX;

	sceneSetZoom(&inst, zoomlevel);
	sceneResetViewport(&inst);
	render_signalTiles();
}

FLASHMEM void render_zoomOut ()
{
	float zoomlevel = sceneGetZoom(&inst);
	zoomlevel += (zoomlevel * (0.1f*(float)zoomMultiplier));
	zoomMultiplier = 1;
	
	if (zoomlevel < SCENE_ZOOM_MIN) zoomlevel = SCENE_ZOOM_MIN;
	else if (zoomlevel > SCENE_ZOOM_MAX) zoomlevel = SCENE_ZOOM_MAX;

	sceneSetZoom(&inst, zoomlevel);
	sceneResetViewport(&inst);
	render_signalTiles();
}

FLASHMEM void render_zoomReset ()
{
	sceneSetZoom(&inst, SCENE_ZOOM);
	sceneResetViewport(&inst);
	sceneLoadTiles(&inst);
}


#if ENABLE_ENCODERS
FLASHMEM void doEncoders (encodersrd_t *encoders)
{
	if (encoders->encoder[2].positionChange != 0){
		if (page_get() == RENDER_PAGE_MAP){
			zoomMultiplier++;
			if (encoders->encoder[2].positionChange > 0){
				if (ui_isEnabled(0, UI_ID_PANEL_LOGS))
					op_push(OP_FUNC_LOG_DOWN);
				else
				op_push(OP_FUNC_ZOOMOUT);
			}else{
				if (ui_isEnabled(0, UI_ID_PANEL_LOGS))
					op_push(OP_FUNC_LOG_UP);
				else
					op_push(OP_FUNC_ZOOMIN);
			}
			op_go();

		}else if (page_get() == RENDER_PAGE_SPECTRUM){
			if (encoders->encoder[2].positionChange > 0){
				spectrumXMarker++;
				if (spectrumXMarker > 255) spectrumXMarker = 255;
			}else{
				spectrumXMarker--;
				if (spectrumXMarker < 0) spectrumXMarker = 0;
			}
			render_signalUpdate();
		}
		return;
	}

	if (encoders->encoder[2].buttonPress){
		if (ui_isEnabled(0, UI_ID_PANEL_LOGS))
			op_push(OP_FUNC_LOG_LOAD);
		else
			op_push(OP_FUNC_ZOOMRESET);
		op_go();
		return;
	}
	
	if (encoders->encoder[1].positionChange != 0){
		if (log_runStatus()){
			if (encoders->encoder[1].positionChange > 0)
				log_runAdvance(25);
			else
				log_runAdvance(-25);
			renderSignal = 1;
		}
		return;
	}
	
	if (encoders->encoder[0].buttonPress){
		if (!log_runStatus()){
			log_runStart();
			log_runPause();
		}else{
			log_runStop();
		}
		renderSignal = 1;
		return;
	}

	if (encoders->encoder[0].positionChange != 0){
		if (log_runStatus()){
			if (encoders->encoder[0].positionChange > 0)
				log_runAdvance(25);
			else
				log_runAdvance(-25);
			renderSignal = 1;			
		}else{
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
		}
		//printf(CS("Backlight: %i"), (int)level);
		return;
	}
}
#endif

FLASHMEM void console_printCmdStats (runState_t *stats)
{
	cmdSendResponse("");
	printf(CS("zoom:%.0f, temp:%.1f, nothing:%lu, update:%.1f"), sceneGetZoom(&inst), InternalTemperature.readTemperatureC(), stats->nothingCountSecond, inst.rstats.rtime.display);
	printf(CS("map:%.2f, strings:%i, route:%.2f"), stats->rtime.map, stats->rtime.strings, stats->rtime.trkpts);
	printf(CS("epoch:%i, rx:%i"), gpsData.rates.epochPerRead, receiver_getRx());
	receiver_resetRxTx();
}

FLASHMEM uint8_t page_get ()
{
	return inst.rstats.page;
}

FLASHMEM void page_set (const uint8_t page)
{
	inst.rstats.page = page;
	
	if (inst.rstats.page > RENDER_PAGE_TOTAL)
		inst.rstats.page = 1;
	
	switch (inst.rstats.page){
	case RENDER_PAGE_MAP:
		drawPanel(1);
		return;

	case RENDER_PAGE_ALTITUDE:
		drawPanel(4);
		return;

	case RENDER_PAGE_ROUTE:
		drawPanel(4);
		return;

	case RENDER_PAGE_SPECTRUM:
		drawPanel(4);
		gps_setReceiver(2);
		gps_msgEnable(UBX_MON, UBX_MON_SPAN);
		return;

	case RENDER_PAGE_CLOCK:
		drawPanel(4);
		gps_setReceiver(2);
		gps_msgDisable(UBX_MON, UBX_MON_SPAN);
		return;
	}
}

FLASHMEM void frameDrawSpectrum ()
{
	sat_stats_t *stats = getSats();
	mon_span_block_t *block = &stats->spectrum.block;


	// total bins
	int tPoints = (float)block->span / (float)block->res;
	if (tPoints > 256) tPoints = 256;
	
	// draw measure	
	const uint8_t graphCol = COLOUR_PAL_LIGHTGREY;
	const uint8_t labelCol = COLOUR_PAL_RED;
	const uint8_t titleCol = COLOUR_PAL_BLACK;
	const uint8_t specCol  = COLOUR_PAL_BLACK;
	const uint8_t markerCol = COLOUR_PAL_BLUE;
	
	// graph offset adjustment
	const float posX = 50.0f;
	const float posY = 30.0f;
	
	// scaling
	const float dbScale = -(2^-2);	// (0.25) according to PDF
	const float vscale = 2.2f;
#if (TFT_WIDTH < 900)
	const float hscale = 3.0f;
	const float titleXScale = 0.8f;
	const int titleXPos = 50;
#else
	const float hscale = 3.4f;
	const float titleXScale = 1.0f;
	const int titleXPos = 7;
#endif

	setGlyphScale(inst.vfont, titleXScale);
	setBrushSize(inst.vfont, 1.0f);
	setBrushColour(inst.vfont, titleCol);

	// title
	char buffer[96];
	snprintf(buffer, sizeof(buffer), "Span: %i Mhz    Res: %i Khz   Center: %i Mhz   Gain: %i", (int)block->span/1000000, (int)block->res/1000, (int)block->center/1000000, (int)block->pga);
	drawString(inst.vfont, buffer, titleXPos, 20);
	setBrushColour(inst.vfont, labelCol);
	setGlyphScale(inst.vfont, 0.8f);

	// freq bands
	// draw a few freq bands with labels
	float y1 = posY + (float)VHEIGHT - ((50.0f*dbScale)*vscale);
	float y2 = posY + (float)VHEIGHT - ((10.0f*dbScale)*vscale);
		
	float x1 = posX-hscale;
	drawLine(x1, y1-10, x1, y2+25, graphCol);
	float bin = 0.0f;
	float f = block->center + block->span * (bin-127.0f) / 256.0f;
	snprintf(buffer, sizeof(buffer), "%.1f", f/1000000.0f);
	drawString(inst.vfont, buffer, x1-10, y2+40);


	int x = posX + (((tPoints-1.0f) / 2.0f) * hscale);
	drawLine(x, y1-10, x, y2+25, graphCol);
	bin = tPoints/2.0f;
	f = block->center + block->span * (bin-127.0f) / 256.0f;
	snprintf(buffer, sizeof(buffer), "%.1f Mhz", f/1000000.0f);
	drawString(inst.vfont, buffer, x-50, y2+40);
	
		
	float x2 = posX + ((256.0f)*hscale);
	drawLine(x2, y1-10, x2, y2+25, graphCol);
	bin = 255.0f;
	f = block->center + block->span * (bin-127.0f) / 256.0f;
	snprintf(buffer, sizeof(buffer), "%.1f", f/1000000.0f);
	drawString(inst.vfont, buffer, x2-70, y2+40);

	setBrushColour(inst.vfont, markerCol);
	bin = spectrumXMarker;
	x = posX + (bin * hscale);
	drawLine(x, y1-10, x, y2+25, markerCol);
	f = block->center + block->span * (bin-127.0f) / 256.0f;
	snprintf(buffer, sizeof(buffer), "%.1f", f/1000000.0f);
	drawString(inst.vfont, buffer, VWIDTH/4, y2+20);
	
	x = posX - 30;
	float y = posY + (float)VHEIGHT - (block->spectrum[(int)bin]*vscale);
	drawLine(x, y, x+((float)tPoints*hscale)+posX, y, markerCol);
	

	// draw dB level marks and labels
	// draw -dB label above first point (-50db)
	setBrushColour(inst.vfont, labelCol);
	y = posY + (float)VHEIGHT - ((50.0f*dbScale)*vscale);
	drawString(inst.vfont, "dB", x-10, y-15);
	
	for (float db = 10.0f; db <= 50.0f; db += 10.0f){
		y = posY + (float)VHEIGHT - ((db*dbScale)*vscale);
		drawLine(x, y, x+((float)tPoints*hscale)+posX, y, graphCol);
		
		snprintf(buffer, sizeof(buffer), "%i", (int)db);
		drawString(inst.vfont, buffer, x-5, y+15);
	}
		
	// draw spectrum
	for (int i = 0; i < tPoints-1; i++){
		float x1 = posX + (i*hscale);
		float y1 = posY + (float)VHEIGHT - (block->spectrum[i]*vscale);
		
		float x2 = posX + ((i+1)*hscale);
		float y2 = posY + (float)VHEIGHT - (block->spectrum[i+1]*vscale);
		
		float a = RAD2DEG(atan2f(x2, y2) - atan2f(x1, y1));
		drawPolyFilled(x1, y1, x2, y2, 2.0f, a, specCol);
	}
}

FLASHMEM void frameDrawAltitude (trackRecord_t *track)
{
	
	const uint8_t altCol  = COLOUR_PAL_BLACK;
	const uint8_t labelCol = COLOUR_PAL_RED;
	const uint8_t measureCol = COLOUR_PAL_DARKGREEN;
	
	setGlyphScale(inst.vfont, 1.0f);
	setBrushSize(inst.vfont, 1.0f);
	setBrushColour(inst.vfont, labelCol);
		
	if (track->marker < 5){
		drawString(inst.vfont, "No Data", 20, 40);
		//return;
	}
	
	const int tPoints = track->marker;
	const float vscale = 0.8f;
	const float hscale = (float)(VWIDTH-100.0f) / (float)tPoints;
	
	// graph offset adjustment
	const float posX = 50.0f;
	const float posY = 30.0f;

	
	// 0 meters
	float y = (float)VHEIGHT - (0.0f*vscale) - posY;
	drawLine(posX-10, y, VWIDTH-(posX-10), y, measureCol);
	drawString(inst.vfont, "0m", posX-10, y+15);

	// 100 meters
	y = (float)VHEIGHT - (100.0f*vscale) - posY;
	drawLine(posX-10, y, VWIDTH-(posX-10), y, measureCol);
	drawString(inst.vfont, "100m", posX-10, y+15);
	
	// 400 meters
	y = (float)VHEIGHT - (400.0f*vscale) - posY;
	drawLine(posX-10, y, VWIDTH-(posX-10), y, measureCol);
	drawString(inst.vfont, "400m", posX-10, y+15);
	
	
	for (int i = 0; i < tPoints-1; i++){
		trackPoint_t *tpt = &track->trackPoints[i];
		trackPoint_t *tptNext = &track->trackPoints[i+1];
		
		float x1 = posX + ((float)i*hscale);
		float y1 = (float)VHEIGHT - (tpt->location.altitude*vscale) - posY;
		
		float x2 = posX + (((float)i+1.0f)*hscale);
		float y2 = (float)VHEIGHT - (tptNext->location.altitude*vscale) - posY;
		
		float a = RAD2DEG(atan2f(x2, y2) - atan2f(x1, y1));
		drawPolyFilled(x1, y1, x2, y2, 2.0f, a, altCol);
	}
}

FLASHMEM void frameDrawRoute (trackRecord_t *track)
{
	//find min max values
	
	const double width = VWIDTH-5;
	const double height = VHEIGHT-5;
	
	double minLat = 3600.0;
	double maxLat = -3600.0;

	double minLon = 3600.0;
	double maxLon = -3600.0;

	const int tPoints = track->marker;	
	for (int i = 10; i < tPoints-11; i+=4){
		trackPoint_t *tpt = &track->trackPoints[i];
		
		if (maxLat < tpt->location.latitude)
			maxLat = tpt->location.latitude;
		if (minLat > tpt->location.latitude)
			minLat = tpt->location.latitude;
		if (maxLon < tpt->location.longitude)
			maxLon = tpt->location.longitude;
		if (minLon > tpt->location.longitude)
			minLon = tpt->location.longitude;
	}

	const double spanLon = (maxLon - minLon);
	const double spanLat = (maxLat - minLat);

	double zoomFactor = 1.0;
	double aspect = spanLon / spanLat;
	double daspect = width / height;
	
	// 3.04 is the Strava.com map aspect/scale
	// 2.35 is my display aspect for a best fit (4/3 * 480/272)
	// scaleFactor = 3.04;
	double scaleFactor = 3.04;//1.71 * daspect;

	const double scaleX = (width / spanLon) / scaleFactor;
	const double scaleY = (height / spanLat);

	double offsetX, offsetY;
	double scale = ((daspect * aspect) / scaleFactor);

	// center route
	if (aspect <= scaleFactor){
		scale *= zoomFactor;
		offsetX = (width - (height * scale)) / 2.0;
		offsetY = (height - (height * zoomFactor))/2.0;
	}else{
		scale /= zoomFactor;
		offsetY = (height - (width / scale)) / 2.0;
		offsetX = (width - (width * zoomFactor))/2.0;
	}
	
	printf(CS("offset: %f %f, %f, %f %f"), offsetX, offsetY, scale, scaleX, scaleY);
	
	for (int i = 10; i < tPoints-11; i+=4){
		trackPoint_t *tpt = &track->trackPoints[i];
		double lon1 = tpt->location.longitude - minLon;
		double lat1 = tpt->location.latitude - minLat;

		tpt = &track->trackPoints[i+4];
		double lon2 = tpt->location.longitude - minLon;
		double lat2 = tpt->location.latitude - minLat;
		
		double x1 = offsetX + (scaleX * lon1);
		double y1 = height - (scaleY * lat1);
		
		double x2 = offsetX + (scaleX * lon2);
		double y2 = height - (scaleY * lat2);
		
		drawLine(x1, y1, x2, y2, COLOUR_PAL_BLACK);
	}
}

FLASHMEM void render_frame ()
{
	const uint32_t t0 = millis();
		
	switch (page_get()){
	case RENDER_PAGE_MAP:
		frameClear();
		frameCompose();
		frameSend();
		break;

	case RENDER_PAGE_ALTITUDE:
		frameClear();
		frameDrawAltitude(&trackRecord);
		frameSend();
		break;

	case RENDER_PAGE_ROUTE:
		frameClear();
		frameDrawRoute(&trackRecord);
		frameSend();
		break;

	case RENDER_PAGE_SPECTRUM:
		frameClear();
		frameDrawSpectrum();
		frameSend();
		break;
		
	case RENDER_PAGE_CLOCK:
		frameSendClock();
		break;
	}
	
	const uint32_t t1 = millis();
	inst.rstats.rtime.display = (t1 - t0);
}

FASTRUN void loop ()
{
#if ENABLE_TOUCH_FT5216
	if (touchCtx.tready){
		touch_task(&touchCtx);
		touchCtx.tready = 0;
	}
#endif

#if ENABLE_ENCODERS
	if (encoders_isReady()){
		encoders_read(&encoders);
		doEncoders(&encoders);
	}
#endif

	gps_task();

	if (gnssReceiver_PassthroughEnabled){
		gps_task();
		
#if (0 && ENABLE_TOUCH_FT5216)
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
		trkPt_trackRecordAppend(&trackRecord, &gpsData);
	}

	gps_task();

#if (0 && ENABLE_TOUCH_FT5216)
	if (touchCtx.tready){
		touch_task(&touchCtx);
		touchCtx.tready = 0;
	}
#endif

	if (renderSignal){
		if (!log_runStatus() || log_stateIsPaused())
			renderSignal = 0;

		render_frame();
		gps_task();

		if (inst.rstats.rflags.console && serialConnected)
			console_printCmdStats(&inst.rstats);
#if 0
		// load auto.ubx once we have a valid date, but not too quickly as not always taken
		// perform once per boot only
		if (inst.assistNowAutoLoad == 1){
			if (recordSignal > 15){
				inst.assistNowAutoLoad = 2;
				gps_loadOfflineAssist(0);
			}
		}
#endif
	}

#if (0 && ENABLE_TOUCH_FT5216)
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

	if (recordSignal > LOG_WRITE_PERIOD){
		recordSignal = 0;
		op_push(OP_FUNC_LOG_WRITE);
		op_push(OP_FUNC_GPS_TASK);
		op_go();
	}

	if (inst.cmdTaskRunMode){
		inst.cmdTaskRunMode = cmd_task(inst.heartbeatPulse);
		inst.heartbeatPulse = 0;
	}

#if (0 && ENABLE_TOUCH_FT5216)
	if (touchCtx.tready){
		touch_task(&touchCtx);
		touchCtx.tready = 0;
	}
#endif


	if (op_state() == OP_READY){
		if (op_execute(op_pop()))
			render_signalUpdate();
	}

	inst.rstats.nothingCount++;
}
