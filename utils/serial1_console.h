

#ifndef serial1_console_h
#define serial1_console_h

#define CMD_LIST			"/list"
#define CMD_DELETE			"/delete"
#define CMD_TOUCH			"/touch"
#define CMD_RENAME			"/rename"
#define CMD_FILEMETA	    "/filemeta"
#define CMD_GETFILE		    "/getfile"
//#define CMD_EXIT			"/exit"			// shutdown <this> engine
#define CMD_HELLO			"/hello"
#define CMD_REBOOT			"/reboot"
#define CMD_ROUTE			"/route"
#define CMD_ZOOM			"/zoom"
#define CMD_DETAIL			"/detail"
#define CMD_PAGE			"/page"
#define CMD_BRIGHTNESS		"/backlight"
#define CMD_LOAD			"/load"
#define CMD_MAPSCHEME		"/style"
#define CMD_DEBUG			"/debug"
#define CMD_RECEIVER		"/receiver"
#define CMD_ODO				"/odo"
#define CMD_ULOAD		    "/ubxload"
#define CMD_MPU			    "/mpu"
#define CMD_SOS			    "/sos"
#define CMD_SENDFILE	    "/sendfile"		// send file from client to navigator
#define CMD_GETFILE			"/getfile"		// retrieve file from navigator
#define CMD_RUNLOG		    "/runlog"
#define CMD_END			  "<cmd:end>"		// tag end


typedef struct {
	char *cmd;
	void (*func)(const char *cmdStr);
	char *helpStr;
}cmdstr_t;

typedef struct{
	double longitude;
	double latitude;
	float altitude;		// hMSL
}__attribute__((packed))pos_rec_t;
typedef struct  FILE fileio_t;


#include "../src/record.h"
#include "../src/cmd.h"


#endif

