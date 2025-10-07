


#include <Arduino.h>
#include <unistd.h>
#include <SD.h>
#include "fileio.h"
#include "config.h"
#include "vfont/vfont.h"
#include "gps.h"
#include "scene.h"
#include "map.h"
#include "cmd.h"
#include "Teensy.h"
#include "record.h"




typedef struct {
	struct {
		uint32_t read;
		uint32_t expected;
	}length;
	char *pending;
}file_trans_t;

static file_trans_t fileTrans;





// <cmd:list><cmd:end>											// list everything in data/
// <cmd:delete>a filename.ext<cmd:end>							// remove file within data/
// <cmd:delete>*<cmd:end>										// delete all files within data/
// <cmd:rename>a filename from.ext:a filename to.ext<cmd:end>	// rename a file from:to
// <cmd:getfiledata>a filename.ext<cmd:end>						// restrive file. respond with: <response:data>bin data<response:end>
// <cmd:getfilelength>a filename.ext<cmd:end>					// restrive length of file. respond with: <response:filename,length>This is a filename.ext:123456<response:end>
// error: <response:error>an error message<response:end>
// msg: <response:msg><response:end>

extern int gnssReceiver_PassthroughEnabled;
extern application_t inst;


static inline void serialFlush ()
{
	Serial.send_now();
}

static inline int validateFilename (const char *filename)
{
	const char *invalid = "\\/:*?\"<>|";
	
	for (int i = 0; invalid[i]; i++){
		if (strchr(filename, invalid[i]))
			return 0;
	}
	return 1;
}

FLASHMEM void cmdSendError (const char *err)
{
	printf("<response:error>%s<response:end>\n", err);
	serialFlush();
}

FLASHMEM void cmdSendResponse (const char *msg)
{
	printf("<response:msg>%s<response:end>\n", msg);
	serialFlush();
}

FLASHMEM static File file_open (const char *file)
{
	fio_setDir(TRACKPTS_DIR);
	return SD.open(file);
}

FLASHMEM static int cmdListDir (const char *dir)
{
	int fileCount = 0;

	File root = file_open(dir);
	while (true){
    	File entry = root.openNextFile();
    	if (!entry) break; // no more files

    	if (entry.isDirectory()){
      		printf("<response:msg>  %s/<response:end>\n", entry.name());
    	}else{
    		const char *name = entry.name();
    		if (name){
    			uint64_t size = entry.size();

    			printf("<response:msg>  %8lli %s<response:end>\n", size, name);
    		}else{
    			cmdSendError("Could not read directory");
    		}
    		fileCount++;
    	}
    	entry.close();
		serialFlush();
		delay(25);
  	}
	
	fio_setDir("/");
  	return fileCount;
}


FLASHMEM static void cmd_fdata (char *msg, const int cmdlen)
{
	if (!strncmp(msg, "start:", 6)){
		fileTrans.length.expected = atoi(&msg[6]);
		
		if (fileTrans.length.expected < 1 || fileTrans.length.expected > 2*1024*1024){		// sensible limits
			cmdSendError("Invalid length");
			return;
		}
		
		if (fileTrans.pending) extmem_free(fileTrans.pending);
		fileTrans.pending = (char*)extmem_calloc(1, fileTrans.length.expected);

		fileTrans.length.read = 0;
		int toRead = fileTrans.length.expected;

		for (int i = 0; toRead > 0; i += 512){
			int ct = 0;
			if (toRead >= 512){
				ct = Serial.readBytes((char*)&fileTrans.pending[i], 512);
				if (ct != 512) break;
			}else{
				ct = Serial.readBytes((char*)&fileTrans.pending[i], toRead);
				if (ct != toRead) break;
			}
			fileTrans.length.read += ct;
			toRead -= 512;
 		}
		printf(CS("%i bytes of %i received"), (int)fileTrans.length.read, (int)fileTrans.length.expected);
		
	}else if (!strncmp(msg, "end:", 4)){
		const char *filename = &msg[4];

		if (!fileTrans.length.read){
			cmdSendError("No data to save");
			return;
		}

		if (!validateFilename(filename)){
			cmdSendError("Invalid filename");
		}else{

			fio_setDir(TRACKPTS_DIR);
			File file = SD.open(filename, FILE_WRITE);
			if (file){
				uint64_t pos = 0;
				file.seek(pos, SEEK_SET);
				
				int written = file.write(fileTrans.pending, (int)fileTrans.length.read);
				file.close();
				
				if (written != (int)fileTrans.length.read)
					cmdSendError("Write Failed");
				else
					printf(CS("%i bytes written to %s"), (int)written, filename);
			}
			fio_setDir("/");
			
			if (fileTrans.pending) extmem_free(fileTrans.pending);
			memset(&fileTrans, 0, sizeof(fileTrans));
		}
	}
}

