
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

static int write_file (const char *path, char *buffer, long len)
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
	CommTimeouts.WriteTotalTimeoutConstant = 100;
	CommTimeouts.WriteTotalTimeoutMultiplier = 0;
	
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

void cmd_download (const char *str)
{
	printf("Downloading: %s...\n", str);
	
	cmd_fileMeta_t fileMeta;
	memset(&fileMeta, 0, sizeof(cmd_fileMeta_t));
	
	char buffer[1024];
	memset(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%s %s\n", CMD_GETMETABIN, str);

	if (serialSendString(hSerial, buffer, 20)){
		uint32_t bytesRead = 0;
		serialRead(hSerial, &fileMeta, sizeof(fileMeta), &bytesRead);

		if (fileMeta.length > 12 && fileMeta.length < 10*1024*1024){
			char *filedata = calloc(1, fileMeta.length);
			if (!filedata) abort();

			serialRead(hSerial, filedata, fileMeta.length, &bytesRead);
			
			if (bytesRead == fileMeta.length){
				if (write_file(str, filedata, fileMeta.length))
					printf("Complete\n%i bytes written to %s\n", fileMeta.length, str);
				else
					printf("Write file failed: %s\n", str);
			}else{
				printf("File length mismatch\nExpected: %i, but received: %i bytes\n", fileMeta.length, bytesRead);
			}
			
			free(filedata);
			return;
		}else{
			printf("Invalid file length received\n");
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

	int port = atoi(argv[1]);
	if ((*argv[1] == 'a') || (*argv[1] == 'A'))
		port = scanForFirstPort();

	hSerial = serialOpen(port, BAUDRATE(COM_BAUD));
	if (hSerial){
		printf("Port %i:%i\n\n", port, BAUDRATE(COM_BAUD));

		cmd_download(argv[2]);
		serialClose(hSerial);
	}

	return EXIT_SUCCESS;
};

