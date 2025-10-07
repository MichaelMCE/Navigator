#pragma once
#ifndef _TEENSY_H_
#define _TEENSY_H_

#include <Arduino.h>
#include "InternalTemperature.h"
#include "IntervalTimer.h"
#include "config.h"
#include "touch.h"
#include "encoders.h"
#include "vfont/vfont.h"
#include "palette.h"
#include "gps.h"
#include "record.h"
#include "map.h"
#include "scene.h"
#include "timedate.h"
#include "cmd.h"




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

#endif

