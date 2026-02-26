


#include "commonGlue.h"


static file_trans_t fileTrans;



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
	printf(CS("%s"), err);
	serialFlush();
}

FLASHMEM void cmdSendMsg (const char *msg)
{
	printf(CS("%s"), msg);
	serialFlush();
}

FLASHMEM void cmdSendResponse (const char *msg)
{
	printf(CS("%s"), msg);
	serialFlush();
}

static File file_open (const char *file)
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
      		printf(CS("  %s"), entry.name());
    	}else{
    		const char *name = entry.name();
    		if (name){
    			uint64_t size = entry.size();

    			printf(CS("  %8lli %s"), size, name);
    		}else{
    			cmdSendError("Could not read directory");
    		}
    		fileCount++;
    	}
    	entry.close();
		serialFlush();
		//delay(1);
  	}
	
	fio_setDir("/");
  	return fileCount;
}

/*
 start:read length
 end:save as filename
*/
FLASHMEM static void cmd_sendfile (char *msg, const int cmdlen)
{
	if (cmdlen < 2) return;
	
	if (!strncmp(msg, "start:", 6)){
		fileTrans.length.expected = atoi(&msg[6]);
		
		if (fileTrans.length.expected < 1 || fileTrans.length.expected > 2*1024*1024){		// sensible limits
			cmdSendError("Invalid length");
			return;
		}
		
		if (fileTrans.pending) extmem_free(fileTrans.pending);
		fileTrans.pending = (char*)extmem_calloc(1, fileTrans.length.expected);
		if (!fileTrans.pending){
			//printf(CS("calloc failed for %i bytes"), fileTrans.length.expected);
			return;
		}

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
	uiLogs_clear();
}

FLASHMEM static void cmd_style (char *msg, const int cmdlen)
{
	if (cmdlen < 2) return;
	
	//if (!strncmp(msg, "colour:", 7)){
		uint8_t colourScheme = atoi(msg)&0xFF;
		sceneSetColourScheme(colourScheme);
	//}
}

FLASHMEM static void cmd_odo (char *msg, const int cmdlen)
{
	if (cmdlen < 2) return;
	
	if (!strncmp(msg, "stop", 4))
		gps_stopOdo();
	else if (!strncmp(msg, "start", 5))
		gps_startOdo();
	else if (!strncmp(msg, "reset", 5))
		gps_resetOdo();
}

FLASHMEM static void cmd_receiver (char *msg, const int cmdlen)
{
	if (cmdlen < 2) return;
	
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
	
	}else if (!strncmp(msg, "savecfg", 7)){
		gps_saveConfig();

	}else if (!strncmp(msg, "rate:", 5)){
		uint8_t rate = atoi(&msg[5]);
		gps_setRate(rate);
		printf(CS("Rate period set to %ims"), rate);
		
	}else if (!strncmp(msg, "baud:", 5)){
		uint32_t baud = atoi(&msg[5]);
		gps_setBaud(baud);

	}else if (!strncmp(msg, "discover", 8)){
		gps_baudDiscover();

	}else if (!strncmp(msg, "reconnect", 9)){
		cmdSendResponse("Reconnecting..");
		gps_reconnect_noConfigure();
		cmdSendResponse("Done");
	
	}else if (!strncmp(msg, "configure", 9)){
		cmdSendResponse("Reconfiguring..");
		gps_reconfigure();
		cmdSendResponse("Done");

	}else if (!strncmp(msg, "poll:", 5)){
		char *pollMsg = &msg[5];
		if (!gps_pollMsg(pollMsg))
			printf(CS("ubx message '%s' not available"), pollMsg);
	
	}else if (!strncmp(msg, "location", 8)){
		gps_printPositionAlt();

	}else if (!strncmp(msg, "time", 4)){
		timegps_t time = log_getLastTime();
		printf(CS("Time: %.02i:%.02i:%.02i"), time.hour, time.min, time.sec);

	}else if (!strncmp(msg, "date", 4)){
		dategps_t date = log_getLastDate();
		printf(CS("Date: %.02i.%.02i.%i"), date.day, date.month, date.year-2000);

	}else if (!strncmp(msg, "itow", 4)){
		int32_t iTow = log_getLastiTow();
		printf(CS("iTow: %ul"), (unsigned int)iTow);

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
		uint8_t which = atoi(&msg[12]);
		if (which == 1){
			gnssReceiver_PassthroughEnabled = 1;
			
		}else if (which == 2){
			gnssReceiver_PassthroughEnabled = 2;
		}
	}
}

