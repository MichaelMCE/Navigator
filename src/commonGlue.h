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

#if ENABLE_MTP
#include "mtp.h"
#endif




void mpu_reboot ();
void log_setRecordState (const int state);
void log_setAcquisitionState (const int state);
void log_reset ();
int log_isActive ();
void log_pause ();
void log_stop ();
void log_start ();
int log_load (const char *filename);

void log_runStep (const uint8_t step);
void log_runStop ();
void log_runSet (const uint32_t position);
void log_runStart ();
void log_runReset ();
void log_runPause ();
void log_runAdvance (const int32_t advanceBy);

void mpu_setClockFreq (const uint32_t freqMhz);

void render_signalUpdate ();
void render_signalTiles ();
void render_zoomIn ();
void render_zoomOut ();
void render_zoomReset ();

void drawPanel (const uint8_t which);

int uiInput (const int32_t x, const int32_t y, const uint32_t flags);


#if defined(__IMXRT1062__)
extern "C" uint32_t set_arm_clock (uint32_t frequency);
#endif


#endif
