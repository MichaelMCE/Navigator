
#ifndef _CMD_H_
#define _CMD_H_


#define CMD_LIST			"/list"
#define CMD_DELETE			"/delete"
#define CMD_TOUCH			"/touch"
#define CMD_RENAME			"/rename"
#define CMD_GETMETA			"/getmeta"
#define CMD_GETMETABIN		"/getmetabin"
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
#define CMD_ULOAD		    "/uload"
#define CMD_MPU			    "/mpu"
#define CMD_SOS			    "/sos"
#define CMD_SENDFILE	    "/sendfile"		// send file from client to navigator
#define CMD_GETFILE			"/getfile"		// retrieve file from navigator
#define CMD_RUNLOG		    "/runlog"
#define CMD_END			  "<cmd:end>"		// tag end

#if 0
#define CL "<response:msg>"
#define CR "<response:end>\n"
#else
#define CL ""
#define CR "\n"
#endif

#define CS(a)	CL a CR





// <cmd:list><cmd:end>											// list everything in data/
// <cmd:delete>a filename.ext<cmd:end>							// remove file within data/
// <cmd:delete>*<cmd:end>										// delete all files within data/
// <cmd:rename>a filename from.ext:a filename to.ext<cmd:end>	// rename a file from:to
// <cmd:getfiledata>a filename.ext<cmd:end>						// restrive file. respond with: <response:data>bin data<response:end>
// <cmd:getfilelength>a filename.ext<cmd:end>					// restrive length of file. respond with: <response:filename,length>This is a filename.ext:123456<response:end>
// error: <response:error>an error message<response:end>
// msg: <response:msg><response:end>



void cmd_init ();
int cmd_task (const int pulse);


void cmdSendError (const char *err);
void cmdSendResponse (const char *msg);


#ifdef __cplusplus
extern "C" {
#endif

int cmdLoadUbx (const char *filename);


#ifdef __cplusplus
}
#endif




typedef struct {
	struct {
		uint32_t read;
		uint32_t expected;
	}length;
	char *pending;
}file_trans_t;

typedef struct {
	uint8_t sec;   // 0-59
	uint8_t min;   // 0-59
	uint8_t hour;  // 0-23
	uint8_t wday;  // 0-6, 0=sunday
	
	uint8_t mday;  // 1-31
	uint8_t mon;   // 0-11
	uint16_t year;  // 70-206, 70=1970, 206=2106
	
	uint32_t length;	// file length
}__attribute__((packed))cmd_fileMeta_t;

typedef struct {
	const char *cmd;
	void (*func)(char *msg, const int msgLen);
	const char *helpStr;
}cmdstr_t;


#endif
