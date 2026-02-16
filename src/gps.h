

#ifndef _GPS_H_
#define _GPS_H_


#include "ubx/ubx.h"
#include "location.h"


#define UART_BAUD				(230400)		// receiver module
#define SERIAL_RATE				UART_BAUD		// Teensy <> PC



#define ASSISTNOW_FILENAME		"auto.ubx"



#define DEBUG_LINES				(24)		// shouldn't be here
#define DEBUG_LINE_LEN			(42)		// shouldn't be here

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
		uint32_t tx;
		uint32_t rx;
		
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


#define LOCATION_BINSIZE		(60)		// bin size of at least 2 seconds (functional rate * 2)

typedef struct {
	int32_t numSvs;
	sat_status_t sv[128];
	
	struct {
		int writePos;
		pos_rec_t sum[LOCATION_BINSIZE];
	}location;

	uint32_t status_msSS[2];						// milliseconds since Startup / Reset
	uint32_t resetCt[2];
	
	mon_spectrum_t spectrum;
}sat_stats_t;


void gps_init ();
void gps_task ();


#ifdef __cplusplus
extern "C" {
#endif


void receiver_cb (const gpsdata_t *const opaque, const intptr_t unused);
void addDebugLine (const uint8_t *str);

int gps_serialWrite (uint8_t *buffer, uint32_t bufferSize);

void gps_reconnect_noConfigure ();
void gps_reconfigure ();
void gps_baudDiscover ();

void gps_setBaud (const uint32_t baud);	// sets baud but does not [re]connect
uint32_t gps_getBaud ();	// sets baud but does not [re]connect

void gps_printVersions ();
void gps_printStatus ();
void gps_printPositionAlt ();

void gps_coldStart ();
void gps_warmStart ();
void gps_hotStart ();

void gps_reinit ();
void gps_reconnect ();
void gps_status ();

void gps_resetOdo ();
void gps_startOdo ();
void gps_stopOdo ();

void gps_setRate (const uint32_t rate);

void gps_msgEnable (const uint8_t clsId, const uint8_t msgId);
void gps_msgDisable (const uint8_t clsId, const uint8_t msgId);

void gps_sosCreateBackup ();
void gps_sosClearFlash ();
void gps_sosPoll ();

void gps_saveConfig ();

void gps_updateReceiverGNSSMenu (const cfg_gnss_t *gnss);
void gps_updateReceiverRateMenu (const uint32_t measRate);
void gps_updateReceiverBaudMenu (const uint32_t baud);

uint8_t gps_getPortActive ();

void gps_setReceiver (const uint8_t port);



//void gps_setIntialPosition (const double lat, const double lon, const float alt_meters, const uint32_t posAcc_cm);
void gps_loadOfflineAssist (const int printInfo);

int gps_writeUbx (void *buffer, const uint32_t bufferSize);

int gps_pollMsg (const char *name);
void gps_pollInf (const uint8_t protocolID);

sat_stats_t *getSats ();
const char *getFixName (const uint8_t type);
void ms_delay (const uint32_t timeMs);

void getDateTime (dategps_t *date, timegps_t *time);

int isSerialConsoleConnected ();

#ifdef __cplusplus
}
#endif




#endif
