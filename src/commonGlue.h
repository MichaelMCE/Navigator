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

#if ENABLE_MTP
#include "mtp.h"
#endif




void doReboot ();
void log_setRecordState (const int state);
void log_setAcquisitionState (const int state);
void log_reset ();
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


#if defined(__IMXRT1062__)
extern "C" uint32_t set_arm_clock (uint32_t frequency);
#endif


#endif

