
#ifndef _CMD_H_
#define _CMD_H_


#define CMD_LIST			"<cmd:list>"
#define CMD_DELETE			"<cmd:delete>"
#define CMD_TOUCH			"<cmd:touch>"
#define CMD_RENAME			"<cmd:rename>"
#define CMD_GETFILE			"<cmd:getfiledata>"
#define CMD_GETFILELEN		"<cmd:getfilelength>"
#define CMD_EXIT			"<cmd:exit>"			// shutdown <this> engine
#define CMD_HELLO			"<cmd:hello>"
#define CMD_END				"<cmd:end>"				// tag end
#define CMD_REBOOT			"<cmd:reboot>"
#define CMD_LOGCFG			"<cmd:logcfg>"
#define CMD_ZOOM			"<cmd:viewport>"
#define CMD_DETAIL			"<cmd:renderdetail>"
#define CMD_BRIGHTNESS		"<cmd:backlight>"
#define CMD_LOAD			"<cmd:load>"
#define CMD_FDATA			"<cmd:fdata>"
#define CMD_MAPSCHEME		"<cmd:scheme>"
#define CMD_DEBUG			"<cmd:debug>"
#define CMD_RECEIVER		"<cmd:receiver>"
#define CMD_ODO				"<cmd:odo>"
#define CMD_ULOAD			"<cmd:uload>"
#define CMD_SOS			    "<cmd:sos>"
#define CMD_RUNLOG		    "<cmd:runlog>"
#define CMD_MPU			    "<cmd:mpu>"


#define CL "<response:msg>"
#define CR "<response:end>\n"
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



#endif