FLASHMEM static void cmd_scheme (char *msg, const int cmdlen)
{
	if (!strncmp(msg, "colour:", 7)){
		uint8_t colourScheme = atoi(&msg[7])&0xFF;
		sceneSetColourScheme(colourScheme);
	}
}

FLASHMEM static void cmd_odo (char *msg, const int cmdlen)
{
	if (!strncmp(msg, "stop", 4))
		gps_stopOdo();
	else if (!strncmp(msg, "start", 5))
		gps_startOdo();
	else if (!strncmp(msg, "reset", 5))
		gps_resetOdo();
}

FLASHMEM static void cmd_receiver (char *msg, const int cmdlen)
{
	if (!strncmp(msg, "poll:inf:", 9)){
		const char *proto = &msg[9];
		if (!strcmp(proto, "ubx"))
			gps_pollInf(INF_PROTO_UBX);
		else if (!strcmp(proto, "raw"))
			gps_pollInf(INF_PROTO_RAW);
		else if (!strcmp(proto, "nema"))
			gps_pollInf(INF_PROTO_NEMA);
		else if (!strcmp(proto, "rtcm3"))
			gps_pollInf(INF_PROTO_RTCM3);
		else if (!strcmp(proto, "user0"))
			gps_pollInf(INF_PROTO_USER0);
		else if (!strcmp(proto, "user1"))
			gps_pollInf(INF_PROTO_USER1);
		else if (!strcmp(proto, "user2"))
			gps_pollInf(INF_PROTO_USER2);
		else if (!strcmp(proto, "user3"))
			gps_pollInf(INF_PROTO_USER3);
	}else if (!strncmp(msg, "poll:", 5)){
		char *pollMsg = &msg[5];
		if (!gps_pollMsg(pollMsg))
			printf(CS("ubx message '%s' not available"), pollMsg);

	}else if (!strncmp(msg, "status", 6)){
		gps_printStatus();
	}else if (!strncmp(msg, "version", 7)){
		gps_printVersions();
	}else if (!strncmp(msg, "hotstart", 8)){
		cmdSendResponse("Resetting: hotstart");
		gps_hotStart();
	}else if (!strncmp(msg, "warmstart", 9)){
		cmdSendResponse("Resetting: warmstart");
		gps_warmStart();
	}else if (!strncmp(msg, "coldstart", 9)){
		cmdSendResponse("Resetting: coldstart");
		gps_coldStart();
	}else if (!strncmp(msg, "passthrough:", 12)){
		uint8_t state = atoi(&msg[12])&0x01;
		gnssReceiver_PassthroughEnabled = state;
	}
}

FLASHMEM static void cmd_brightness (char *msg, const int cmdlen)
{
	if (!strncmp(msg, "level:", 6)){
		uint8_t level = atoi(&msg[6])&0xFF;
		if (level) tft_setBacklight(level);
	}
}

FLASHMEM static void cmd_zoom (char *msg, const int cmdlen)
{
	if (!strncmp(msg, "zoom:", 5)){
		float zoomlevel = atof(&msg[5]);

		sceneSetZoom(&inst, zoomlevel);
		sceneResetViewport(&inst);
		//sceneLoadTiles(&inst);
	}
}
		
