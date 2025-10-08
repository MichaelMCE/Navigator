

#include "commonGlue.h"






extern application_t inst;
extern trackRecord_t trackRecord;



void log_runReset ()
{
	inst.runLog.idx = 0;
	inst.runLog.step = 4;
}

void log_runStart ()
{
	inst.runLog.enabled = 1;
	inst.runLog.pause = 0;
}

void log_runSet (const uint32_t position)
{
	if (position < trackRecord.marker)
		inst.runLog.idx = position&0x00FFFFFF;
}

void log_runStop ()
{
	inst.runLog.enabled = 0;
}

void log_runPause ()
{
	inst.runLog.pause = (inst.runLog.pause+1)&0x01;
}

void log_runStep (const uint8_t step)
{
	inst.runLog.step = step&0x0F;
}

int log_load (const char *filename)
{
	log_runStop();
	log_runReset();

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

void log_reset ()
{
	extmem_free(trackRecord.trackPoints);
	
	fpRecord_init(&trackRecord);
	
	log_setAcquisitionState(1);
	log_setRecordState(1);
	gps_resetOdo();
	
	inst.rstats.trkptsTotal = 0;
	inst.rstats.trkptsToWrite = 0;
}

