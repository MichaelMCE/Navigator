
//  Copyright (c) Michael McElligott
// 
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU LIBRARY GENERAL PUBLIC LICENSE
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU LIBRARY GENERAL PUBLIC LICENSE for details.

// Tabs at 4 spaces

#include <process.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <conio.h>
#include <wininet.h>
#include <math.h>
#include "serial1_console.h"




#define UBLOX_OFFLINE_SERVER_1			1
#define UBLOX_OFFLINE_SERVER_2			2
#define UBLOX_OFFLINE_SERVER_3			3

static const char *ubloxUrl = "http://offline-live%i.services.u-blox.com/GetOfflineData.ashx?token=%s;gnss=gps,glo;alm=gps,glo;period=1;resolution=1";
static const uint32_t baudRates[] = {9600, 9600*2, 9600*4, 9600*6, 115200, 115200*2, 115200*4, 115200*8, 0};

static HANDLE hSerial;
//static int reportMode = 1;
static int oncePerSecond = 0;
static int exitSig = 0;
		


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



void cmd_help (const char *cmdStr);





static struct tm *getTimeReal (double *nanos)
{
	if (nanos){
		struct timespec tp;
		clock_gettime(0, &tp);	// for nanoseconds only
		*nanos = tp.tv_nsec/1000000000.0;
	}

	const __time64_t t = _time64(0);
	//_localtime64_s (struct tm *_Tm, const __time64_t *_Time);
    return _localtime64(&t);
}

static int write_file (char *path, char *buffer, long len)
{
	FILE *file = fopen(path,"wb");
	if (file){
		fwrite(buffer, 1, len, file);
		fclose(file);
		return 1;
	}else{
		return 0;
	}
}

static char *getUrl (const char *url, size_t *totalRead)
{
	
	HINTERNET hOpenUrl;
	HINTERNET hSession = InternetOpen("httpGetFile", INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, 0);
	if (hSession){
		hOpenUrl = InternetOpenUrl(hSession, url, 0, 0, INTERNET_FLAG_IGNORE_CERT_DATE_INVALID|INTERNET_FLAG_RELOAD|INTERNET_FLAG_EXISTING_CONNECT, 0);
		if (!hOpenUrl){
			InternetCloseHandle(hSession);
			return NULL;
		}
	}else{
		return NULL;
	}

	printf("Connected\n");
	
	*totalRead = 0;
	const size_t allocStep = 2048*1024;
	size_t allocSize = allocStep;
	char *buffer = calloc(1, allocSize);
	if (!buffer) return NULL;

	int cycleCt = 5;
	
	if (buffer){
		unsigned long bread = 0;
		int status = 0;

		do {
			status = InternetReadFile(hOpenUrl, &buffer[*totalRead], allocStep, &bread);
			//printf("\nstatus: %i %i %i %i", bread, *totalRead, status, allocSize);

			if (bread > 0 && status == 1){
				
				if (bread != allocStep)
					cycleCt--;
				if (!cycleCt){
					if (buffer) free(buffer);
					buffer = NULL;
					*totalRead = 0;
					break;
				}


				*totalRead += bread;
				allocSize += (bread*2); //allocStep;
				buffer = realloc(buffer, allocSize);
			}else{
				buffer = realloc(buffer, *totalRead);
			}
		}while (buffer && status == 1 && bread > 0);

		InternetCloseHandle(hOpenUrl);
		InternetCloseHandle(hSession);
	}
	return buffer;
}

static inline void bufferDump (const uint8_t *buffer, const int32_t bufferSize)
{
	for (int i = 0; i < bufferSize; i++)
		printf("%02X ", buffer[i]);

	printf("\n");
}

static inline char *memstr (const char *block, const int bsize, const char *pattern)
{
    char *where;
    char *start = (char*)block;
    int found = 0;
    
    while (!found) {
        where = (char*)memchr(start, (int)pattern[0], (size_t)bsize - (size_t)(start - block));
        if (where==NULL){
            found++;
        }else{
			if (!memcmp(where, pattern, strlen(pattern)))
				found++;
        }
        start = where+1;
    }
    return where;
}

