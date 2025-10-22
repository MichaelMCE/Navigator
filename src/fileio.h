
#ifndef _fileio_h_
#define _fileio_h_

#include <SD.h>
#include "gps.h"




#define FIO_READ		(O_RDONLY)
#define FIO_WRITE		(O_WRONLY)
#define FIO_NEW			(O_CREAT)


typedef struct FsFile fileio_t;


bool fio_init ();
fileio_t *fio_open (const uint8_t *filename, const uint32_t fio_flag);
size_t fio_read (fileio_t *fp, void *buffer, const size_t len);
int fio_write (fileio_t *fp, void *buffer, const size_t len);
void fio_close (fileio_t *fp);
bool fio_seek (fileio_t *fp, size_t pos);
void fio_advance (fileio_t *fp, const size_t amount);
size_t fio_length (fileio_t *fp);
bool fio_mkdir (const char *dir);
//int fio_listDir (const uint8_t *dir);
int fio_setDir (const char *dir);

void fio_setCreationTime (const char *filename, dategps_t *date, timegps_t *time);
int fio_setModifyTime (const char *filename, dategps_t *date, timegps_t *time);

#endif

