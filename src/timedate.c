

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include "timedate.h"




void date_formatDateTime (gpsdata_t *gps, char *buffer, const uint32_t len)
{
	snprintf(buffer, len, "%.02i%.02i%.02i_%.02i%.02i%.02i",
		gps->date.day,  gps->date.month, gps->date.year, 
		gps->time.hour, gps->time.min,   gps->time.sec);
}

int date_nWeekdayOfMonth (int week, int wday, int month, int year, struct tm *result)
{
   memset(result, 0, sizeof(*result));

   year -= 1900;
   result->tm_year = year;
   result->tm_mon  = month;
   result->tm_mday = 1;

   time_t t = mktime(result);
   if (t == (time_t)-1)
      return 0;

   if (wday != result->tm_wday)
      result->tm_mday += wday - result->tm_wday + 7 * (wday < result->tm_wday);

   result->tm_mday += 7 * week;

   t = mktime(result);
   if (t == (time_t)-1)
      return 0;

   return 1;
}

void date_adjustUTCToTimeZone (utcTime_t *gps, uint16_t tz, uint8_t tzdir)
{
	static uint8_t maxDayInAnyMonth[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	if (gps->Yr%4 == 0)	//adjust for leapyear
		maxDayInAnyMonth[2] = 29;

	uint8_t maxDayUtcMth = maxDayInAnyMonth[gps->Mth];
	uint8_t maxDayPrevMth = maxDayInAnyMonth[gps->Mth-1];
        
	// month before utc month
	if (!maxDayPrevMth) maxDayPrevMth = 31;

	uint16_t hr = (gps->Hm/100)*100;
	uint16_t m = gps->Hm-hr;  //2115 --> 2100 hr and 15 min
	
	if (tzdir){ //adjusting forwards
		tz += gps->Hm;
		if (tz >= 24){
			gps->Hm = tz-24;
			gps->Day++;                //spill over to next day
			
			if (gps->Day > maxDayUtcMth){
				gps->Day = 1;
				gps->Mth++; //spill over to next month
				
				if (gps->Mth > 12){
					gps->Mth = 1;
					gps->Yr++;        //spill over to next year
				}
			}
		}else{
			gps->Hm = tz;
		}
	}else{ //adjusting backwards
		if (tz > gps->Hm){
			gps->Hm = (24-(tz-hr))+m;
			gps->Day--;  // back to previous day
			
			if (gps->Day == 0){                  //back to previous month
				gps->Mth--;
				gps->Day = maxDayPrevMth;
				
				if (!gps->Mth){
					gps->Mth = 12;               //back to previous year
					gps->Yr--;
				}
			}
		}else{
			gps->Hm -= tz;
		}
	}
}

void date_adjustTime4BST (gpsdata_t *data)
{
	if (data->timeAdjusted)
		return;
	
	int lastSunMarch = 0;
	int lastSunOctober = 0;
	struct tm result;
	
	int resultOk = date_nWeekdayOfMonth(-1, 0/*Sunday*/, 3/*March*/, data->date.year, &result);
	if (resultOk)
		lastSunMarch = result.tm_mday;
	resultOk = date_nWeekdayOfMonth(-1, 0/*Sunday*/, 10/*October*/, data->date.year, &result);
	if (resultOk)
		lastSunOctober = result.tm_mday;

	if ((data->date.month == 3 && data->date.day >= lastSunMarch) || data->date.month > 3){
		if ((data->date.month == 10 && data->date.day < lastSunOctober) || data->date.month < 10){
			utcTime_t utc;
			utc.Yr = data->date.year;
			utc.Mth = data->date.month;
			utc.Day = data->date.day;
			utc.Hm = data->time.hour;			
			
			date_adjustUTCToTimeZone(&utc, 0001/*adjust by 1 hr*/, 1/*1=forward, 0=backward*/);
			data->time.hour = utc.Hm;
			data->date.day = utc.Day;
			data->date.month = utc.Mth;
			data->date.year = utc.Yr;

			data->timeAdjusted = 1;
		}
	}
}
