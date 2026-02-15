#pragma once
#ifndef _COMMONGLUE_H_
#define _COMMONGLUE_H_

#include <Arduino.h>
#include <unistd.h>
#include "InternalTemperature.h"
#include "IntervalTimer.h"
#include "config.h"
#include "displays.h"
#include "touch.h"
#include "encoders.h"
#include "vfont/vfont.h"
#include "palette.h"
#include "gps.h"
#include "fileio.h"
#include "polyfile.h"
#include "record.h"
#include "map.h"
#include "scene.h"
#include "timedate.h"
#include "cmd.h"
#include "tiles.h"
#include "poi.h"
#include "ui/ui.h"
#include "powersave.h"
#include "clock.h"


#if ENABLE_MTP
#include "mtp.h"
#endif



#define RENDER_PAGE_MAP			1
#define RENDER_PAGE_CLOCK		2
#define RENDER_PAGE_SPECTRUM	3

void page_set (const uint8_t page);
uint8_t page_get ();



#define LOG_WRITE_PERIOD		60	// write data to log once per n seconds


void mpu_reboot ();
void mpu_setClockFreq (const uint32_t freqMhz);
void mpu_updateMPUFreqMenu (const uint32_t freqMhz);

void log_setRecordState (const int state);
void log_setAcquisitionState (const int state);
void log_reset ();
int log_isActive ();
void log_pause ();
void log_stop ();
void log_start ();
int log_load (const char *filename);
int log_runStatus ();
int log_stateIsPaused ();

void log_runStep (const uint8_t step);
void log_runStop ();
void log_runSet (const uint32_t position);
void log_runStart ();
void log_runReset ();
void log_runPause ();
void log_runAdvance (const int32_t advanceBy);
void log_write ();

pos_rec_t log_getLastPosition ();
timegps_t log_getLastTime ();
dategps_t log_getLastDate ();
uint32_t log_getLastiTow ();

void render_signalUpdate ();
void render_signalTiles ();
void render_zoomIn ();
void render_zoomOut ();
void render_zoomReset ();

void drawPanel (const uint8_t which);
uint16_t *drawPanel_getPixels (const uint8_t which);

void render_screenBlank ();



int uiInput (const int32_t x, const int32_t y, const uint32_t flags);


#if defined(__IMXRT1062__)
extern "C" uint32_t set_arm_clock (uint32_t frequency);
#endif






#endif