FLASHMEM static void cmd_backlight (char *msg, const int cmdlen)
{
	if (cmdlen < 2) return;
	
	uint8_t level = atoi(msg)&0xFF;
	if (level) tft_setBacklight(level);
}

FLASHMEM static void cmd_zoom (char *msg, const int cmdlen)
{
	if (cmdlen < 2) return;
	
	const float zoomlevel = atof(msg);
	if (zoomlevel < 5.0f || zoomlevel > 10000.0f)
		return;

	sceneSetZoom(&inst, zoomlevel);
	sceneResetViewport(&inst);
	sceneLoadTiles(&inst);
	render_signalTiles();
	render_signalUpdate();
}

FLASHMEM static void cmd_page (char *msg, const int cmdlen)
{
	if (cmdlen < 2) return;
	
	page_set(atoi(msg)&0x0F);
}
		
FLASHMEM static void cmd_detail (char *msg, const int cmdlen)
{
	if (cmdlen < 2) return;
	
	if (!strncmp(msg, "poi:", 4)){
		map_setDetail(MAP_RENDER_POI, atoi(&msg[4])&0x01);

	}else if (!strncmp(msg, "map:", 4)){	
		map_setDetail(MAP_RENDER_VIEWPORT, atoi(&msg[4])&0x01);

	}else if (!strncmp(msg, "path:", 5)){
		inst.scheme.pathThickness = atoi(&msg[5])&0xFF;

	}else if (!strncmp(msg, "spot:", 5)){
		inst.scheme.spotRadius = atoi(&msg[5])&0xFF;

	}else if (!strncmp(msg, "page:", 5)){
		page_set(atoi(&msg[5]));
			
	}else if (!strncmp(msg, "world:", 6)){
		map_setDetail(MAP_RENDER_SWORLD, atoi(&msg[6])&0x01);
	
	}else if (!strncmp(msg, "route:", 6)){
		map_setDetail(MAP_RENDER_TRACKPOINTS, atoi(&msg[6])&0x01);
			
	}else if (!strncmp(msg, "compass:", 8)){	
		map_setDetail(MAP_RENDER_COMPASS, atoi(&msg[8])&0x01);
			
	}else if (!strncmp(msg, "slevels:", 8)){
		map_setDetail(MAP_RENDER_SLEVELS, atoi(&msg[8])&0x01);

	}else if (!strncmp(msg, "tilesClean", 10)){
		tilesUnloadAll(&inst);
		render_signalTiles();
		render_signalUpdate();

	}else if (!strncmp(msg, "locgraphic:", 11)){	
		map_setDetail(MAP_RENDER_LOCGRAPTHIC, atoi(&msg[11])&0x01);

	}else if (!strncmp(msg, "loadTilesAll", 12)){
		sceneLoadTilesComplete(&inst);
		render_signalUpdate();

	}else if (!strncmp(msg, "savailability:", 14)){
		map_setDetail(MAP_RENDER_SAVAIL, atoi(&msg[14])&0x01);
	}
	
	render_signalUpdate();
}

FLASHMEM static void cmd_list (char *msg, const int cmdlen)
{
	cmdSendResponse(" ");
	cmdSendResponse(" Contents of " TRACKPTS_DIR);

	int ct = cmdListDir(TRACKPTS_DIR);
	printf(CS(" %i files"), ct);
	serialFlush();
}

FLASHMEM static void loadRoute (const char *filename)
{
	printf(CS("Loading: %s"), filename);
	serialFlush();
			
	fio_setDir(TRACKPTS_DIR);
	int ct = log_load(filename);
	if (ct)
		printf(CS("%i trackPoints imported from %s"), ct, filename);

	fio_setDir("/");
}

FLASHMEM static void cmd_load (char *filename, const int cmdlen)
{
	if (cmdlen < 3) return;
	
	if (!validateFilename(filename))
		cmdSendError("Invalid filename");
	else
		loadRoute(filename);
}

FLASHMEM static void cmd_log (char *msg, const int cmdlen)
{
	if (cmdlen < 2) return;
	
	if (!strncmp(msg, "reset", 5)){
		log_reset();
		cmdSendResponse("Log reset");
	
	}else if (!strncmp(msg, "load:", 5)){
		char *filename = &msg[5];
		loadRoute(filename);

	}else if (!strncmp(msg, "state:", 6)){
		int state = (atoi(&msg[6]))&0x03;
		log_setAcquisitionState(state&0x01);
		log_setRecordState(state&0x02);
		cmdSendResponse("");
	}
}

