#pragma once
#ifndef _TEENSY_H_
#define _TEENSY_H_






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


typedef struct _recvData {
	uint8_t *readIn;
	int inCt;
}recvDataCtx_t;


void doReboot ();
void log_setRecordState (const int state);
void log_setAcquisitionState (const int state);
void log_reset ();
int log_load (const char *filename);

#endif

