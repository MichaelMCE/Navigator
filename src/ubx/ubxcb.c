
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





#include <string.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include "ubx.h"
#include "ubxcb.h"
#include "../gps.h"
#include "../cmd.h"







PROGMEM static const char *GNSSIDs[8]		= {"GPS", "SBAS", "Galileo", "BeiDou", "IMES", "QZSS", "GLONASS", ""};
PROGMEM static const char *geoConfidence[6]	= {"None", "68%", "95%", "99.7%", "99.9999%", "99.999999%"};
PROGMEM static const char *geoState[4]		= {"Unknown", "Inside", "Outside", ""};
PROGMEM static const char *UTCStandard[10]	= {"Auto", "1", "2", "GPS", "4", "Galileo", "GLONASS", "BeiDou", "NavIC", ""};
PROGMEM static const char *dynModel[14]		= {"Portable", "1", "Stationary", "Pedestrian", "Automotive", "Sea", "Airborne < 1g", "Airborne < 2g", "Airborne < 4g", "Wrist", "MBike", "Lawn Mower", "Kick Scooter", ""};
PROGMEM static const char *fixMode[4]		= {"Unknown", "2D only", "3D only", "Auto 2D/3D"};
PROGMEM static const char *portId[8]        = {"I2C", "UART1", "UART2", "USB", "SPI", "USER0", "USER1", ""};
PROGMEM static const char *status[2]		= {"Disabled", "Enabled"};
PROGMEM static const char *timeRef[8] 		= {"UTC", "GPS", "GLONASS", "Beidou", "Galieo", "NavIC", "", ""};
PROGMEM static const char *navFixType[6]    = {"None", "Dead reckoning", "2D", "3D", "GNSS+deadReck", "Time only"};
PROGMEM static const char *psmState[4]		= {"ACQUISITION", "TRACKING", "POWER OPTIMIZED TRACKING", "INACTIVE"};
PROGMEM static const char *spoofDetState[4]	= {"Unknown or deactivated", "OK. No spoofing indicated", "Spoofing indicated", "Multiple spoofing indications"};
PROGMEM static const char *navStatus[4]     = {"Position and velocity valid and within DOP and ACC masks", "Diff corrections applied", "Week number valid", "Time of week valid"};
PROGMEM static const char *sbasSystem[18]   = {"Unknown", "WAAS", "EGNOS", "MSAS", "GAGAN", "", "", "", "", "", "", "", "", "", "", "", "", "GPS"};
PROGMEM static const char *odoProfile[8]	= {"Running", "Cycling", "Swimming", "Car", "Custom", "", "", ""};
PROGMEM static const char *odoFlags[4]		= {"Odometer-enabled", "Low-speed COG filter enabled", "Output low-pass filtered velocity", "Output low-pass filtered heading"};
PROGMEM static const char *protoId[16]		= {"UBX", "NEMA", "", "RAW", "", "RTCM3", "", "", "", "", "", "", "USER0", "USER1", "USER2", "USER3"};
PROGMEM static const char *updResponseAck[4]= {"Not acknowledged", "Acknowledged"};
PROGMEM static const char *updResponseRes[4]= {"Unknown", "Failed restoring from backup", "Restored from backup", "Not restored (No backup)"};




static sat_stats_t stats;



const char *getFixName (const uint8_t type)
{
	return navFixType[type];
}

sat_stats_t *getSats ()
{
	return &stats;
}


/*
typedef struct {
	uint32_t iTow;				// milliseconds
	 int32_t velN;				// cm/s North velocity component
	 int32_t velE;				// cm/s East velocity component
	 int32_t velD;				// cm/s Down velocity component
	uint32_t speed;				// cm/s Speed (3D)
	uint32_t gSpeed;			// cm/s Ground Speed (2D)
	 int32_t heading;			// deg, 1e-5. heading 2-D
	uint32_t sAcc;				// cm/s, speed acc estimate
	uint32_t cAcc;				// deg, 1e-5, Course/heading acc  estimate
}__attribute__((packed))nav_velned_t;


*/
FLASHMEM int nav_velned (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	printf(CS("\nnav_velned %i"), msg_len);

	const nav_velned_t *vel = (nav_velned_t*)payload;

	printf(CS(" iTow:    %u"), (unsigned int)vel->iTow);
	printf(CS(" velN:    %.2f"), dec32flt3(vel->velN));
	printf(CS(" velE:    %.2f"), dec32flt3(vel->velE));
	printf(CS(" velD:    %.2f"), dec32flt3(vel->velD));
	printf(CS(" speed:   %.3f"), dec32flt3(vel->speed)*3.6f);
	printf(CS(" gSpeed:  %.3f"), dec32flt3(vel->gSpeed)*3.6f);
	printf(CS(" heading: %.1f"), dec32flt5(vel->heading));
	printf(CS(" sAcc:    %.2f"), dec32flt3(vel->sAcc));
	printf(CS(" cAcc:    %.2f"), dec32flt5(vel->cAcc));
	
	return CBFREQ_NONE;
}

