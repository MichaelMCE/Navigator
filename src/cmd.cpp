


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





static int trkPtsRead = 0;
static int trkPtPreLoad = 0;
//EXTMEM static trackPoint_t trkPts[TRACKPTS_MAX/2];
static trackPoint_t *trkPts = NULL;



// <cmd:list><cmd:end>											// list everything in data/
// <cmd:delete>a filename.ext<cmd:end>							// remove file within data/
// <cmd:delete>*<cmd:end>										// delete all files within data/
// <cmd:rename>a filename from.ext:a filename to.ext<cmd:end>	// rename a file from:to
// <cmd:getfiledata>a filename.ext<cmd:end>						// restrive file. respond with: <response:data>bin data<response:end>
// <cmd:getfilelength>a filename.ext<cmd:end>					// restrive length of file. respond with: <response:filename,length>This is a filename.ext:123456<response:end>
// error: <response:error>an error message<response:end>
// msg: <response:msg><response:end>


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

void cmdSendError (const char *err)
{
	printf("<response:error>%s<response:end>\n", err);
	serialFlush();
}

void cmdSendResponse (const char *msg)
{
	printf("<response:msg>%s<response:end>\n", msg);
	serialFlush();
}

static inline File file_open (const char *file)
{
	fio_setDir(TRACKPTS_DIR);
	return SD.open(file);
}

static inline int cmdListDir (const char *dir)
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

