
#ifndef _record_h_
#define _record_h_


#define TRACKPTS_DIR	"/data/"		// save track points here
#define TRACKPTS_MAX	(60*60*24)		// 1 day


typedef struct {
	uint32_t iTow;		// UTC time as supplied by module
	pos_rec_t location;
	uint16_t heading;	// heading(*10) as return from module
	uint16_t speed;		// speed*10 km/h
}trackPoint_t;

typedef struct {
	//trackPoint_t trackPoints[TRACKPTS_MAX];
	trackPoint_t *trackPoints;
	uint32_t marker;
	uint32_t lastFrom;
	fileio_t *fp;
	
	uint32_t recordActive:1;
	uint32_t firstFix:1;
	uint32_t writeDisabled:1;		// 1 if trkPt writes to sdcard are disabled
	uint32_t acquDisabled:1;		// 1 if trpPt acquisition s disabled
	uint32_t stub:28;
	
	char filename[32];
}trackRecord_t;


int fpRecord_init (trackRecord_t *trackRecord);
int fpRecord_open (trackRecord_t *trackRecord, const uint8_t *filename, const uint32_t flags);
int fpRecord_write (trackRecord_t *trackRecord, const int tpFrom, const int tpTo);
void fpRecord_close (trackRecord_t *trackRecord);
void fpRecord_appendLog (trackRecord_t *trackRecord);
int fpRecord_read (trackRecord_t *trackRecord, const int tpFrom, const int tpTo);
int fpRecord_import (trackRecord_t *trackRecord, const char *filename);



#endif

