
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>
#include "fileio.h"
#include "gps.h"
#include "record.h"
#include "cmd.h"




FLASHMEM int fpRecord_init (trackRecord_t *trackRecord)
{
	memset(trackRecord, 0, sizeof(*trackRecord));
	
	trackRecord->trackPoints = (trackPoint_t*)extmem_calloc(1, sizeof(trackPoint_t)*TRACKPTS_MAX);
	trackRecord->recordActive = 0;
	trackRecord->firstFix = 0;
	trackRecord->writeDisabled = 0;
	trackRecord->acquDisabled = 0;

	if (!SD.exists(TRACKPTS_DIR))
		fio_mkdir(TRACKPTS_DIR);

	return 1;
}

int fpRecord_open (trackRecord_t *trackRecord, const uint8_t *filename, const uint32_t flags)
{
	trackRecord->fp = fio_open(filename, flags);
	//printf(CS("fpRecord_open: %s"), filename);
	return (trackRecord->fp != 0);
}

int fpRecord_read (trackRecord_t *trackRecord, const int tpFrom, const int tpTo)
{
	void *buffer = &trackRecord->trackPoints[tpFrom];
	int total = (tpTo - tpFrom) + 1;
		
	if (!fio_seek(trackRecord->fp, tpFrom * sizeof(trackPoint_t))){
		printf(CS("fpRecord_read: fio_seek failed to %i, %i %i"), tpFrom * sizeof(trackPoint_t), tpFrom, tpTo);
		return 0;
	}

	int ret = fio_read(trackRecord->fp, buffer, total * sizeof(trackPoint_t));
	//printf(CS("fpRecord_read: ret %i"), ret);
	return ret;
}

int fpRecord_write (trackRecord_t *trackRecord, const int tpFrom, const int tpTo)
{
	void *buffer = &trackRecord->trackPoints[tpFrom];
	int total = (tpTo - tpFrom) + 1;
		
	if (!fio_seek(trackRecord->fp, tpFrom * sizeof(trackPoint_t))){
		printf(CS("fpRecord_write: fio_seek failed to %i, %i %i"), tpFrom * sizeof(trackPoint_t), tpFrom, tpTo);
		return 0;
	}

	int ret = fio_write(trackRecord->fp, buffer, total * sizeof(trackPoint_t));
	printf(CS("fpRecord_write: ret %i"), ret);
	return ret;
}

void fpRecord_close (trackRecord_t *trackRecord)
{
	fio_close(trackRecord->fp);
}

void fpRecord_appendLog (trackRecord_t *trackRecord)
{
	int recFrom = trackRecord->lastFrom;
	int recTo = trackRecord->marker-1;
		
	printf(CS("recordSignal signal: %i %i"), recFrom, recTo);
	
	if (recFrom < recTo && (recTo - recFrom > 55)){
		if (fpRecord_open(trackRecord, (uint8_t*)trackRecord->filename, FIO_NEW|FIO_WRITE)){
			if (fpRecord_write(trackRecord, recFrom, recTo))
				trackRecord->lastFrom = recTo+1;
		}

		fpRecord_close(trackRecord);
	}
}

int fpRecord_import (trackRecord_t *trackRecord, const char *filename)
{
	int ct = 0;
	if (fpRecord_open(trackRecord, (uint8_t*)filename, FIO_READ)){
		size_t length = fio_length(trackRecord->fp);
		if (length){
			uint32_t totalPts = length / sizeof(trackPoint_t);
			if (totalPts <= TRACKPTS_MAX){
				if (fpRecord_read(trackRecord, 0, totalPts-1)){
					strcpy(trackRecord->filename, filename);
					trackRecord->lastFrom = totalPts;
					trackRecord->marker = totalPts;
					trackRecord->firstFix = 1;
					trackRecord->recordActive = 1;
					ct = totalPts;
				}else{
					cmdSendError("fpRecord_read: track read failed");
				}
			}else{
				cmdSendError("fpRecord_read: invalid length");
			}
		}else{
			cmdSendError("fpRecord_read: nothing to read");
			
		}
		fpRecord_close(trackRecord);
	}else{
		cmdSendError("fpRecord_read: open failed");
	}
	return ct;
}