static inline int cmdExtract (char *buffer, const int cmdlen)
{
	if (buffer[0] != '<')
		return 1;
	
	char *cmdend = strstr(buffer, CMD_END);
	if (!cmdend) return 1;
	*cmdend = 0;

	if (!strncmp(buffer, CMD_TRKPT, strlen(CMD_TRKPT))){
		char *msg = &buffer[strlen(CMD_TRKPT)];
		if (!strncmp(msg, "start:", 6)){
			trkPtPreLoad = atoi(&msg[6]);
			
			if (trkPtPreLoad < 1 || trkPtPreLoad > TRACKPTS_MAX){
				cmdSendError("Invalid trackPoint total");
				return 1;
			}
			
			if (trkPts) extmem_free(trkPts);
			trkPts = (trackPoint_t*)extmem_calloc(1, trkPtPreLoad*sizeof(trackPoint_t));
			
			trackPoint_t tp;
			trkPtsRead = 0;
			
			for (int i = 0; i < trkPtPreLoad; i++){
				int ct = Serial.readBytes((char*)&tp, (size_t)sizeof(tp));
				if (ct != (int)sizeof(tp)) break;
				
	 			trkPts[trkPtsRead++] = tp;
	 		}
			printf(CS("%i trackPoints of %i received"), trkPtsRead, trkPtPreLoad);
			
		}else if (!strncmp(msg, "end:", 4)){
			const char *filename = &msg[4];

			if (trkPtsRead < 1 || !trkPts){
				cmdSendError("No trackPoints to save");
				return 1;
			}

			if (!validateFilename(filename)){
				cmdSendError("Invalid filename");
			}else{

				fio_setDir(TRACKPTS_DIR);
				File file = SD.open(filename, FILE_WRITE);
				if (file){
					uint64_t pos = 0;
					file.seek(pos, SEEK_SET);
					
					int length = trkPtsRead * sizeof(trackPoint_t);
					int written = file.write(trkPts, length);
					file.close();
					
					if (written != length){
						cmdSendError("Write Failed");
					}else{
						printf(CS("Saved %i trackPoints to %s"), trkPtsRead, filename);
					}
				}
				fio_setDir("/");
				
				if (trkPts) extmem_free(trkPts);
				trkPts = NULL;
			}
		}
	}else if (!strncmp(buffer, CMD_EXIT, strlen(CMD_EXIT))){
		return 0;

	}else if (!strncmp(buffer, CMD_MAPSCHEME, strlen(CMD_MAPSCHEME))){
		char *msg = &buffer[strlen(CMD_MAPSCHEME)];
		if (!strncmp(msg, "colour:", 7)){
			uint8_t colourScheme = atoi(&msg[7])&0xFF;
			sceneSetColourScheme(colourScheme);
		}		
	}else if (!strncmp(buffer, CMD_BRIGHTNESS, strlen(CMD_BRIGHTNESS))){
		char *msg = &buffer[strlen(CMD_BRIGHTNESS)];
		if (!strncmp(msg, "level:", 6)){
			uint8_t level = atoi(&msg[6])&0xFF;
			if (level) tft_backlight(level);
		}
		
	}else if (!strncmp(buffer, CMD_RDETAIL, strlen(CMD_RDETAIL))){
		char *msg = &buffer[strlen(CMD_RDETAIL)];
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
	}else if (!strncmp(buffer, CMD_ZOOM, strlen(CMD_ZOOM))){
		char *msg = &buffer[strlen(CMD_ZOOM)];
		if (!strncmp(msg, "zoom:", 5)){
			float zoomlevel = atof(&msg[5]);

			sceneSetZoom(&inst, zoomlevel);
			sceneResetViewport(&inst);
			//sceneLoadTiles(&inst);
		}
	}else if (!strncmp(buffer, CMD_LOGCFG, strlen(CMD_LOGCFG))){
		char *msg = &buffer[strlen(CMD_LOGCFG)];
		
		if (!strncmp(msg, "reset", 5)){
			log_reset();
			cmdSendResponse("Log reset");
			
		}else if (!strncmp(msg, "state:", 6)){
			int state = (atoi(&msg[6]))&0x03;
			log_setAcquisitionState(state&0x01);
			log_setRecordState(state&0x02);
		}
		
	}else if (!strncmp(buffer, CMD_REBOOT, strlen(CMD_REBOOT))){
		char *msg = &buffer[strlen(CMD_REBOOT)];
		if (!strncmp(msg, "reset", 5))
			doReboot();

	}else if (!strncmp(buffer, CMD_HELLO, strlen(CMD_HELLO))){
		char *msg = &buffer[strlen(CMD_HELLO)];
		cmdSendResponse(msg);
		
	}else if (!strncmp(buffer, CMD_LIST, strlen(CMD_LIST))){
		cmdSendResponse("heartbeat");
		cmdSendResponse("heartbeat");
		cmdSendResponse("heartbeat");
		cmdSendResponse("");
		cmdSendResponse(" Contents of " TRACKPTS_DIR);

		int ct = cmdListDir(TRACKPTS_DIR);
		printf("<response:msg> %i files<response:end>\n", ct);
		serialFlush();

	}else if (!strncmp(buffer, CMD_LOAD, strlen(CMD_LOAD))){
		char *filename = &buffer[strlen(CMD_LOAD)];
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
	}else if (!strncmp(buffer, CMD_DELETE, strlen(CMD_DELETE))){
		char *filename = &buffer[strlen(CMD_DELETE)];
		
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
	}else if (!strncmp(buffer, CMD_TOUCH, strlen(CMD_TOUCH))){
		char *filename = &buffer[strlen(CMD_TOUCH)];
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
	}else if (!strncmp(buffer, CMD_RENAME, strlen(CMD_RENAME))){
		char *from = &buffer[strlen(CMD_RENAME)];
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
	}else if (!strncmp(buffer, CMD_GETFILE, strlen(CMD_GETFILE))){
		char *filename = &buffer[strlen(CMD_GETFILE)];
		
		if (!validateFilename(filename)){
			cmdSendError("Invalid filename");
			return 1;
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
			//free(buffer);
			//cmdSendResponse("Data sent");
			
			file.close();
			fio_setDir("/");
		}else{
			cmdSendError("File open failed");
		}
		fio_setDir("/");
		
	}else if (!strncmp(buffer, CMD_GETFILELEN, strlen(CMD_GETFILELEN))){
		char *filename = &buffer[strlen(CMD_GETFILELEN)];
		
		if (!validateFilename(filename)){
			cmdSendError("Invalid filename");
			return 1;
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
	serialFlush();
	
	return 1;
}

void cmd_init ()
{
	trkPts = NULL;
	
	serialFlush();
	cmdSendResponse("heartbeat");
	fio_setDir(TRACKPTS_DIR);

	cmdSendResponse("SatNav Mk2: cmd mode");
	cmdSendResponse("\n");
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