FLASHMEM int upd_sos (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	printf(CS("\nupd_sos %i"), msg_len);

	const updsos_cmd_t *upd = (updsos_cmd_t*)payload;
	
	printf(CS(" cmd: %i"), upd->cmd);
	
	if (msg_len == 4){
		printf(CS(" reserved1[3]: %i,%i,%i"), upd->reserved1[0], upd->reserved1[1], upd->reserved1[2]);

	}else if (msg_len == 8){
		if (upd->cmd == 2){
			updsos_ack_t *ack = (updsos_ack_t*)payload;
			printf(CS(" reserved1[3]: %i,%i,%i"), ack->reserved1[0], ack->reserved1[1], ack->reserved1[2]);
			printf(CS(" response:     %i - %s"), ack->response, updResponseAck[ack->response&0x01]);
			printf(CS(" reserved2[3]: %i,%i,%i"), ack->reserved2[0], ack->reserved2[1], ack->reserved2[2]);
			
		}else if (upd->cmd == 3){
			updsos_res_t *res = (updsos_res_t*)payload;
			printf(CS(" reserved1[3]: %i,%i,%i"), res->reserved1[0], res->reserved1[1], res->reserved1[2]);
			printf(CS(" response:     %i - %s"), res->response, updResponseRes[res->response&0x03]);
			printf(CS(" reserved2[3]: %i,%i,%i"), res->reserved2[0], res->reserved2[1], res->reserved2[2]);
		}
	}
	return CBFREQ_NONE;
}

FLASHMEM int cfg_odo (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	printf(CS("\ncfg_odo %i"), msg_len);
	
	const cfg_odo_t *odo = (cfg_odo_t*)payload;

	printf(CS(" version:       %i"), odo->version);
	printf(CS(" reserved1[3]:  %i,%i,%i"), odo->reserved1[0], odo->reserved1[1], odo->reserved1[2]);

	printf(CS(" flags:         0x%X"), (unsigned int)odo->flags);
	if (odo->flags&ODO_FLAGS_USEODO)
		printf(CS("   %s"), odoFlags[0]);
	if (odo->flags&ODO_FLAGS_USECFG)
		printf(CS("   %s"), odoFlags[1]);
	if (odo->flags&ODO_FLAGS_OUTLPVEL)
		printf(CS("   %s"), odoFlags[2]);
	if (odo->flags&ODO_FLAGS_OUTLPCOG)
		printf(CS("   %s"), odoFlags[3]);
	
	printf(CS(" odoCfg:        %i"), odo->odoCfg);
	printf(CS("   profile:     %s"), odoProfile[odo->odoCfg&ODO_PROFILE_MASK]);
	printf(CS(" reserved2[6]:  %i,%i,%i,%i,%i,%i"), odo->reserved2[0], odo->reserved2[1], odo->reserved2[2], odo->reserved2[3], odo->reserved2[4], odo->reserved2[5]);
	
	printf(CS(" cogMaxSpeed:   %.2f"), odo->cogMaxSpeed/10.0f);
	printf(CS(" cogMaxPosAcc:  %u"), odo->cogMaxPosAcc);
	printf(CS(" reserved3[2]:  %i,%i"), odo->reserved3[0], odo->reserved3[1]);
	printf(CS(" velLpGain:     %.2f"), odo->velLpGain/255.0f);
	printf(CS(" cogLpGain:     %.2f"), odo->cogLpGain/255.0f);
	printf(CS(" reserved4[2]:  %i,%i"), odo->reserved4[0], odo->reserved4[1]);

	return CBFREQ_NONE;
}

FLASHMEM int nav_odo (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	const nav_odo_t *odo = (nav_odo_t*)payload;	
	
	gpsdata_t *gps = (gpsdata_t*)opaque;
	gps->misc.distance = odo->distance;
	
#if 0
	printf(CS("\nnav_odo %i"), msg_len);
	printf(CS(" version:       %i"), odo->version);
	printf(CS(" reserved1[3]:  %i,%i,%i"), odo->reserved1[0], odo->reserved1[1], odo->reserved1[2]);
	printf(CS(" iTow:          %u"), (unsigned int)odo->iTow);
	printf(CS(" distance:      %u"), (unsigned int)odo->distance);
	printf(CS(" totalDistance: %u"), (unsigned int)odo->totalDistance);
	printf(CS(" distanceStd:   %u"), (unsigned int)odo->distanceStd);
#endif
	return CBFREQ_NONE;
}

FLASHMEM int nav_sbas (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	printf(CS("\nnav_sbas %i"), msg_len);
	
	const nav_sbas_t *sbas = (nav_sbas_t*)payload;
	
	printf(CS(" iTow:         %u"), (unsigned int)sbas->iTow);
	printf(CS(" geo:          %i"), sbas->geo);
	printf(CS(" mode:         %i"), sbas->mode);
	printf(CS(" sys:          %s"), sbasSystem[sbas->sys]);
	printf(CS(" service:      %i"), sbas->service);
	printf(CS(" cnt:          %i"), sbas->cnt);
	printf(CS(" statusFlags:  %X"), sbas->statusFlags);
	printf(CS(" reserved1[2]: %i,%i"), sbas->reserved1[0], sbas->reserved1[1]);
	
	for (int i = 0; i < sbas->cnt; i++){
		const nav_sbas_sv_t *sv = &sbas->sv[i];
		
		printf(CS("  %i"), i);
		printf(CS("   svid:         %i"), sv->svid);
		printf(CS("   flags:        %X"), (unsigned int)sv->flags);
		printf(CS("   udre:         %i"), sv->udre);
		printf(CS("   svSys:        %i"), sv->svSys);
		printf(CS("   svService:    %i"), sv->svService);
		printf(CS("   reserved2:    %i"), sv->reserved2);
		printf(CS("   prc:          %i"), sv->prc);
		printf(CS("   reserved3[2]: %i,%i"), sv->reserved3[0], sv->reserved3[1]);
		printf(CS("   ic:           %i"), sv->ic);
	}
	
	return CBFREQ_NONE;
}

