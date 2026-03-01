

#include "commonGlue.h"





trackRecord_t trackRecord;
extern application_t inst;


const char *log_getFilename ()
{
	return fpRecord_getFilename(&trackRecord);
}

void log_runReset ()
{
	inst.runLog.idx = 0;
	inst.runLog.step = 4;
	inst.runLog.pause = 0;
}

int log_stateIsPaused ()
{
	return inst.runLog.pause;
}

int log_runStatus ()
{
	return inst.runLog.enabled;
}

void log_runStart ()
{
	inst.runLog.enabled = 1;
	inst.runLog.pause = 0;
}

void log_runStop ()
{
	inst.runLog.enabled = 0;
	inst.loadTiles = 1;
	render_signalUpdate();
}

void log_runPause ()
{
	inst.runLog.pause = (inst.runLog.pause+1)&0x01;
}

void log_runAdvance (const int32_t advanceBy)
{
	int32_t newTrk = ((int32_t)inst.runLog.idx + advanceBy);
	if (newTrk < 0){
		newTrk = (trackRecord.marker-1) - abs(newTrk);
		if (newTrk < 0) newTrk = 0;
	}else if (newTrk >= (int32_t)trackRecord.marker){
		newTrk = 0;
	}
	
	inst.runLog.idx = newTrk&0x00FFFFFF;
}

void log_runSet (const uint32_t position)
{
	if (position < trackRecord.marker)
		inst.runLog.idx = position&0x00FFFFFF;
}

void log_runStep (const uint8_t step)
{
	inst.runLog.step = step&0x0F;
}

int log_load (const char *filename)
{
	log_runStop();
	log_runReset();

	inst.loadTiles = 1;

	return fpRecord_import(&trackRecord, filename);
}

void log_setRecordState (const int state)
{
	if (state)
		trackRecord.writeDisabled = 0;		// allow writes
	else
		trackRecord.writeDisabled = 1;		// disable writes
}

void log_setAcquisitionState (const int state)
{
	if (state)
		trackRecord.acquDisabled = 0;		// enable logging
	else
		trackRecord.acquDisabled = 1;		// disable it
}

int log_getAcquisitionState ()
{
	return !trackRecord.acquDisabled;
}

int log_getRecordState ()
{
	return !trackRecord.writeDisabled;
}

void log_reset ()
{
	fpRecord_free(&trackRecord);
	fpRecord_init(&trackRecord);
	
	log_setAcquisitionState(1);
	log_setRecordState(1);
	gps_resetOdo();
	
	inst.rstats.trkptsTotal = 0;
	inst.rstats.trkptsToWrite = 0;
}

void log_start ()
{
	trackRecord.recordActive = 1;
	trackRecord.acquDisabled = 0;
}

void log_stop ()
{
	trackRecord.recordActive = 0;
	trackRecord.acquDisabled = 1;
	trackRecord.firstFix = 0;
}

int log_hasFirstFix ()
{
	return trackRecord.firstFix;
}

void log_pause ()
{
	trackRecord.acquDisabled = 1;
}

int log_isActive ()
{
	return trackRecord.recordActive;
}

void log_write ()
{
	if (trackRecord.recordActive){
		if (!trackRecord.writeDisabled)
			fpRecord_appendLog(&trackRecord);
	}
}
