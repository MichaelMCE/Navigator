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



#if ENABLE_TOUCH_FT5216
typedef struct _touchCtx {
	touch_t touch;
	
	uint8_t enabled;	// send reports. does not reflect current FT5216 comm state
	uint8_t pressed;	// is being pressed
	uint8_t rotate;		// touch rotation direction
	uint8_t tready;
	
	elapsedMillis t0;
}touchCtx_t;

#define TOUCH_REPORTS_HALT		0
#define TOUCH_REPORTS_OFF		1
#define TOUCH_REPORTS_ON		2
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

#endif

