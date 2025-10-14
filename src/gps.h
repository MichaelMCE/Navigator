

#ifndef _GPS_H_
#define _GPS_H_


#include "ubx/ubx.h"
#include "ubx/ubxcb.h"
#include "location.h"



#define SERIAL_RATE				(230400)
#define DEBUG_LINES				(24)
#define DEBUG_LINE_LEN			(42)


#define COM_BAUD_9600			0
#define COM_BAUD_19200			1
#define COM_BAUD_38400			2
#define COM_BAUD_57600			3
#define COM_BAUD_115200			4
#define COM_BAUD_230400			5
#define COM_BAUD_460800			6
#define COM_BAUD_921600			7

#define COM_BAUD				COM_BAUD_230400
#define COM_BAUD_FWDEFAULT		COM_BAUD_9600
#define COM_BAUD_LASTSAVED		COM_BAUD_115200
#define BAUDRATE(n)				(baudRates[(n)])

#define ASSISTNOW_FILENAME		"auto.ubx"




typedef struct {
	uint8_t *line[DEBUG_LINES][DEBUG_LINE_LEN];
	
	uint8_t totalAdded;
	uint8_t ready;
	uint8_t stub[2];
	
	uint32_t timeAdded;
}debugOverlay_t;



typedef struct{
	double longitude;
	double latitude;
	float altitude;		// hMSL
}__attribute__((packed))pos_rec_t;

typedef struct {
	uint16_t year;
	uint8_t month;
	uint8_t day;
}dategps_t;
	
typedef struct {
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint8_t ms;			// ms*10 
}timegps_t;


typedef struct {
	pos_rec_t nav;
	pos_rec_t navAvg;
	
	struct{
	  uint8_t type;
	  uint8_t sats;			// Satellites used for this fix
	  uint16_t hAcc;		// 2D Acc
	  uint16_t vAcc;
	  uint16_t pAcc;		// 3D Acc
	}fix;

	struct{
	  uint16_t horizontal;
	  uint16_t vertical;
	  uint16_t position;
	  uint16_t geometric;
	}dop;
	
	struct{
		float speed;		//		km/h   kilometer per hour
		float heading;
		uint32_t distance;
	}misc;
	
	uint32_t iTow;
	dategps_t date;
	timegps_t time;

	uint32_t timeAdjusted:1;
	uint32_t firstFix:1;
	uint32_t dateConfirmed:1;
	uint32_t timeConfirmed:1;
	uint32_t stub:28;
	
	struct{
		uint32_t msgCt;	
		uint32_t rx;
		uint32_t tx;
		
		int16_t epoch;
		int16_t epochPerRead;
	}rates;
}gpsdata_t;



typedef struct {
	uint8_t gnssId;						// GNSSID_
	uint8_t svId;
	uint8_t cno;
	int8_t elev;
	
	int16_t azim;
	int16_t prRes;
	
	uint8_t flags;
	uint8_t stub[3];
}sat_status_t;

typedef struct {
	uint8_t numSvs;

	sat_status_t sv[72];
}sat_stats_t;




void gps_init ();
void gps_task ();


#ifdef __cplusplus
extern "C" {
#endif


void msgPostMed (const gpsdata_t *const opaque, const intptr_t unused);
void addDebugLine (const uint8_t *str);

int gps_serialWrite (uint8_t *buffer, uint32_t bufferSize);
void gps_configurePorts (ubx_device_t *dev);
void gps_configure (ubx_device_t *dev);
uint32_t gps_ubxMsgRun (ubx_device_t *dev, uint8_t *inBuffer, uint32_t bufferSize, int32_t *writePos, uint8_t *serBuffer, uint8_t serLen);

void gps_printVersions ();
void gps_printStatus ();
void gps_coldStart ();
void gps_warmStart ();
void gps_hotStart ();

void gps_resetOdo ();
void gps_startOdo ();
void gps_stopOdo ();

void gps_sosCreateBackup ();
void gps_sosClearFlash ();
void gps_sosPoll ();

void gps_setIntialPosition (const double lat, const double lon, const float alt_meters, const uint32_t posAcc_cm);
void gps_loadOfflineAssist (const int printInfo);

int gps_writeUbx (void *buffer, const uint32_t bufferSize);

int gps_pollMsg (const char *name);
void gps_pollInf (const uint8_t protocolID);

sat_stats_t *getSats ();
const char *getFixName (const uint8_t type);
void ms_delay (const uint32_t timeMs);

void getDateTime (dategps_t *date, timegps_t *time);

#ifdef __cplusplus
}
#endif




#endif
