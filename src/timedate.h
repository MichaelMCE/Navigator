

#ifndef _timedate_h
#define _timedate_h


#include "gps.h"



typedef struct {
	uint16_t Yr;
	uint16_t Mth;
	uint16_t Day;
	uint16_t Hm;
}utcTime_t;


#ifdef __cplusplus
extern "C" {
#endif


void date_adjustTime4BST (gpsdata_t *data);
void date_adjustUTCToTimeZone (utcTime_t *gps, uint16_t tz, uint8_t tzdir);
int date_nWeekdayOfMonth (int week, int wday, int month, int year, struct tm *result);
void date_formatDateTime (gpsdata_t *gps, char *buffer, const uint32_t len);

#ifdef __cplusplus
}
#endif



#endif