FLASHMEM static void cmd_detail (char *msg, const int cmdlen)
{
	if (!strncmp(msg, "poi:", 4)){
		map_setDetail(MAP_RENDER_POI, atoi(&msg[4])&0x01);

	}else if (!strncmp(msg, "map:", 4)){	
		map_setDetail(MAP_RENDER_VIEWPORT, atoi(&msg[4])&0x01);
			
	}else if (!strncmp(msg, "world:", 6)){
		map_setDetail(MAP_RENDER_SWORLD, atoi(&msg[6])&0x01);
			
	}else if (!strncmp(msg, "route:", 6)){
		map_setDetail(MAP_RENDER_TRACKPOINTS, atoi(&msg[6])&0x01);
			
	}else if (!strncmp(msg, "compass:", 8)){	
		map_setDetail(MAP_RENDER_COMPASS, atoi(&msg[8])&0x01);
			
	}else if (!strncmp(msg, "slevels:", 8)){
		map_setDetail(MAP_RENDER_SLEVELS, atoi(&msg[8])&0x01);

	}else if (!strncmp(msg, "console:", 8)){
		map_setDetail(MAP_RENDER_CONSOLE, atoi(&msg[8])&0x01);

	}else if (!strncmp(msg, "locgraphic:", 11)){	
		map_setDetail(MAP_RENDER_LOCGRAPTHIC, atoi(&msg[11])&0x01);
						
	}else if (!strncmp(msg, "savailability:", 14)){
		map_setDetail(MAP_RENDER_SAVAIL, atoi(&msg[14])&0x01);
	}
}

FLASHMEM static void cmd_log (char *msg, const int cmdlen)
{
	if (!strncmp(msg, "reset", 5)){
		log_reset();
		cmdSendResponse("Log reset");
			
	}else if (!strncmp(msg, "state:", 6)){
		int state = (atoi(&msg[6]))&0x03;
		log_setAcquisitionState(state&0x01);
		log_setRecordState(state&0x02);
		cmdSendResponse("");
	}
}

FLASHMEM static void cmd_list (char *msg, const int cmdlen)
{
	cmdSendResponse("heartbeat");
	cmdSendResponse("heartbeat");
	cmdSendResponse("heartbeat");
	cmdSendResponse("");
	cmdSendResponse(" Contents of " TRACKPTS_DIR);

	int ct = cmdListDir(TRACKPTS_DIR);
	printf("<response:msg> %i files<response:end>\n", ct);
	serialFlush();
}

FLASHMEM static void cmd_load (char *filename, const int cmdlen)
{
	if (!validateFilename(filename)){
		cmdSendError("Invalid filename");
	}else{
			
		printf("<response:msg>Loading: %s<response:end>\n", filename);
		serialFlush();
			
		fio_setDir(TRACKPTS_DIR);
		int ct = log_load(filename);
		if (ct)
			printf("<response:msg>%i trackPoints imported from %s<response:end>", ct, filename);

		fio_setDir("/");
	}
}

FLASHMEM static void cmd_reset (char *msg, const int cmdlen)
{
	if (!strncmp(msg, "reset", 5))
		doReboot();
}

FLASHMEM static void cmd_hello (char *msg, const int cmdlen)
{
	cmdSendResponse(msg);
}

FLASHMEM static void cmd_delete (char *filename, const int cmdlen)
{
	// check for special case
	if (strlen(filename) == 1 && filename[0] == '*'){
		// delete all
		cmdSendResponse("Deleting all from " TRACKPTS_DIR);
		// but not actually implemented
			
	}else{
		if (validateFilename(filename)){
			printf("<response:msg>Deleting: %s ...<response:end>\n", filename);
			serialFlush();
				
			fio_setDir(TRACKPTS_DIR);
			if (SD.remove(filename))
				cmdSendResponse("File removed");
			else
				cmdSendResponse("File delete failed");
			fio_setDir("/");
		}else{
			cmdSendError("Invalid filename");
		}
	}
}

FLASHMEM static void cmd_touch (char *filename, const int cmdlen)
{
	if (validateFilename(filename)){
		printf("<response:msg>Touching: %s ...<response:end>\n", filename);
		serialFlush();
			
		dategps_t gdate; timegps_t gtime;
		getDateTime(&gdate, &gtime);
			
		fio_setDir(TRACKPTS_DIR);
		if (fio_setModifyTime(filename, &gdate, &gtime))
			cmdSendResponse("File touched");
		else
			cmdSendResponse("File touch failed");
		fio_setDir("/");
	}else{
		cmdSendError("Invalid filename");
	}
}

FLASHMEM static void cmd_rename (char *msg, const int cmdlen)
{
	char *from = msg;
	char *to = strchr(from, ':');
	if (to){
		*to = 0;
		to++;
			
		if (validateFilename(from) && validateFilename(to)){
			printf("<response:msg>Renaming: %s -> %s ...<response:end>\n", from, to);
			serialFlush();
				
			fio_setDir(TRACKPTS_DIR);
			if (SD.rename(from, to))
				cmdSendResponse("File renamed");
			else
				cmdSendResponse("File rename failed");
			fio_setDir("/");
		}else{
			cmdSendError("Invalid filename");
		}
	}
}