FLASHMEM int nav_sat (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	//printf(CS("nav_sat %i"), msg_len);
	
	const nav_sat_t *sat = (nav_sat_t*)payload;
	stats.numSvs = sat->numSvs;

	for (int i = 0; i < sat->numSvs; i++){
		const nav_sat_sv_t *sv = &sat->sv[i];

		stats.sv[i].gnssId = sv->gnssId;
		stats.sv[i].svId = sv->svId;
		stats.sv[i].cno = sv->cno;
		stats.sv[i].elev = sv->elev;
		stats.sv[i].azim = sv->azim;
		stats.sv[i].prRes = sv->prRes;
		stats.sv[i].flags = sv->flags;
	}
	
#if 0
	printf(CS(" iTow:    %i"), sat->iTow);
	printf(CS(" version: %i"), sat->version);
	printf(CS(" numSvs:  %i"), sat->numSvs);

	for (int i = 0; i < sat->numSvs; i++){
		const nav_sat_sv_t *sv = &sat->sv[i];
		printf(CS("  %i:"), i);
		printf(CS("   gnssId: %s"), GNSSIDs[sv->gnssId&0x07]);
		printf(CS("   svId:   %i"), sv->svId);
		printf(CS("   cno:    %i"), sv->cno);
		printf(CS("   elev:   %i"), sv->elev);
		printf(CS("   azim:   %i"), sv->azim);
		printf(CS("   prRes:  %.1f"), sv->prRes/10.0f);
		printf(CS("   flags:  %X"), sv->flags);
	}
	printf(CS("\n");
	printf(CS("\n");
#endif
	
	return CBFREQ_NONE;
}

FLASHMEM int nav_timebds (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 1
	printf(CS("nav_timebds %i"), msg_len);

	const nav_timebds_t *bds = (nav_timebds_t*)payload;
	

	printf(CS(" iTow:       %u"), (unsigned int)bds->iTow);
	printf(CS(" SOW:        %u"), (unsigned int)bds->SOW);
	printf(CS(" fSOW:       %u"), (unsigned int)bds->fSOW);
	printf(CS(" week:       %i"), bds->week);
	printf(CS(" leapS:      %i"), bds->leapS);
	printf(CS(" valid:      %i"), bds->valid);
	printf(CS(" tAcc:       %u"), (unsigned int)bds->tAcc);
#endif
	return CBFREQ_NONE;
}

FLASHMEM int nav_svinfo (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 1
	printf(CS("\nnav_svinfo %i"), msg_len);


	const nav_svinfo_t *svinfo = (nav_svinfo_t*)payload;

	printf(CS(" iTow:        %u"), (unsigned int)svinfo->iTow);
	printf(CS(" numCh:       %i"), svinfo->numCh);
	printf(CS(" globalFlags: %X"), svinfo->globalFlags);
	//printf(CS("  0: %i"), svinfo->reserverd1[0]);
	//printf(CS("  2: %i"), svinfo->reserverd1[1]);
	
	for (int i = 0; i < svinfo->numCh; i++){
		const nav_svinfo_chn_t *sat = &svinfo->sats[i];
		
		printf(CS("  Sv: %i"), i);
		printf(CS("   chn:     %i"), sat->chn);
		printf(CS("   svid:    %i"), sat->svid);
		printf(CS("   flags:   %X"), sat->flags);
		printf(CS("   quality: %i"), sat->quality);
		printf(CS("   elev:    %i"), sat->elev);
		printf(CS("   azim:    %i"), sat->azim);
		printf(CS("   prRes:   %.2f"), dec32flt2(sat->prRes));
	}
#endif
	return CBFREQ_NONE;
}

FLASHMEM int nav_posecef (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	const nav_posecef_t *cef = (nav_posecef_t*)payload;
	
	gpsdata_t *gps = (gpsdata_t*)opaque;
	gps->fix.pAcc = cef->pAcc;

#if 0
	printf(CS("iTow: %u"), cef->iTow);
	printf(CS("ecefX: %i"), cef->ecefX);
	printf(CS("ecefY: %i"), cef->ecefY);
	printf(CS("ecefZ: %i"), cef->ecefZ);
	printf(CS("pAcc: %.2f"), dec32flt2(cef->pAcc));
#endif	
	
	return CBFREQ_HIGH;
}

static int recPos = 0;
static pos_rec_t posRecLLH[32];

static inline void navAddSum (gpsdata_t *gps, pos_rec_t *pos)
{
	gps->nav.longitude = pos->longitude;
	gps->nav.latitude = pos->latitude;
	gps->nav.altitude = pos->altitude;

	posRecLLH[recPos].longitude = pos->longitude;
	posRecLLH[recPos].latitude  = pos->latitude;
	posRecLLH[recPos].altitude  = pos->altitude;
	if (++recPos >= 32) recPos = 0;
	
	double lat = 0.0;
	double lon = 0.0;
	float alt = 0.0f;
	
	for (int i = 0; i < 32; i++){
		lon = (lon + posRecLLH[i].longitude) / 2.0;
		lat = (lat + posRecLLH[i].latitude) / 2.0;
		alt = (alt + posRecLLH[i].altitude) / 2.0f;
	}
	
	gps->navAvg.longitude = lon;
	gps->navAvg.latitude  = lat;
	gps->navAvg.altitude  = alt;

	gps->rates.epoch++;
}

FLASHMEM int nav_pvt (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	//printf(CS("nav_pvt %i"), msg_len);

	const nav_pvt_t *pvt = (nav_pvt_t*)payload;
	gpsdata_t *gps = (gpsdata_t*)opaque;

#if 0
	gps->nav.longitude = dec32dbl7(pvt->lon);
	gps->nav.latitude = dec32dbl7(pvt->lat);
	gps->nav.altitude = dec32flt3(pvt->hMSL);
#endif

	gps->fix.type = pvt->fixType;
    gps->fix.sats = pvt->numSv;
    //gps->fix.hAcc = pvt->hAcc/10.0f;
    //gps->fix.vAcc = pvt->vAcc/10.0f;

#if 1
	gps->iTow = pvt->iTow;

    gps->date.year = pvt->year;
	gps->date.month = pvt->month;
	gps->date.day = pvt->day;
	
	gps->time.hour = pvt->hour;
	gps->time.min = pvt->min;
	gps->time.sec = pvt->sec;
	gps->time.ms = dec32flt7(pvt->nano);

	gps->timeAdjusted = 0;
#endif
	
	gps->misc.speed = dec32flt3(pvt->gSpeed)*3.60f;
	gps->misc.heading = dec32flt5(pvt->headMot);

#if 0
	printf(CS(" Lon: %f"), dec32flt7(pvt->lon));
	printf(CS(" Lat: %f"), dec32flt7(pvt->lat));
	printf(CS(" Alt: %f"), dec32flt3(pvt->hMSL));
	printf(CS(" SVs: %i"), pvt->numSv);
	
#elif 0
	printf(CS(" iTow:    %i"), pvt->iTow);
	printf(CS(" year:    %i"), pvt->year);
	printf(CS(" month:   %i"), pvt->month);
	printf(CS(" day:     %i"), pvt->day);
	printf(CS(" hour:    %i"), pvt->hour);
	printf(CS(" min:     %i"), pvt->min);
	printf(CS(" sec:     %i"), pvt->sec);
	printf(CS(" valid:   %i"), pvt->valid);
	printf(CS(" tAcc:    %u"), pvt->tAcc);
	printf(CS(" nano:    %i"), pvt->nano);
	printf(CS(" fixType: %s"), fixType[pvt->fixType]);
	printf(CS(" flags:   %X"), pvt->flags);
	printf(CS(" flags2:  %X"), pvt->flags2);
	printf(CS(" numSv:   %i"), pvt->numSv);
	printf(CS(" lon:     %.8f"), dec32flt7(pvt->lon));
	printf(CS(" lat:     %.8f"), dec32flt7(pvt->lat));
	printf(CS(" height:  %f"), dec32flt3(pvt->height));
	printf(CS(" hMSL:    %f"), dec32flt3(pvt->hMSL));
	printf(CS(" hAcc:    %f"), dec32flt3(pvt->hAcc));
	printf(CS(" vAcc:    %f"), dec32flt3(pvt->vAcc));
	printf(CS(" velN:    %f"), dec32flt3(pvt->velN));
	printf(CS(" velE:    %f"), dec32flt3(pvt->velE));
	printf(CS(" velD:    %f"), dec32flt3(pvt->velD));
	printf(CS(" gSpeed:  %f"), dec32flt3(pvt->gSpeed));
	printf(CS(" headMot: %f"), dec32flt5(pvt->headMot));
	
	printf(CS(" sAcc:    %f"), dec32flt3(pvt->sAcc));
	printf(CS(" headAcc: %f"), dec32flt5(pvt->headAcc));
	printf(CS(" pDop:    %f"), dec32flt2(pvt->pDop));
	
	//for (int i = 0; i < sizeof(pvt->reserved1); i++)
		//printf(CS("  %i: %i"), i, pvt->reserved1[i]);
	
	printf(CS(" headVeh: %f"), dec32flt5(pvt->headVeh));
	printf(CS(" magDec:  %f"), dec32flt2(pvt->magDec));
	printf(CS(" magAcc:  %f"), dec32flt2(pvt->magAcc));
	printf(CS("\n");
#endif	

	return CBFREQ_HIGH;
}

FLASHMEM int ack_ack (const uint8_t *payload, uint16_t msg_len, void *opaque)
{   
	//printf(CS("ack_ack %i"), msg_len);
#if 0
	const ack_ack_t *ack = (ack_ack_t*)payload;
	
	printf(CS("\nack_ack: %.2X/%.2X\n"), ack->clsId, ack->msgId);
#endif
	return CBFREQ_NONE;
}

FLASHMEM int ack_nak (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	//printf(CS("ack_nak %i"), msg_len);
#if 0
	const ack_nak_t *nak = (ack_nak_t*)payload;
	
	printf(CS("\nack_nak: %.2X/%.2X\n"), nak->clsId, nak->msgId);
#endif
	return CBFREQ_NONE;
}

FLASHMEM int nav_eoe (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	//printf(CS("nav_eoe %i"), msg_len);
	//const nav_eoe_t *eoe = (nav_eoe_t*)payload;	

	return CBFREQ_HIGH;
}

FLASHMEM int aid_eph (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 1
	printf(CS("\naid_eph %i"), msg_len);
	
	const aid_eph_t *eph = (aid_eph_t*)payload;
	
	printf(CS(" SvId: %i"), (int)eph->svid);
	printf(CS(" how:  %X"), (unsigned int)eph->how);
	
	if (msg_len == 104){	// 104 as per ublox8 ubx spec PDF (33.9.3.3)
		printf(CS("  sf1d"));
		for (int i = 0; i < 8; i++)
			printf(CS("   %.8X"), (unsigned int)eph->sf1d[i]);

		printf(CS("  sf2d"));
		for (int i = 0; i < 8; i++)
			printf(CS("   %.8X"), (unsigned int)eph->sf2d[i]);

		printf(CS("  sf3d"));
		for (int i = 0; i < 8; i++)
			printf(CS("   %.8X"), (unsigned int)eph->sf3d[i]);

	}

#endif
	return CBFREQ_NONE;
}

FLASHMEM int aid_alm (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 1
	printf(CS("\naid_alm %i"), msg_len);

	const aid_alm_t *alm = (aid_alm_t*)payload;
	
	printf(CS(" SvId: %i"), (int)alm->svid);
	printf(CS(" week: %i"), (int)alm->week);
	
	if (msg_len == 40){
		printf(CS("  dwrd"));
		for (int i = 0; i < 8; i++)
			printf(CS("   %.8X"), (unsigned int)alm->dwrd[i]);

	}
#endif	
	return CBFREQ_NONE;
}


FLASHMEM int aid_aop (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 1
	printf(CS("\naid_aop %i"), msg_len);

	const aid_aop_t *aop = (aid_aop_t*)payload;
	
	printf(CS(" gnssId: %s"), GNSSIDs[aop->gnssId&0x07]);
	printf(CS(" svId:   %i"), aop->svId);
	
	if (msg_len == 68){
		printf(CS("  data"));
		for (int i = 0; i < 64; i++)
			printf(CS("   %.2X"), aop->data[i]);
	}
#endif	
	return CBFREQ_NONE;
}

FLASHMEM int nav_dop (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	//printf(CS("nav_dop %i"), msg_len);

	const nav_dop_t *dop = (nav_dop_t*)payload;
	gpsdata_t *gps = (gpsdata_t*)opaque;
	
	gps->dop.horizontal = dop->hDOP;
	gps->dop.vertical = dop->vDOP;
	gps->dop.position = dop->pDOP;
	gps->dop.geometric = dop->gDOP;

#if 0
	printf(CS(" iTow: %i"), dop->iTow);
	printf(CS(" gDOP: %.2f"), dec32flt2(dop->gDOP));
	printf(CS(" pDOP: %.2f"), dec32flt2(dop->pDOP));
	printf(CS(" tDOP: %.2f"), dec32flt2(dop->tDOP));
	printf(CS(" vDOP: %.2f"), dec32flt2(dop->vDOP));
	printf(CS(" hDOP: %.2f"), dec32flt2(dop->hDOP));
	printf(CS(" nDOP: %.2f"), dec32flt2(dop->nDOP));
	printf(CS(" eDOP: %.2f"), dec32flt2(dop->eDOP));
	printf(CS("\n");
#endif
	
	return CBFREQ_NONE;
}

FLASHMEM int nav_posllh (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	//printf(CS("nav_posllh %i"), msg_len);
	
	const nav_posllh_t *posllh = (nav_posllh_t*)payload;
	gpsdata_t *gps = (gpsdata_t*)opaque;


	pos_rec_t pos;
	pos.longitude = dec32dbl7(posllh->lon);
	pos.latitude = dec32dbl7(posllh->lat);
	pos.altitude = dec32flt3(posllh->hMSL);
	
	navAddSum(gps, &pos);

/*
	gps->nav.longitude = dec32dbl7(posllh->lon);
	gps->nav.latitude = dec32dbl7(posllh->lat);
	gps->nav.altitude = dec32flt3(posllh->hMSL);

	posRecLLH[recPos].longitude = gps->nav.longitude;
	posRecLLH[recPos].latitude  = gps->nav.latitude;
	posRecLLH[recPos].altitude  = gps->nav.altitude;
	if (++recPos >= 32) recPos = 0;

	double lat = 0.0;
	double lon = 0.0;
	float alt = 0.0f;
	
	for (int i = 0; i < 32; i++){
		lon = (lon + posRecLLH[i].longitude) / 2.0;
		lat = (lat + posRecLLH[i].latitude) / 2.0;
		alt = (alt + posRecLLH[i].altitude) / 2.0f;
	}
	
	gps->navAvg.longitude = lon;
	gps->navAvg.latitude  = lat;
	gps->navAvg.altitude  = alt;
*/	
	gps->fix.hAcc = posllh->hAcc/10.0f;
    gps->fix.vAcc = posllh->vAcc/10.0f;
    
	gps->time.hour = (((posllh->iTow/1000)/60)/60)%24;
	gps->time.min = ((posllh->iTow/1000)/60)%60;
	gps->time.sec = ((posllh->iTow/1000)%60);
    gps->time.ms = (posllh->iTow%1000)/10;
	//gps->iTow = posllh->iTow;

#if 0
	printf(CS(" iTow:   %u"), posllh->iTow);
	printf(CS(" lon:    %.8f"), dec32flt7(posllh->lon));
	printf(CS(" lat:    %.8f"), dec32flt7(posllh->lat));
	printf(CS(" height: %.3f"), dec32flt3(posllh->height));
	printf(CS(" hMSL:   %.3f"), dec32flt3(posllh->hMSL));
	printf(CS(" hAcc:   %.3f"), dec32flt3(posllh->hAcc));
	printf(CS(" vAcc:   %.3f"), dec32flt3(posllh->vAcc));
	printf(CS("\n");
#endif

	return CBFREQ_NONE;
}

FLASHMEM int nav_status (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 1
	const nav_status_t*status = (nav_status_t*)payload;

	printf(CS("\nnav_status %i"), msg_len);
	printf(CS(" iTow:    %u"), (unsigned int)status->iTow);
	if (status->gpsFix < 6)
		printf(CS(" fixType: %s"), navFixType[status->gpsFix]);
		
	printf(CS(" flags:   0x%X"), status->flags);
	if (status->flags&STATUS_FLAGS_GPSFIXOK)
		printf(CS("  %s"), navStatus[0]);
	if (status->flags&STATUS_FLAGS_DIFFSOLN)
		printf(CS("  %s"), navStatus[1]);
	if (status->flags&STATUS_FLAGS_WKNSET)
		printf(CS("  %s"), navStatus[2]);
	if (status->flags&STATUS_FLAGS_TOWSET)
		printf(CS("  %s"), navStatus[3]);	
	
	printf(CS(" fixStat: %X"), status->fixStat);
	printf(CS(" flags2:  %X"), status->flags2);
	printf(CS("   psmState: %s"), psmState[status->flags2&STATUS_FLAGS2_PSMSTATE]);
	printf(CS("   spoofDetState: %s"), spoofDetState[(status->flags2&STATUS_FLAGS2_SPOOFDETSTATE)>>3]);
	printf(CS(" ttff:    %u"), (unsigned int)status->ttff);
	printf(CS(" msss:    %u"), (unsigned int)status->msss);

#endif
	return CBFREQ_NONE;
}

FLASHMEM int mon_ver (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	//printf(CS("mon_ver %i"), msg_len);
	//addDebugLine(" mon_ver: ");
	
	if (msg_len < 40 || (msg_len%30 != 10)){
		//printf(CS("mon_ver: msg corrupt\n");
		return CBFREQ_INVALID;
	}

	const int32_t tInfo = (msg_len - 40) / 30;
	if (tInfo > 0){
		const mon_ver_t *ver = (mon_ver_t*)payload;
		//printf(CS("'%s'"), ver->swVersion);
		//printf(CS("'%s'"), ver->hwVersion);
		addDebugLine(ver->swVersion);
		addDebugLine(ver->hwVersion);

		uint8_t *str = (uint8_t*)ver->extension;
		for (int i = 0; i < tInfo; i++){
			//printf(CS("  %i: '%s'"), i, str);
			addDebugLine(str);
			str += 30;
		}
	}
	return CBFREQ_NONE;
}

FLASHMEM int mon_io (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 1
	printf(CS("\nmon_io %i"), msg_len);
	
	const mon_io_t *io = (mon_io_t*)payload;
	const int tPorts = msg_len / 20;
	
	if (tPorts < 1 || (msg_len%20)){		// according to ubx protocol 18
		printf(CS("mon_io: msg corrupt"));
		return CBFREQ_INVALID;
	}
	
	//static size_t txBytesPre = 0;
	const mon_io_port_t *port = &io->port1;
	
	for (int i = 0; i < tPorts; i++, port++){
		//if (port->rxBytes || port->txBytes){
			printf(CS(" port:         %s"), portId[i&0x07]);
			printf(CS("  rxBytes:     %u"), (unsigned int)port->rxBytes);
			printf(CS("  txBytes:     %u"), (unsigned int)port->txBytes);
			printf(CS("  parityErrs:  %i"), port->parityErrs);
			printf(CS("  framingErrs: %i"), port->framingErrs);
			printf(CS("  overrunErrs: %i"), port->overrunErrs);
			printf(CS("  breakCond:   %i"), port->breakCond);
			printf(CS("  rxBusy:      %i"), port->rxBusy);
			printf(CS("  txBusy:      %i"), port->txBusy);
			
			//printf(CS(" txBytes since previous call: %u"), (unsigned int)(port->txBytes - txBytesPre));
			//txBytesPre = port->txBytes;
			printf(CS(""));
		//}
	}
	
#endif
	return CBFREQ_MEDIUM;
}

FLASHMEM int rxm_sfrbx (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 1
	printf(CS("\nrxm_sfrbx %i"), msg_len);
	const rxm_sfrbx_t *sfrbx = (rxm_sfrbx_t*)payload;
	
	printf(CS("gnssId:     %s"), GNSSIDs[sfrbx->gnssId&0x07]);
	printf(CS("svId:       %i"), sfrbx->svId);
	printf(CS("reserved1:  %i"), sfrbx->reserved1);
	printf(CS("freqId:     %i"), sfrbx->freqId);
	printf(CS("numWords:   %i"), sfrbx->numWords);
	printf(CS("chn:        %i"), sfrbx->chn);
	printf(CS("version:    %i"), sfrbx->version);
	printf(CS("reserved3:  %i"), sfrbx->reserved3);
	
	for (int i = 0; i < sfrbx->numWords; i++)
		printf(CS("  %.8X "), (unsigned int)sfrbx->dwrd[i]);

#endif	
	return CBFREQ_NONE;
}

FLASHMEM int cfg_geofence (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 1
	printf(CS("\ncfg_geofence %i"), msg_len);
	
	const cfg_geofence_t *geo = (cfg_geofence_t*)payload;
	
	printf(CS(" version:     %X"), geo->version);
	printf(CS(" numFences:   %i"), geo->numFences);
	printf(CS(" confLvl:     %i - %s"), geo->confLvl, geoConfidence[geo->confLvl]);
	printf(CS(" pioEnabled:  %i"), geo->pioEnabled);
	printf(CS(" pinPolarity: %i"), geo->pinPolarity);
	printf(CS(" pin: %i"), geo->pin);

	
	for (int i = 0; i < geo->numFences; i++){
		printf(CS("  fence %i: lat:    %.8f"), i+1, dec32flt7(geo->fence[i].lat));
		printf(CS("  fence %i: lon:    %.8f"), i+1, dec32flt7(geo->fence[i].lon));
		printf(CS("  fence %i: radius: %.2fm"),i+1, dec32flt2(geo->fence[i].radius));
	}
	//printf(CS("\n");
#endif		
	return CBFREQ_NONE;
}

FLASHMEM int nav_geofence (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 1
	printf(CS("\nnav_geofence %i"), msg_len);
	
	const nav_geofence_t *geo = (nav_geofence_t*)payload;
	
	printf(CS("iTow:      %u"), (unsigned int)geo->iTow);
	printf(CS("version:   %X"), geo->version);
	printf(CS("status:    %i"), geo->status);
	printf(CS("numFences: %i"), geo->numFences);
	printf(CS("combState: %i"), geo->combState);
	
	for (int i = 0; i < geo->numFences; i++)
		printf(CS("  fence %i: state: %i - %s"),i+1, geo->fence[i].state, geoState[geo->fence[i].state&0x03]);

#endif	
	return CBFREQ_NONE;
}	

FLASHMEM int inf_debug (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 1
	//printf(CS("\ninf_debug %i"), msg_len);

	uint8_t msg[msg_len+1];
	memcpy(msg, payload, msg_len);
	msg[msg_len] = 0;
	
	//printf(CS("## \"%s\"\n"), msg);
#endif

	addDebugLine(msg);
	return CBFREQ_NONE;	
}

FLASHMEM int cfg_rate (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	printf(CS("\ncfg_rate %i"), msg_len);
	
	const cfg_rate_t *rate = (cfg_rate_t*)payload;
	char str[64] = {0};

	char *tStd;
	if (rate->timeRef < sizeof(timeRef) / sizeof(*timeRef))
		tStd = (char *)timeRef[rate->timeRef];
	else
		tStd = "Invalid";

	snprintf(str, sizeof(str), " mRate %i, nRate %i, tRef %s", rate->measRate, rate->navRate, tStd);
	addDebugLine((uint8_t*)str);
	
	return CBFREQ_NONE;	
}

FLASHMEM int cfg_inf (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 1
	printf(CS("\ncfg_inf %i"), msg_len);
	
	const cfg_inf_t *inf = (cfg_inf_t*)payload;

	printf(CS(" protocolID: %s"), protoId[inf->protocolID&0x0F]);
	printf(CS(" infMsgMask I2C:   %X"), inf->infMsgMask[CFG_PORTID_I2C]);
	printf(CS(" infMsgMask UART1: %X"), inf->infMsgMask[CFG_PORTID_UART1]);
	printf(CS(" infMsgMask UART2: %X"), inf->infMsgMask[CFG_PORTID_UART2]);
	printf(CS(" infMsgMask USB:   %X"), inf->infMsgMask[CFG_PORTID_USB]);
	printf(CS(" infMsgMask SPI:   %X"), inf->infMsgMask[CFG_PORTID_SPI]);
	
	//printf(CS("\n");
#endif
	return CBFREQ_NONE;	
}

FLASHMEM int cfg_nav5 (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	printf(CS("\ncfg_nav5 %i"), msg_len);
	
	const cfg_nav5_t *nav = (cfg_nav5_t*)payload;
	char str[64] = {0};	

#if 1	
	printf(CS(" mask:              %X"), nav->mask);
	printf(CS(" dynModel:          %s"), dynModel[nav->dynModel]);
	printf(CS(" fixMode:           %s"), fixMode[nav->fixMode&0x03]);
	printf(CS(" fixedAlt:          %.2f"), dec32flt2(nav->fixedAlt));
	printf(CS(" fixedAltVar:       %.4f"), dec32flt4(nav->fixedAltVar));
	printf(CS(" minElv:            %i"), nav->minElv);
	printf(CS(" drLimit:           %i"), nav->drLimit);
	printf(CS(" pDop:              %.1f"), nav->pDop/10.0f);
	printf(CS(" tDop:              %.1f"), nav->tDop/10.0f);
	printf(CS(" pAcc:              %i"), nav->pAcc);
	printf(CS(" tAcc:              %i"), nav->tAcc);
	printf(CS(" staticHoldThresh:  %i"), nav->staticHoldThresh);
	printf(CS(" dynssTimeout:      %i"), nav->dynssTimeout);
	printf(CS(" cnoThreshNumSVs:   %i"), nav->cnoThreshNumSVs);
	printf(CS(" cnoThresh:         %i"), nav->cnoThresh);
	printf(CS(" pAccADR:           %i"), nav->pAccADR);
	printf(CS(" staticHoldMaxDist: %i"), nav->staticHoldMaxDist);
	printf(CS(" utcStandard:       %s"), UTCStandard[nav->utcStandard&0x07]);
	//printf(CS("\n"));
#endif

	//snprintf(str, sizeof(str), "Model: %s", dynModel[nav->dynModel]);
	//addDebugLine((uint8_t*)str);
	//snprintf(str, sizeof(str), "FixMode: %s", fixMode[nav->fixMode&0x03]);
	//addDebugLine((uint8_t*)str);
	
	snprintf(str, sizeof(str), " Model: %s, FixMode: %s", fixMode[nav->fixMode&0x03], dynModel[nav->dynModel]);
	addDebugLine((uint8_t*)str);
	
	return CBFREQ_NONE;	
}

FLASHMEM int cfg_navx5 (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 1
	printf(CS("\ncfg_navx5 %i"), msg_len);
	
	const cfg_navx5_t *nav = (cfg_navx5_t*)payload;

	printf(CS(" version:          %i"), nav->version);
	printf(CS(" mask1:            %X"), nav->mask1);
	printf(CS(" mask2:            %X"), (int)nav->mask2);
	printf(CS(" minSVs:           %i"), nav->minSVs);
	printf(CS(" maxSVs:           %i"), nav->maxSVs);
	printf(CS(" minCNO:           %i"), nav->minCNO);
	printf(CS(" reserved2:        %i"), nav->reserved2);
	printf(CS(" iniFix3D:         %i"), nav->iniFix3D);
	printf(CS(" ackAiding:        %i"), nav->ackAiding);
	printf(CS(" wknRollover:      %i"), nav->wknRollover);
	printf(CS(" sigAttenCompMode: %i"), nav->sigAttenCompMode);
	printf(CS(" reserved4:        %i"), nav->reserved4);
	printf(CS(" usePPP:           %i"), nav->usePPP);
	printf(CS(" aopCfg:           %X"), nav->aopCfg);
	printf(CS(" aopOrbMaxErr:     %i"), nav->aopOrbMaxErr);
	printf(CS(" useAdr:           %i"), nav->useAdr);
	//printf(CS("\n"));
#endif	
	return CBFREQ_NONE;	
}

FLASHMEM int cfg_gnss (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	printf(CS("\ncfg_gnss %i"), msg_len);
	
	const cfg_gnss_t *gnss = (cfg_gnss_t*)payload;
	char str[64] = {0};

	printf(CS(" msgVer:          %i"), gnss->msgVer);
	printf(CS(" numTrkChHw:      %i"), gnss->numTrkChHw);
	printf(CS(" numTrkChUse:     %i"), gnss->numTrkChUse);
	printf(CS(" numConfigBlocks: %i"), gnss->numConfigBlocks);
	
	for (int i = 0; i < gnss->numConfigBlocks; i++){
		const cfg_cfgblk_t *blk = &gnss->cfgblk[i];
		printf(CS("   gnssId:%i - %s - %s"), blk->gnssId, GNSSIDs[blk->gnssId&0x07], status[(blk->flags&GNSS_CFGBLK_ENABLED&0x01)]);

		if (blk->flags&GNSS_CFGBLK_ENABLED){
			if (str[0])
				strcat(str, ", ");
			else
				strcat(str, " Enabled: ");
			strcat(str, GNSSIDs[blk->gnssId&0x07]);
		}
	}
	printf(CS(""));
	addDebugLine((uint8_t*)str);
	
	return CBFREQ_NONE;	
}

FLASHMEM int cfg_prt (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 1
	printf(CS("\ncfg_prt %i"), msg_len);
	
	const cfg_prt_t *prt = (cfg_prt_t*)payload;
	printf(CS(" portId:       %s"), portId[prt->port.id&0x07]);

	if (prt->port.id == CFG_PORTID_UART1 || prt->port.id == CFG_PORTID_UART2){
		const cfg_prt_uart_t *uart = (cfg_prt_uart_t*)payload;	// or use 'prt.uart.'

		printf(CS(" reserved1:    %i"), uart->reserved1);
		printf(CS(" txReady:      %.4X - en=%i, pol=%i, pin=%i, thres=%i"), uart->txReady.flags, 
			uart->txReady.bits.en, uart->txReady.bits.pol, uart->txReady.bits.pin, uart->txReady.bits.thres);
		printf(CS(" mode:         %.8X - charLen=%X, partity=%X, nStopBits=%X, bitOrder=%X"), (int)uart->mode.flags, 
			uart->mode.bits.charLen, uart->mode.bits.partity, uart->mode.bits.nStopBits, uart->mode.bits.bitOrder);

		printf(CS(" baudRate:     %i"), (int)uart->baudRate);
		printf(CS(" flags:        %.4X"), uart->flags);
		printf(CS(" inProtoMask:  %.4X:"), uart->inProtoMask);
		if (uart->inProtoMask&CFG_PROTO_UBX)   printf(CS("  UBX "));
		if (uart->inProtoMask&CFG_PROTO_NMEA)  printf(CS("  NMEA "));
		if (uart->inProtoMask&CFG_PROTO_RTCM2) printf(CS("  RTCM2 "));
		if (uart->inProtoMask&CFG_PROTO_RTCM3) printf(CS("  RTCM3 "));
		printf(CS(" outProtoMask: %.4X:"), uart->outProtoMask);
		if (uart->outProtoMask&CFG_PROTO_UBX)   printf(CS("  UBX "));
		if (uart->outProtoMask&CFG_PROTO_NMEA)  printf(CS("  NMEA "));
		if (uart->outProtoMask&CFG_PROTO_RTCM2) printf(CS("  RTCM2 "));
		if (uart->outProtoMask&CFG_PROTO_RTCM3) printf(CS("  RTCM3 "));
		//printf(CS("\n"));

	}
	//printf(CS("\n");
#endif	
	return CBFREQ_NONE;	
}

FLASHMEM int cfg_usb (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	printf(CS("\ncfg_usb %i"), msg_len);
	//addDebugLine(" cfg_usb: ");
	
	const cfg_usb_t *usb = (cfg_usb_t*)payload;
	
	if (msg_len == 108){		// 108 as per ubx proto18 spec
		printf(CS(" vendorID:         %.4X"), usb->vendorID);
		printf(CS(" productID:        %.4X"), usb->productID);
		printf(CS(" powerConsumption: %ima"), usb->powerConsumption);
		printf(CS(" flags:            %.4X"), usb->flags);
		printf(CS(" vendorString:    '%s'"),  usb->vendorString);
		printf(CS(" productString:   '%s'"),  usb->productString);
		printf(CS(" serialNumber:    '%s'"),  usb->serialNumber);
		
		addDebugLine(usb->vendorString);
		addDebugLine(usb->productString);
		//addDebugLine(usb->serialNumber);
	}

	return CBFREQ_NONE;	
}
