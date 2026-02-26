
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


#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <windows.h>
#include <wininet.h>




#define UBLOX_OFFLINE_SERVER_1			1
#define UBLOX_OFFLINE_SERVER_2			2
#define UBLOX_OFFLINE_SERVER_3			3

//static const char *ubloxUrl = "http://offline-live%i.services.u-blox.com/GetOfflineData.ashx?token=%s;gnss=gps,glo,gal,bds;alm=gps,glo,gal,bds;period=1;resolution=1";
static const char *ubloxUrl = "http://offline-live%i.services.u-blox.com/GetOfflineData.ashx?token=%s;gnss=gps,gal,bds;alm=gps,gal,bds;period=1;resolution=1";





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

static int validateMessage (const char *message)
{
	int check = strstr(message, "{")
			 && strstr(message, "message")
			 && strstr(message, "\":\"")
			 && strstr(message, "Invalid token");
	
	if (check) return 1;
	return 0;
}


static int formatTimeFilename (char *buffer, const int bufferLen)
{
	const struct tm *date = getTimeReal(NULL);
	return snprintf(buffer, bufferLen, "%.2i%.2i%.4i_%.2i%.2i%.2i.ubx", date->tm_mday, date->tm_mon, date->tm_year+1900, date->tm_hour, date->tm_min, date->tm_sec);
}

static void cmd_ubxfetch (const char *cmdStr)
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

int main (const int argc, const char *argv[])
{   

	if (argc > 1 && !strncmp("token:", argv[1], 6))
		cmd_ubxfetch(argv[1]);
	else
		printf("\nUsage: ubxfetch.exe token:<your token>\n");

	return EXIT_SUCCESS;
};