FLASHMEM static void cmd_getfile (char *filename, const int cmdlen)
{
	if (!validateFilename(filename)){
		cmdSendError("Invalid filename");
		return;
	}

	printf(CS("Sending: %s.."), filename);
	serialFlush();
		
	fio_setDir(TRACKPTS_DIR);
	File file = file_open(filename);
	if (file){
		uint32_t length = file.size();
		printf("<response:filename:data>%s:\n", filename);
			
		char buffer[1024];
		for (int i = 0; i < (int)length; i += 1024){
			int bytesRead = file.read(buffer, 1024);
			if (bytesRead > 0){
				Serial.write(buffer, bytesRead);
					
			}else if (bytesRead < 0){
				cmdSendError("File read error");
				break;
			}else{
				break;
			}
		}
		printf("<response:end>\n");
		serialFlush();
			
		file.close();
		fio_setDir("/");
	}else{
		cmdSendError("File open failed");
	}
	fio_setDir("/");
}

FLASHMEM static void cmd_getfileLen (char *filename, const int cmdlen)
{
	if (!validateFilename(filename)){
		cmdSendError("Invalid filename");
		return;
	}

	File file = file_open(filename);
	if (file){
		uint32_t length = file.size();
		file.close();

		printf("<response:filename:length>%s:%u<response:end>\n", filename, (int)length);
	}else{
		cmdSendError("File open failed");
	}
	fio_setDir("/");
}

FLASHMEM static int cmd_uload (char *filename, const int cmdlen)
{
	if (!validateFilename(filename) || !strstr(filename, ".ubx")){
		cmdSendError("Invalid filename");
		return 0;
	}

	printf(CS("Importing %s"), filename);
	
	int ret = 0;
	fio_setDir(TRACKPTS_DIR);
	File file = file_open(filename);
	if (!file){
		cmdSendError("File open failed");
		return 0;

	}else{
		uint32_t length = file.size();
		
		int bytesSent = 0;
		char buffer[512];
		
		for (int i = 0; i < (int)length; i += 512){
			int bytesRead = file.read(buffer, 512);
			if (bytesRead > 0){
				gps_writeUbx(buffer, bytesRead);
				bytesSent += bytesRead;
				delay(1);
			}else if (bytesRead < 0){
				cmdSendError("File read error");
				break;
			}else{
				break;
			}
		}
		file.close();
		printf(CS("%i bytes sent to receiver"), bytesSent);
		ret = bytesSent;

		gps_pollMsg("nav_posllh");
		gps_pollMsg("nav_pvt");
		gps_pollMsg("nav_dop");
		gps_pollMsg("nav_sat");
	}
	fio_setDir("/");
	
	return ret;
}

FLASHMEM static void cmd_sos (char *msg, const int cmdlen)
{
	if (!strncmp(msg, "poll", 4)){
		gps_sosPoll();
		
	}else if (!strncmp(msg, "clear", 5)){
		gps_sosClearFlash();
			
	}else if (!strncmp(msg, "create", 6)){
		gps_sosCreateBackup();
	}
}

