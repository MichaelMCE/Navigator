
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
#include <math.h>
#include "console.h"




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

static const uint32_t baudRates[] = {9600, 9600*2, 9600*4, 9600*6, 115200, 115200*2, 115200*4, 115200*8, 0};
#define BAUDRATE(n)				(baudRates[(n)])


static HANDLE hSerial = NULL;




void cmd_help (const char *str)
{
}

static struct tm *getTimeReal (double *nanos)
{
	if (nanos){
		struct timespec tp;
		clock_gettime(0, &tp);	// for nanoseconds only
		*nanos = tp.tv_nsec/1000000000.0;
	}

	const __time64_t t = _time64(0);
    return _localtime64(&t);
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


static HANDLE serialOpen (const int port, const int baud)
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
	CommTimeouts.WriteTotalTimeoutConstant = 1500;
	CommTimeouts.WriteTotalTimeoutMultiplier = 1;
	
	CommTimeouts.ReadTotalTimeoutConstant = 1500;
	CommTimeouts.ReadTotalTimeoutMultiplier = 1;
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
	//printf("serialRead: %i %i, %X\n", ret, *bytesRead, (int)GetLastError());
	return ret;
}

static uint32_t serialSendString (HANDLE hserial, const char *str, const int waitMs)
{
	uint32_t len = strlen(str);
	uint32_t bytesWritten = 0;

	serialWrite(hserial, str, len, &bytesWritten);
	if (waitMs)
		Sleep(waitMs);
	return (bytesWritten == len);
}

int formatTimeFilename (char *buffer, const int bufferLen)
{
	const struct tm *date = getTimeReal(NULL);
	return snprintf(buffer, bufferLen, "%.2i%.2i%.4i_%.2i%.2i%.2i.ubx", date->tm_mday, date->tm_mon, date->tm_year+1900, date->tm_hour, date->tm_min, date->tm_sec);
}

static size_t fio_length (FILE *fp)
{
	fpos_t pos;
	
	fgetpos(fp, &pos);
	fseek(fp, 0, SEEK_END);
	size_t fl = ftell(fp);
	fsetpos(fp, &pos);
	
	return fl;
}

void cmd_upload (const char *filename)
{
	FILE *file = fopen(filename, "rb");
	if (file){
		size_t length = fio_length(file);
		if (length){
			void *data = calloc(1, length);
			if (data){
				if (fread(data, 1, length, file)){
					char buffer[256];
					printf("Sending %s (%i bytes)...\n", filename, (int)length);
	
					snprintf(buffer, sizeof(buffer), "%s start:%i\n", CMD_SENDFILE, (int)length);
					serialSendString(hSerial, buffer, 1500);
					FlushFileBuffers(hSerial);

					uint32_t bytesWritten = 0;
					serialWrite(hSerial, data, length, &bytesWritten);
					printf("BytesWritten: %i\n", (int)bytesWritten);
					
					FlushFileBuffers(hSerial);
					printf("File sent\n");
					snprintf(buffer, sizeof(buffer), "%s end:%s\n", CMD_SENDFILE, filename);
					serialSendString(hSerial, buffer, 100);
				}
				free(data);
			}
		}
		fclose(file);
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

	int port = atoi(argv[1]);
	if ((*argv[1] == 'a') || (*argv[1] == 'A'))
		port = scanForFirstPort();

	hSerial = serialOpen(port, BAUDRATE(COM_BAUD));
	if (hSerial){
		printf("Port %i:%i\n\n", port, BAUDRATE(COM_BAUD));

		cmd_upload(argv[2]);
		serialClose(hSerial);
	}

	return EXIT_SUCCESS;
};

