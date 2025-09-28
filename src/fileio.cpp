
#include <Arduino.h>
#include <SD.h>
#include <inttypes.h>
#include "gps.h"
#include "fileio.h"
#include "record.h"
#include "cmd.h"




void dateTime_cb (uint16_t *date, uint16_t *time, uint8_t *ms10)
{

	dategps_t dategps;
	timegps_t timegps;

	//printf("dateTime_cb:\n");
	getDateTime(&dategps, &timegps);
	
	// Return date using FS_DATE macro to format fields.
	*date = FS_DATE(dategps.year, dategps.month, dategps.day);

	// Return time using FS_TIME macro to format fields.
	*time = FS_TIME(timegps.hour, timegps.min, timegps.sec);

	// Return low time bits in units of 10 ms, 0 <= ms10 <= 199.
	*ms10 = timegps.sec & 1 ? 100 : 0;
}

void fio_setCreationTime (const char *filename, dategps_t *date, timegps_t *time)
{
	DateTimeFields tm = {0};
	
	tm.mday = date->day;
	tm.mon = date->month-1;
	tm.year = date->year - 1900;
	
	tm.hour = time->hour;
	tm.min = time->min;
	tm.sec = time->sec;

	File file = SD.open(filename);
	if (file){
   		file.setCreateTime(tm);
   		file.close();
   	}
}

int fio_setModifyTime (const char *filename, dategps_t *date, timegps_t *time)
{
	int ret = 0;
	
	DateTimeFields tm = {0};
	
	tm.mday = date->day;
	tm.mon = date->month-1;
	tm.year = date->year - 1900;
	
	tm.hour = time->hour;
	tm.min = time->min;
	tm.sec = time->sec;

	File file = SD.open(filename);
	if (file){
   		ret = file.setModifyTime(tm);
   		file.close();
   	}
   	return ret;
}

#if 0
int fio_listDir (const uint8_t *dir)
{
	int fileCount = 0;

	File root = SD.open((char*)dir);
	while (true){
    	File entry = root.openNextFile();
    	if (!entry) break; // no more files

    	if (entry.isDirectory()){
      		printf("  %s/\n", entry.name());
    	}else{
    		printf("  %8lli %s\n", entry.size(), entry.name());
    		fileCount++;
    	}

    	entry.close();
  	}

  	return fileCount;
}
#endif

bool fio_init ()
{
	if (!SD.sdfs.begin(SdioConfig(FIFO_SDIO))){
		//printf("SD.sdfs.begin() failed\r\n");
		return false;
	}
	
	FsDateTime::setCallback(dateTime_cb);
	
	return true;
}

size_t fio_read (fileio_t *fp, void *buffer, const size_t len)
{
	return (size_t)((fp->read(buffer, len) == (int)len));
}

int fio_write (fileio_t *fp, void *buffer, const size_t len)
{
	int ret = fp->write(buffer, len);
	printf(CS("fio_write: len %i, ret %i"), len, ret);

	return (ret == (int)len);
}

fileio_t *fio_open (const uint8_t *filename, const uint32_t fio_flag)
{
	fileio_t *fp = (fileio_t*)calloc(1, sizeof(fileio_t));
	if (!fp) return NULL;
	
	bool fileIsOpen = fp->open((const char*)filename, fio_flag);
	if (fileIsOpen){
		//printf("file open: '%s'\r\n", filename);
		return fp;
	}else{
		free(fp);
		//printf("file open FAILED: '%s'\r\n", filename);
		return NULL;
	}
}

void fio_close (fileio_t *fp)
{
	//printf("fio_close\r\n");
	fp->close();
	free(fp);
}

bool fio_mkdir (const char *dir)
{
	//FatPos_t fatpos;
	//fatpos.position = pos;
	//fp->setpos(&fatpos);
	
	return SD.mkdir(dir);
}

bool fio_seek (fileio_t *fp, size_t pos)
{
	//FatPos_t fatpos;
	//fatpos.position = pos;
	//fp->setpos(&fatpos);
	
	return fp->seekSet(pos);
}

void fio_advance (fileio_t *fp, const size_t amount)
{
	fp->seekCur(amount);
}

size_t fio_length (fileio_t *fp)
{
	return fp->fileSize();
}

int fio_setDir (const char *dir)
{
	return SD.sdfs.chdir(dir);
}