static inline void serialClean (HANDLE hserial)
{
	FlushFileBuffers(hserial);

	unsigned long comError = 0;
	COMSTAT comstat = {0};
	PurgeComm(hserial, PURGE_RXCLEAR | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_TXABORT);
	ClearCommError(hserial, &comError, &comstat);
}

static inline void serialClose (HANDLE hserial)
{
    if (hserial != INVALID_HANDLE_VALUE){
    	//FlushFileBuffers(hSerial);
		CloseHandle(hserial);
		hSerial = NULL;
	}
}

int scanForFirstPort ()
{
  	char dev_name[MAX_PATH+8] = "";
    int scanMax = 100;
    int scanMin = 0;
 
	for (int n = scanMin; n < scanMax; n++){
		sprintf(dev_name, "\\\\.\\COM%d", n);
		HANDLE hSerial = CreateFile(dev_name, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		
		if (hSerial != INVALID_HANDLE_VALUE){
			CloseHandle(hSerial);
			return n;
		}
	}
	return 0;
}

int scanForPorts ()
{
	int ct = 0;
  	char dev_name[MAX_PATH+8] = "";
    int scanMax = 100;
    int scanMin = 0;
 
	for (int n = scanMin; n < scanMax; n++){
		sprintf(dev_name, "\\\\.\\COM%d", n);
		HANDLE hSerial = CreateFile(dev_name, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		
		if (hSerial != INVALID_HANDLE_VALUE){
			CloseHandle(hSerial);

			printf("Port: COM %i\n", n);
			ct++;
		}
	}
	return ct;
}


HANDLE serialOpen (const int port, const int baud)
{
 
	HANDLE hSerial = INVALID_HANDLE_VALUE;
	char dev_name[MAX_PATH+8] = "";
	
 	if (port < 1){
	    int scanMax = 40;
	    int scanMin = 0;
 
		for (int n = scanMax; n >= scanMin; --n){
			sprintf(dev_name, "\\\\.\\COM%d", n);
			hSerial = CreateFile(dev_name, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
			if (hSerial != INVALID_HANDLE_VALUE)
				break;
		}
 	}else{
		sprintf(dev_name, "\\\\.\\COM%d", port);
		hSerial = CreateFile(dev_name, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	}

	if (hSerial == INVALID_HANDLE_VALUE)
		return NULL;
	   
	DCB dcb;
 	GetCommState(hSerial, &dcb);
	dcb.fRtsControl = RTS_CONTROL_DISABLE;
	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	dcb.fOutX = dcb.fInX = FALSE;
	dcb.fOutxCtsFlow = FALSE;
	dcb.fOutxDsrFlow = 0;
	dcb.BaudRate = baud;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;
	SetCommState(hSerial, &dcb);

	COMMTIMEOUTS CommTimeouts;
 	GetCommTimeouts(hSerial, &CommTimeouts);
	CommTimeouts.WriteTotalTimeoutConstant = 100;
	CommTimeouts.WriteTotalTimeoutMultiplier = 0;
	
	CommTimeouts.ReadTotalTimeoutConstant = 100;
	CommTimeouts.ReadTotalTimeoutMultiplier = 0;
	CommTimeouts.ReadIntervalTimeout = MAXWORD;
	SetCommTimeouts(hSerial, &CommTimeouts);

	serialClean(hSerial);
 	return hSerial;
}

static inline int serialWrite (HANDLE hserial, const void *buffer, const uint32_t bufferSize, uint32_t *bytesWritten)
{
	int ret = WriteFile(hserial, buffer, bufferSize, (unsigned long*)bytesWritten, NULL);
	//printf("serialWrite %i %i, %X\n", ret, *bytesWritten, GetLastError());
	return ret;
}

static inline int serialRead (HANDLE hserial, void *buffer, const uint32_t bufferSize, uint32_t *bytesRead)
{
	int ret = ReadFile(hserial, buffer, bufferSize, (unsigned long*)bytesRead, NULL);
	//printf("serialRead %i %i, %X\n", ret, *bytesRead, (int)GetLastError());
	return ret;
}

static char *serialReadResponse (HANDLE hserial, char *buffer, size_t bufferSize, uint32_t *len)
{
	*len = 0;
	uint32_t bytesRead = 0;
	uint32_t bytesReadTotal = 0;
	buffer[0] = 0;

	do{
		bytesRead = 0;
		if (!serialRead(hserial, &buffer[bytesReadTotal], bufferSize-bytesReadTotal, &bytesRead))
			break;
			
		bytesReadTotal += bytesRead;
		if (!bytesRead && bytesReadTotal){
			*len = bytesReadTotal;
			return buffer;
		}
	}while(1);

	return NULL;
}

char *serialProcessResponse (char *response, const uint32_t length)
{
	char *responseEnd = memstr(response, length, ":end>\n")+6;
	if (!responseEnd) return NULL;
	
	char *start = response + strncmp(response, "<response:filename:length>", 26);
	if (start == response){
		start += strlen("<response:filename:length>")-1;
		*start = 0;
		start++;
		char *end = memstr(start, length-(start-response), "<response:end>");
		if (end){
			*end = 0;
			
			char *filenameEnd = strchr(start, ':');
			if (filenameEnd){
				*filenameEnd = 0;
				filenameEnd++;
				int32_t length = atoi(filenameEnd);
				printf("File length: %s:%i\n", start, length);
			}
		}
		return responseEnd;
	}
	
	start = response + strncmp(response, "<response:filename:data>", 24);
	if (start == response){
		start += strlen("<response:filename:data>")-1;
		*start = 0;
		start++;
		char *end = memstr(start, length-(start-response), "<response:end>");
		if (end){
			*end = 0;
			char *filenameEnd = strchr(start, ':');
			*filenameEnd = 0;
			filenameEnd++;

			char *filename = start;
			start = filenameEnd+1;
			void *data = start;
			uint32_t len = (end-(char*)data);

			FILE *file = fopen(filename, "wb");
			if (file){
				fwrite(data, 1, len, file);
				fclose(file);
				
				if (strstr(filename, ".tpts"))
					printf("\nFile saved: %s\nLength: %i\ntrackPoints: %i\n\n", filename, len, (int)(len/sizeof(trackPoint_t)));
				else
					printf("\n%i bytes received\nFile saved: %s\n\n", len, filename);
			}else{
				printf("Could not write file to local disk\n");
			}
		}
		return responseEnd;
	}
		
	start = response + strncmp(response, "<response:error>", 16);
	if (start == response){
		start += strlen("<response:error>")-1;
		start++;
		char *end = memstr(start, length-(start-response), "<response:end>");
		*end = 0;
		printf("ERROR: %s\n", start);
	
		return responseEnd;	
	}
	
	start = response + strncmp(response, "<response:msg>", 14);
	if (start == response){
		start += strlen("<response:msg>")-1;
		start++;
		char *end = memstr(start, length-(start-response), "<response:end>");
		*end = 0;
		
		if (!strncmp(start, "goodbye", 7)){
			//reportMode = 2;
			printf("\n\n");

		}else if (strncmp(start, "heartbeat", 9)){
			printf("%s\n", start);
		}

		return responseEnd;
	}

	return responseEnd;
}

static inline int serialSendString (HANDLE hserial, const char *str, const int waitMs)
{
	uint32_t len = strlen(str);
	uint32_t bytesWritten = 0;

	serialWrite(hserial, str, len, &bytesWritten);
	//printf("serialWrite %i\n", ret);
	if (waitMs)
		Sleep(waitMs);
	return (bytesWritten == len);
}

static inline int serialSendCmd (HANDLE hserial, const char *cmd, const char *msg)
{
	char str[strlen(cmd)+strlen(msg)+strlen(CMD_END)+4];
	snprintf(str, sizeof(str), "%s%s%s\n", cmd, msg, CMD_END);
	
	return serialSendString(hserial, str, 20);
}

static inline int serialSendCmdEx (HANDLE hserial, const char *cmd, const char *msg, const int waitMs)
{
	char str[strlen(cmd)+strlen(msg)+strlen(CMD_END)+4];
	snprintf(str, sizeof(str), "%s%s%s\n", cmd, msg, CMD_END);
	
	return serialSendString(hserial, str, waitMs);
}

static int kbWait = 0;
static int reportMode = 1;
	
void setReadResponseState (HANDLE hserial, const int kb_Wait, int report_Mode)
{
	kbWait = kb_Wait;
	reportMode = report_Mode;

	if (!kbWait && !reportMode){
		Sleep(20);
		exitSig = 1;
		serialClose(hSerial);
	}
}

unsigned int __stdcall readThreadEx (void *ptr)
{
	size_t bufferSize = 8*1024*1024;
	char *buffer = calloc(1, bufferSize);
	if (buffer == NULL) return 0;

	HANDLE hserial = hSerial;

	do {
		uint32_t bytesRead = 0;

		char *response = serialReadResponse(hserial, buffer, bufferSize, &bytesRead);
		if (exitSig) break;
		
		if (response == NULL){
			Sleep(10);
			if (kbhit()) break;
			continue;
		}
		
		if (!reportMode){
			if (bytesRead){
				printf("%i\n%s", bytesRead, response);
				memset(buffer, 0, bytesRead);
			}
		}else if (reportMode == 1){
			if (bytesRead < bufferSize){
				char *end = serialProcessResponse(response, bytesRead);
				if (end == NULL) continue;
				if (end - response >= bufferSize) continue;

				if ((response+bytesRead) - end > 0){
					while (end[0] == '<'){
						char *last = serialProcessResponse(end, bytesRead);
						if (last < end) break;
						end = last;
					}
				}
				memset(buffer, 0, bytesRead);
			}
		}else if (reportMode == 2){
			reportMode = 0;
			memset(buffer, 0, bufferSize);
		}
	}while (!kbhit() && kbWait && !exitSig);

	exitSig = 1;
	free(buffer);
	_endthreadex(1);
	return 1;
}

size_t fio_length (FILE *fp)
{
	fpos_t pos;
	
	fgetpos(fp, &pos);
	fseek(fp, 0, SEEK_END);
	size_t fl = ftell(fp);
	fsetpos(fp, &pos);
	
	return fl;
}

void sendFile (HANDLE hSerial, const char *filename)
{
	FILE *file = fopen(filename, "rb");
	if (file){
		size_t length = fio_length(file);
		if (length){
			void *data = calloc(1, length);
			if (data){
				if (fread(data, 1, length, file)){
					char buffer[64];
					printf("Sending %i bytes..\n", (int)length);
										
					snprintf(buffer, sizeof(buffer), "start:%i", (int)length);
					serialSendCmdEx(hSerial, CMD_FDATA, buffer, 5);
					FlushFileBuffers(hSerial);

					uint32_t bytesWritten = 0;
					serialWrite(hSerial, data, length, &bytesWritten);
					printf("bytesWritten %i\n", (int)bytesWritten);
					
					FlushFileBuffers(hSerial);
					printf("complete\n");
					serialSendCmd(hSerial, CMD_FDATA "end:", filename);
				}
				free(data);
			}
		}
		fclose(file);
	}
}

void cmd_hello (const char *cmdStr)
{
	serialSendCmd(hSerial, CMD_HELLO, "heartbeat");
	setReadResponseState(hSerial, 0, 1);
}

void cmd_logcfg (const char *cmdStr)
{
	if (!strncmp("reset", cmdStr, 5) || !strncmp("state:", cmdStr, 6)){
		serialSendCmd(hSerial, CMD_LOGCFG, cmdStr);
		setReadResponseState(hSerial, 0, 1);
	}
}

void cmd_detail (const char *cmdStr)
{
	if (strlen(cmdStr) < 5) return;
	
	if (strchr(cmdStr, ':') && (strchr(cmdStr, '0') || strchr(cmdStr, '1'))){
		printf("Setting %s\n", cmdStr);
		serialSendCmd(hSerial, CMD_DETAIL, cmdStr);
	}
}

void cmd_backlight (const char *cmdStr)
{
	if (strlen(cmdStr) > 6 && !strncmp("level:", cmdStr, 6))
		serialSendCmd(hSerial, CMD_BRIGHTNESS, cmdStr);
}

void cmd_odo (const char *cmdStr)
{
	if (strlen(cmdStr) >= 4){
		if (!strcmp("stop", cmdStr) || !strcmp("start", cmdStr) || !strcmp("reset", cmdStr))
			serialSendCmd(hSerial, CMD_ODO, cmdStr);
	}
}

void cmd_map (const char *cmdStr)
{
	if (strlen(cmdStr) > 5 && !strncmp("zoom:", cmdStr, 5))
		serialSendCmdEx(hSerial, CMD_ZOOM, cmdStr, 20);
	else if (strlen(cmdStr) > 7 && !strncmp("colour:", cmdStr, 7))
		serialSendCmd(hSerial, CMD_MAPSCHEME, cmdStr);
}

void cmd_fsend (const char *cmdStr)
{
	if (strlen(cmdStr) && !strchr(cmdStr, '*')){
		sendFile(hSerial, cmdStr);
		setReadResponseState(hSerial, 0, 1);
	}
}

void cmd_fget (const char *cmdStr)
{
	if (strlen(cmdStr) && !strchr(cmdStr, '*')){
		serialSendCmd(hSerial, CMD_GETFILE, cmdStr);
		setReadResponseState(hSerial, 0, 1);
	}
}

void cmd_fload (const char *cmdStr)
{
	if (strlen(cmdStr) && !strchr(cmdStr, '*')){
		serialSendCmd(hSerial, CMD_LOAD, cmdStr);
		setReadResponseState(hSerial, 0, 1);
	}
}

void cmd_ftouch (const char *cmdStr)
{
	if (strlen(cmdStr) && !strchr(cmdStr, '*')){
		serialSendCmd(hSerial, CMD_TOUCH, cmdStr);
		setReadResponseState(hSerial, 0, 1);
	}
}

void cmd_fdelete (const char *cmdStr)
{
	if (strlen(cmdStr) && !strchr(cmdStr, '*')){
		serialSendCmd(hSerial, CMD_DELETE, cmdStr);
		setReadResponseState(hSerial, 0, 1);
	}
}

void cmd_frename (const char *cmdStr)
{
	if (strlen(cmdStr) >= 3 && strchr(cmdStr, ':')){
		if (!strchr(cmdStr, '*')){
			serialSendCmd(hSerial, CMD_RENAME, cmdStr);
			setReadResponseState(hSerial, 0, 1);
		}
	}
}

void cmd_flength (const char *cmdStr)
{
	if (strlen(cmdStr) >= 1 && !strchr(cmdStr, '*')){
		serialSendCmd(hSerial, CMD_GETFILELEN, cmdStr);
		setReadResponseState(hSerial, 0, 1);
	}
}

void cmd_list (const char *cmdStr)
{
	serialSendCmd(hSerial, CMD_LIST, "");
	setReadResponseState(hSerial, 0, 1);
}

void cmd_disable (const char *cmdStr)
{
	serialSendCmd(hSerial, CMD_EXIT, "");
}

void cmd_reboot (const char *cmdStr)
{
	serialSendCmdEx(hSerial, CMD_REBOOT, "reset", 0);
	printf("Reboot sent\n");
	setReadResponseState(hSerial, 0, 0);
}

void cmd_debug (const char *cmdStr)
{
	if (strlen(cmdStr) > 8 && !strncmp("console:", cmdStr, 8)){
		serialSendCmd(hSerial, CMD_DETAIL, cmdStr);
		
		if (!strncmp("console:1", cmdStr, 9))
			setReadResponseState(hSerial, 1, 1);
		else if (!strncmp("console:2", cmdStr, 9))
			setReadResponseState(hSerial, 1, 0);
	}
}

void cmd_receiver (const char *cmdStr)
{
	if (!strncmp("hotstart", cmdStr, 8)){
		serialSendCmd(hSerial, CMD_RECEIVER, cmdStr);
	}else if (!strncmp("warmstart", cmdStr, 9)){
		serialSendCmd(hSerial, CMD_RECEIVER, cmdStr);
	}else if (!strncmp("coldstart", cmdStr, 9)){
		serialSendCmd(hSerial, CMD_RECEIVER, cmdStr);

	}else if (!strncmp("poll:", cmdStr, 5)){
		serialSendCmd(hSerial, CMD_RECEIVER, cmdStr);
		
	}else if (!strncmp("status", cmdStr, 6)){
		serialSendCmd(hSerial, CMD_RECEIVER, cmdStr);
				
	}else if (!strncmp("version", cmdStr, 7)){
		serialSendCmd(hSerial, CMD_RECEIVER, cmdStr);
	
	}else if (!strncmp("passthrough:", cmdStr, 12)){
		serialSendCmd(hSerial, CMD_RECEIVER, cmdStr);
		printf("Passthrough enabled\n");
		setReadResponseState(hSerial, 0, 0);
		return;
	}

	setReadResponseState(hSerial, 0, 1);
}

void cmd_uload (const char *cmdStr)
{
	if (strlen(cmdStr) && !strchr(cmdStr, '*')){
		serialSendCmd(hSerial, CMD_ULOAD, cmdStr);
		setReadResponseState(hSerial, 1, 1);
	}
}

void cmd_sos (const char *cmdStr)
{
	if (strlen(cmdStr) > 3){
		serialSendCmd(hSerial, CMD_SOS, cmdStr);
		setReadResponseState(hSerial, 0, 1);
	}
}

static int validateMessage (const char *message)
{
	int check = strstr(message, "{")
			 && strstr(message, "message")
			 && strstr(message, "\":\"")
			 && strstr(message, "Invalid token");
	
	if (check) return 1;
	return 0;
}

int formatTimeFilename (char *buffer, const int bufferLen)
{
	const struct tm *date = getTimeReal(NULL);
	return snprintf(buffer, bufferLen, "%.2i%.2i%.4i_%.2i%.2i%.2i.ubx", date->tm_mday, date->tm_mon, date->tm_year+1900, date->tm_hour, date->tm_min, date->tm_sec);
}

void cmd_ufetch (const char *cmdStr)
{
	char buffer[strlen(ubloxUrl) + 1024];

	if (strlen(cmdStr) >= 28 && !strncmp("token:", cmdStr, 6)){
		const char *token = &cmdStr[strlen("token:")];
		char *hasFilename = strchr(token, ':');
		
		if (hasFilename && strlen(hasFilename) > 3){
			*hasFilename = 0;
			hasFilename++;
		}else{
			hasFilename = NULL;
		}
		
		snprintf(buffer, sizeof(buffer), ubloxUrl, UBLOX_OFFLINE_SERVER_1, token);
		printf("Connecting ..\n");

		size_t len = 0;
		char *data = getUrl(buffer, &len);
		if (data){
			printf("Data Retrieved\nLength: %i\n", (int)len);

			if (len < 2048){
				int err = validateMessage(data);
				if (err == 1)
					printf("ERROR: invalid token\n");
				//free(data);
				//return;
			}

			if (hasFilename)
				strcpy(buffer, hasFilename);
			else
				formatTimeFilename(buffer, sizeof(buffer));
			
			if (write_file(buffer, data, len))
				printf("Data saved to: %s\n", buffer);
			else
				printf("unable to write file: %s\n", buffer);
			
			free(data);
		}else{
			const int err = GetLastError();
			if (err == 12029 || err == 12030)
				printf("ERROR: unable to connect to host\n");
			else if (err == 12007)
				printf("ERROR: hostname not resolved\n");
			else if (err == 12002)
				printf("ERROR: request has timed out, try again later\n");
			printf("Data retrieval failed\n");
		}
	}
}

void cmd_runlog (const char *cmdStr)
{
	if (strlen(cmdStr) < 4) return;
		
	//if (!strncmp("start", cmdStr, 5)){
		serialSendCmd(hSerial, CMD_RUNLOG, cmdStr);	
	//}
}

static const cmdstr_t cmdstrs[] = {
	{"hello",    cmd_hello,     ""},
	{"help",     cmd_help,      ""},
	{"debug",    cmd_debug,     "console:0/1/2"},
	{"receiver", cmd_receiver,  "version, hotstart, warmstart, coldstart, poll:ubx_msg"},
	{"log",      cmd_logcfg,    "state:0-3, reset"},
	{"detail",   cmd_detail,    "poi:0/1, world:0/1, slevels:0/1, savailability:0/1, compass:0/1, route:0/1, map:0/1, locgraphic:0/1"},
	{"backlight",cmd_backlight, "level:1-255"},
	{"map",      cmd_map,       "zoom:15-1800, colour:0/1"},
	{"fsend",    cmd_fsend,     "<a filename.tpts>. Send local file (trackPoints) to device"},
	{"fget",     cmd_fget,      "<a filename.tpts>. Retrieve remote file and save locally"},
	{"fload",    cmd_fload,     "<a filename.tpts>. Load trackPts of this file"},
	{"fdelete",  cmd_fdelete,   "<a filename.tpts>. Delete this file."},
	{"frename",  cmd_frename,   "filenameFrom.tpts:filenameTo.tpts"},
	{"flength",  cmd_flength,   "<a filename.tpts>. Return length of file"},
	{"ftouch",   cmd_ftouch,    "<a filename.tpts>. Set modify time to current time"},
	{"list",     cmd_list,      "Display saved data files from /data/"},
	{"disable",  cmd_disable,   "Disable cmd task processing (this) until next power cycle"},
	{"reboot",   cmd_reboot,    "Reboot device"},
	{"odo",      cmd_odo,       "start, stop, reset"},
	{"uload",    cmd_uload,     "filename.ubx. import a .ubx file in to receiver"},
	{"ufetch",   cmd_ufetch,    "token:<yourtoken>. Downloads and saves locally .ubx file of latest offline AssistNow data from u-blox website"},
	{"sos",      cmd_sos,       "create, clear, poll"},
	{"runlog",   cmd_runlog,    "start, stop, pause, reset, trpt:n, step:n"},
	
	{"", NULL, ""}
};

void cmd_help (const char *cmdStr)
{
	
	
	printf("Usage: serial_console.exe Auto/portNumber command subCmd:value\n");
	printf("\n");
	printf("Examples:\n");
	printf(" serial_console.exe auto list\n");
	printf(" serial_console.exe 20 detail poi:0\n");
	printf("\n");
	printf("Commands available:\n");
	
	
	HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO con;
	GetConsoleScreenBufferInfo(hStdout, &con);
	uint16_t oldColour = con.wAttributes;

	
	for (int i = 0; cmdstrs[i].func; i++){
		if (cmdstrs[i].helpStr[0]){
			//printf(" %s - %s\n", cmdstrs[i].cmd, cmdstrs[i].helpStr);

			SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN | FOREGROUND_INTENSITY);
			printf(" %s", cmdstrs[i].cmd);
			SetConsoleTextAttribute(hStdout, oldColour);
			
			GetConsoleScreenBufferInfo(hStdout, &con);
			con.dwCursorPosition.X = 16;
			SetConsoleCursorPosition(hStdout, con.dwCursorPosition);
			
			printf("%s\n", cmdstrs[i].helpStr);
		}
	}
}

int main (const int argc, const char *argv[])
{   

	if (argc < 3){
		if (argc == 2){
			if ((*argv[1] == 'h') || (*argv[1] == 'H')){
				cmd_help("");
				return 0;
			}else{
				if ((*argv[1] == 'a') || (*argv[1] == 'A')){
					if (!scanForPorts())
						printf("No COM ports found\n");
				}
			}
		}else if (argc == 1){
			if (!scanForPorts())
				printf("No COM ports found\n");
		}
		return 0;
	}

	char *cmd = (char*)argv[2];
	int port = atoi(argv[1]);

	if ((*argv[1] == 'a') || (*argv[1] == 'A'))
		port = scanForFirstPort();


	hSerial = serialOpen(port, BAUDRATE(COM_BAUD));
	if (hSerial){
		serialSendCmd(hSerial, CMD_DETAIL, "console:0");
		unsigned int tid = 0;
		HANDLE hReadThread = (HANDLE)_beginthreadex(NULL, 0, readThreadEx, NULL, 0, &tid);
		serialSendCmd(hSerial, CMD_DETAIL, "console:0");
		Sleep(10);
		printf("Port %i:%i\n\n", port, BAUDRATE(COM_BAUD));


		char *cmdStr = "";
		if (argc > 3) cmdStr = (char*)argv[3];

		for (int i = 0; cmdstrs[i].func; i++){
			if (!strcmp(cmd, cmdstrs[i].cmd)){
				cmdstrs[i].func(cmdStr);
				break;
			}
		}

		while(!exitSig && !kbhit()){
			// do something
			Sleep(25);
			if (++oncePerSecond >= 40){
				oncePerSecond = 0;
				// do something
			}
		}

		WaitForSingleObject(hReadThread, INFINITE);
		CloseHandle(hReadThread);

		if (!exitSig)
			serialClose(hSerial);
	}

	return EXIT_SUCCESS;
};