FLASHMEM static void cmd_reset (char *msg, const int cmdlen)
{
	mpu_reboot();
}

FLASHMEM static void cmd_hello (char *msg, const int cmdlen)
{
	cmdSendResponse(msg);
}

FLASHMEM static void cmd_delete (char *filename, const int cmdlen)
{
	if (cmdlen < 2) return;
	
	// check for special case
	if (strlen(filename) == 1 && filename[0] == '*'){
		// delete all
		cmdSendResponse("Deleting all from " TRACKPTS_DIR);
		// but not actually implemented
			
	}else{
		if (validateFilename(filename)){
			printf(CS(" Deleting: %s ..."), filename);
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
	if (cmdlen < 2) return;
	
	if (validateFilename(filename)){
		printf(CS("Touching: %s ..."), filename);
		serialFlush();
			
		dategps_t gdate;
		timegps_t gtime;
		date_getAdjustedTime(NULL, &gdate, &gtime);
			
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
	if (cmdlen < 2) return;
	
	char *from = msg;
	char *to = strchr(from, ':');
	if (to){
		*to = 0;
		to++;
			
		if (validateFilename(from) && validateFilename(to)){
			printf(CS("Renaming: %s -> %s ..."), from, to);
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

FLASHMEM static void cmd_getmeta (char *filename, const int cmdlen)
{
	if (cmdlen < 2) return;

	if (!validateFilename(filename)){
		cmdSendError("Invalid filename");
		return;
	}

	fio_setDir(TRACKPTS_DIR);
	File file = file_open(filename);
	if (file){
		DateTimeFields create;
		DateTimeFields modify;
		int createDateValid = file.getCreateTime(create);
		int modifyDateValid = file.getModifyTime(modify);
		
		uint32_t length = file.size();
		file.close();

		cmdSendMsg(filename);
		printf(CS(" Len: %i"), (int)length);
		if (createDateValid){
			printf(CS(" Create date: %.2i %.2i %.4i"), create.mday, create.mon+1, create.year+1900);
			printf(CS("        time: %.2i:%.2i:%.2i"), create.hour, create.min, create.sec);
		}
		if (modifyDateValid){
			printf(CS(" Modify date: %.2i %.2i %.4i"), modify.mday, modify.mon+1, modify.year+1900);
			printf(CS("        time: %.2i:%.2i:%.2i"), modify.hour, modify.min, modify.sec);
		}

	}else{
		cmdSendError("File open failed");
	}
	fio_setDir("/");
}

FLASHMEM static void cmd_getfile (char *filename, const int cmdlen)
{
	cmd_fileMeta_t fileMeta;
	memset(&fileMeta, 0, sizeof(cmd_fileMeta_t));
	
	if (cmdlen < 2){
		fileMeta.length = 1;	// no filename
		Serial.write((char*)&fileMeta, sizeof(cmd_fileMeta_t));
		return;
	}

	if (!validateFilename(filename)){
		fileMeta.length = 2;		// invalid filename
		Serial.write((char*)&fileMeta, sizeof(cmd_fileMeta_t));
		return;
	}

	fio_setDir(TRACKPTS_DIR);
	File file = file_open(filename);
	if (file){
		DateTimeFields date;
		int dateValid = file.getCreateTime(date);
		if (!dateValid)
			dateValid = file.getModifyTime(date);
		
		uint32_t length = file.size();

		fileMeta.sec = date.sec;
		fileMeta.min = date.min;
		fileMeta.hour = date.hour;
		fileMeta.wday = date.wday;
		fileMeta.mday = date.mday;
		fileMeta.mon = date.mon+1;
		fileMeta.year = date.year+1900;
		fileMeta.length = length;

		Serial.write((char*)&fileMeta, sizeof(cmd_fileMeta_t));
		serialFlush();
		delay(20);
		
		char buffer[1024];
		for (int i = 0; i < (int)length; i += 1024){
			int bytesRead = file.read(buffer, 1024);
			if (bytesRead > 0){
				Serial.write(buffer, bytesRead);
					
			}else if (bytesRead < 0){
				//cmdSendError("File read error");
				break;
			}else{
				break;
			}
		}
		
		serialFlush();
		file.close();
		
	}else{
		fileMeta.length = 3;	// file open failed
		Serial.write((char*)&fileMeta, sizeof(cmd_fileMeta_t));
	}
	fio_setDir("/");
}

FLASHMEM static int cmd_uloadRet (char *filename, const int cmdlen)
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

FLASHMEM static void cmd_uload (char *msg, const int cmdlen)
{
	if (cmdlen < 3) return;
	
	cmd_uloadRet(msg, cmdlen);
}

FLASHMEM static void cmd_sos (char *msg, const int cmdlen)
{
	if (cmdlen < 2) return;
	
	if (!strncmp(msg, "poll", 4)){
		gps_sosPoll();
		
	}else if (!strncmp(msg, "clear", 5)){
		gps_sosClearFlash();
			
	}else if (!strncmp(msg, "create", 6)){
		gps_sosCreateBackup();
	}
}

FLASHMEM static void cmd_runlog (char *msg, const int cmdlen)
{
	if (cmdlen < 3) return;
	
	if (!strncmp(msg, "stop", 4)){
		log_runStop();
		
	}else if (!strncmp(msg, "start", 5)){
		log_runStart();

	}else if (!strncmp(msg, "reset", 5)){
		log_runReset();

	}else if (!strncmp(msg, "pause", 5)){
		log_runPause();
	
	}else if (!strncmp(msg, "step:", 5)){
		uint8_t step = atoi(&msg[5])&0xFF;
		log_runStep(step);
	
	}else if (!strncmp(msg, "tkpt:", 5)){
		uint32_t trkPt = atoi(&msg[5]);
		log_runSet(trkPt);
	}
	render_signalUpdate();
}

FLASHMEM static void cmd_mpu (char *msg, const int cmdlen)
{
	if (cmdlen < 3) return;
	
	if (!strncmp(msg, "freq:", 5)){
		const uint32_t freq = atoi(&msg[5]);
		if (freq >= 24 && freq <= 960){
			mpu_setClockFreq(freq);
			delay(1);
		}
		printf(CS("Clock frequency: %u"), (unsigned int)F_CPU_ACTUAL);

	}else if (!strncmp(msg, "status", 6)){
		extern uint8_t external_psram_size;
		
		cmdSendResponse(CFG_STRING);
		printf(CS("SDCard size: %iGB"), SDCARD_SIZE);
		printf(CS("Clock frequency: %uMhz"), (unsigned int)F_CPU_ACTUAL/1000/1000);
		printf(CS("CPU temp: %.2fc"), InternalTemperature.readTemperatureC());
		printf(CS("ExtMem: %uMB"), external_psram_size);
		printf(CS("Tiles: %i"), tilesCount());
		printf(CS("Blocks: %i"), blocksCount());
		printf(CS("Tile memory used: %u"), tileMemoryUsage());

		printf("GNSS Receiver: ");
		if (RECEIVER_SINGLE == 1)
			printf("Single");
		else
			printf("Dual");
			
		if (RECEIVER_M10 == 1)
			printf(" UBlox M10\n");
		else
			printf(" UBlox M8\n");

	}else if (!strncmp(msg, "powersave:on", 12)){
		powersaveEnableForce();

	}else if (!strncmp(msg, "powersave:off", 13)){
		//drawPanel(1);
		powersaveDisable();
	}
}

FLASHMEM static void cmd_map (char *msg, const int cmdlen)
{
	cmdSendResponse("map: add me");
}

FLASHMEM static void cmd_debug (char *msg, const int cmdlen)
{
	if (strlen(msg) < 3) return;
	
	if (!strncmp(msg, "console:1", 9))
		map_setDetail(MAP_RENDER_CONSOLE, 1);
	else if (!strncmp(msg, "console:0", 9))
		map_setDetail(MAP_RENDER_CONSOLE, 0);
}

FLASHMEM static void cmd_tiles (char *msg, const int cmdlen)
{
	if (cmdlen < 3) return;
	
	if (!strncmp("load", msg, 4)){
		sceneLoadTilesComplete(&inst);
		render_signalUpdate();
		
	}else if (!strncmp("flush", msg, 5)){
		tilesUnloadAll(&inst);
		render_signalTiles();
		render_signalUpdate();
	}
}

FLASHMEM static void cmd_help (char *msg, const int cmdlen);

static const cmdstr_t cmdstrs[] = {
	{"/hello",    cmd_hello,     "Greatings"},
	{"/help",     cmd_help,      "This"},
	{"/debug",    cmd_debug,     "console:0/1/2"},
	{"/receiver", cmd_receiver,  "version, hotstart, warmstart, coldstart, poll:ubx_msg"},
	{"/log",      cmd_log,       "state:0-3, reset"},
	{"/detail",   cmd_detail,    "poi:0/1, world:0/1, slevels:0/1, savailability:0/1, compass:0/1, route:0/1, map:0/1, locgraphic:0/1"},
	{"/backlight",cmd_backlight, "level:1-255"},
	{"/map",      cmd_map,       "zoom:15-1800, colour:0/1"},
	{"/reboot",   cmd_reset,     "Reboot device"},
	{"/getfile",  cmd_getfile,   "send file to client"},
	{"/sendfile", cmd_sendfile,  "<a filename.tpts>. Send a local .tpts file to device"},
	{"/load",     cmd_load,      "<a filename.tpts>. Load trackPts of this file"},
	{"/delete",   cmd_delete,    "<a filename.tpts>. Delete this file."},
	{"/rename",   cmd_rename,    "filenameFrom.tpts:filenameTo.tpts"},
	{"/filemeta", cmd_getmeta,   "<a filename.tpts>. Return length, create, modify date and time"},
	{"/touch",    cmd_touch,     "<a filename.tpts>. Set modify time to current time"},
	{"/list",     cmd_list,      "Display saved data files from /data/"},
	{"/odo",      cmd_odo,       "start, stop, reset"},
	{"/ubxload",  cmd_uload,     "filename.ubx. import a .ubx data file from SDcard to receiver"},
	{"/sos",      cmd_sos,       "create, clear, poll"},
	{"/tiles",    cmd_tiles,     "flush, load"},
	{"/runlog",   cmd_runlog,    "start, stop, pause, reset, trpt:n, step:n"},
	{"/mpu",      cmd_mpu,       "reboot, freq:mhz, powersave:on/off. Set microcontroller frequency"},
	{"/page",     cmd_page,      "Switch page (1, 2, etc..)"},
	{"/zoom",     cmd_zoom,      "Set mp zoom level"},
	{"/style",    cmd_style,     "Set map rendering style (profile)"},
	
	{"", NULL, ""}
};

FLASHMEM static void cmd_help (char *msg, const int cmdlen)
{
	printf(CS("Usage: /command subCmd:value"));
	printf(CS(" "));
	printf(CS("Examples: "));
	printf(CS(" /list"));
	printf(CS(" /detail poi:0"));
	printf(CS(" "));
	printf(CS("Commands available:"));
	
	for (int i = 0; cmdstrs[i].func; i++){
		if (cmdstrs[i].helpStr[0])
			printf(CS(" %s - %s"), cmdstrs[i].cmd, cmdstrs[i].helpStr);
	}
}

FLASHMEM static int cmdExtractCmd (char *cmd, const int cmdlen)
{
	if (cmd[0] != '/') return 1;
	
	char *cmdend = strstr(cmd, "\n");
	if (cmdend) *cmdend = 0;
		
	for (int i = 0; cmdstrs[i].func; i++){
		if (!strncmp(cmd, cmdstrs[i].cmd, strlen(cmdstrs[i].cmd))){
			char *msg = &cmd[strlen(cmdstrs[i].cmd)];
			if (*msg == ' ') msg++;
			cmdstrs[i].func(msg, cmdlen);
			serialFlush();
			return 1;
		}
	}
	return 1;
}

int cmdLoadUbx (const char *filename)
{
	return cmd_uloadRet((char*)filename, strlen(filename));
}

FLASHMEM void cmd_init ()
{
	memset(&fileTrans, 0, sizeof(fileTrans));
	
	serialFlush();
	cmdSendResponse("cmd_init");
	fio_setDir(TRACKPTS_DIR);
}

int cmd_task (const int pulse)
{
	static char cmdbuffer[128];
	static int pos = 0;
	

	if (Serial.dtr() && Serial.available()){
		if (pos >= (int)sizeof(cmdbuffer)) pos = 0;
		
		int len = Serial.readBytes(cmdbuffer+pos, sizeof(cmdbuffer)-pos);
		if (len > 0){
			pos += len;
			if (pos >= (int)sizeof(cmdbuffer)){
				pos = 0;
				memset(cmdbuffer, 0, sizeof(cmdbuffer));
				return 1;
			}
			cmdbuffer[pos] = 0;
			void *cmdstart = memchr(cmdbuffer, '/', pos);
			if (cmdstart){
				if (memchr(cmdstart, '\n', pos)){
					powersaveDisable();
					if (!cmdExtractCmd((char*)cmdstart, pos)){
						cmdSendResponse("goodbye");
						fio_setDir("/");
						pos = 0;
						return 1;
					}
					pos = 0;
					memset(cmdbuffer, 0, sizeof(cmdbuffer));
				}
			}
		}
	}
	return 1;
}