FLASHMEM static int cmdExtract (char *buffer, const int cmdlen)
{
	if (buffer[0] != '<') return 1;
	
	char *cmdend = strstr(buffer, CMD_END);
	if (!cmdend) return 1;
	*cmdend = 0;


	if (!strncmp(buffer, CMD_ULOAD, strlen(CMD_ULOAD))){
		char *filename = &buffer[strlen(CMD_ULOAD)];
		cmd_uload(filename, cmdlen);
		
	}else if (!strncmp(buffer, CMD_SOS, strlen(CMD_SOS))){
		char *msg = &buffer[strlen(CMD_SOS)];
		cmd_sos(msg, cmdlen);
		
	}else if (!strncmp(buffer, CMD_FDATA, strlen(CMD_FDATA))){
		char *msg = &buffer[strlen(CMD_FDATA)];
		cmd_fdata(msg, cmdlen);
		
	}else if (!strncmp(buffer, CMD_EXIT, strlen(CMD_EXIT))){
		return 0;

	}else if (!strncmp(buffer, CMD_MAPSCHEME, strlen(CMD_MAPSCHEME))){
		char *msg = &buffer[strlen(CMD_MAPSCHEME)];
		cmd_scheme(msg, cmdlen);
	
	}else if (!strncmp(buffer, CMD_ODO, strlen(CMD_ODO))){
		char *msg = &buffer[strlen(CMD_ODO)];
		cmd_odo(msg, cmdlen);

	}else if (!strncmp(buffer, CMD_RECEIVER, strlen(CMD_RECEIVER))){
		char *msg = &buffer[strlen(CMD_RECEIVER)];
		cmd_receiver(msg, cmdlen);

	}else if (!strncmp(buffer, CMD_DEBUG, strlen(CMD_DEBUG))){

	}else if (!strncmp(buffer, CMD_BRIGHTNESS, strlen(CMD_BRIGHTNESS))){
		char *msg = &buffer[strlen(CMD_BRIGHTNESS)];
		cmd_brightness(msg, cmdlen);

	}else if (!strncmp(buffer, CMD_DETAIL, strlen(CMD_DETAIL))){
		char *msg = &buffer[strlen(CMD_DETAIL)];
		cmd_detail(msg, cmdlen);

	}else if (!strncmp(buffer, CMD_ZOOM, strlen(CMD_ZOOM))){
		char *msg = &buffer[strlen(CMD_ZOOM)];
		cmd_zoom(msg, cmdlen);

	}else if (!strncmp(buffer, CMD_LOGCFG, strlen(CMD_LOGCFG))){
		char *msg = &buffer[strlen(CMD_LOGCFG)];
		cmd_log(msg, cmdlen);
		
	}else if (!strncmp(buffer, CMD_REBOOT, strlen(CMD_REBOOT))){
		char *msg = &buffer[strlen(CMD_REBOOT)];
		cmd_reset(msg, cmdlen);

	}else if (!strncmp(buffer, CMD_HELLO, strlen(CMD_HELLO))){
		char *msg = &buffer[strlen(CMD_HELLO)];
		cmd_hello(msg, cmdlen);
		
	}else if (!strncmp(buffer, CMD_LIST, strlen(CMD_LIST))){
		char *msg = &buffer[strlen(CMD_LIST)];
		cmd_list(msg, cmdlen);

	}else if (!strncmp(buffer, CMD_LOAD, strlen(CMD_LOAD))){
		char *filename = &buffer[strlen(CMD_LOAD)];
		cmd_load(filename, cmdlen);

	}else if (!strncmp(buffer, CMD_DELETE, strlen(CMD_DELETE))){
		char *filename = &buffer[strlen(CMD_DELETE)];
		cmd_delete(filename, cmdlen);

	}else if (!strncmp(buffer, CMD_TOUCH, strlen(CMD_TOUCH))){
		char *filename = &buffer[strlen(CMD_TOUCH)];
		cmd_touch(filename, cmdlen);
		
	}else if (!strncmp(buffer, CMD_RENAME, strlen(CMD_RENAME))){
		char *msg = &buffer[strlen(CMD_RENAME)];
		cmd_rename(msg, cmdlen);

	}else if (!strncmp(buffer, CMD_GETFILE, strlen(CMD_GETFILE))){
		char *filename = &buffer[strlen(CMD_GETFILE)];
		cmd_getfile(filename, cmdlen);

	}else if (!strncmp(buffer, CMD_GETFILELEN, strlen(CMD_GETFILELEN))){
		char *filename = &buffer[strlen(CMD_GETFILELEN)];
		cmd_getfileLen(filename, cmdlen);
	}
	serialFlush();
	
	return 1;
}

int cmdLoadUbx (const char *filename)
{
	return cmd_uload((char*)filename, strlen(filename));
}

FLASHMEM void cmd_init ()
{
	memset(&fileTrans, 0, sizeof(fileTrans));
	
	serialFlush();
	cmdSendResponse("heartbeat");
	fio_setDir(TRACKPTS_DIR);
}

int cmd_task (const int pulse)
{
	static char task_buffer[256];

	if (Serial.dtr() && Serial.available()){
		int len = Serial.readBytesUntil('\n', task_buffer, sizeof(task_buffer)-1);
		if (len > 0){
			task_buffer[len] = 0;
			if (!cmdExtract(task_buffer, len/*strlen(task_buffer)*/)){
				cmdSendResponse("goodbye");
				fio_setDir("/");
				return 0;
			}
		}
	}else if (pulse){
		cmdSendResponse("heartbeat");
	}
	return 1;
}
