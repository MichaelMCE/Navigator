
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




static const char *GNSSIDs[8]		= {"GPS", "SBAS", "Galileo", "BeiDou", "IMES", "QZSS", "GLONASS", ""};
static const char *fixType[6]		= {"None", "Dead reckoning", "2D", "3D", "GNSS+deadReck", "Time only"};
//static const char *geoConfidence[6]	= {"None", "68%", "95%", "99.7%", "99.9999%", "99.999999%"};
//static const char *geoState[4]		= {"Unknown", "Inside", "Outside", ""};
//static const char *UTCStandard[8]	= {"Auto", "1", "2", "GPS", "4", "Galileo", "GLONASS", "BeiDou", "NavIC"};
static const char *dynModel[14]		= {"Portable", "1", "Stationary", "Pedestrian", "Automotive", "Sea", "Airborne < 1g", "Airborne < 2g", "Airborne < 4g", "Wrist", "MBike", "Lawn Mower", "Kick Scooter", ""};
static const char *fixMode[4]		= {"Unknown", "2D only", "3D only", "Auto 2D/3D"};
//static const char *psmState[4]		= {"ACQUISITION", "TRACKING", "POWER OPTIMIZED TRACKING", "INACTIVE"};
//static const char *spoofDetState[4]	= {"Unknown or deactivated", "No spoofing indicated", "Spoofing indicated", "Multiple spoofing indications"};
//static const char *portId[8]        = {"I2C", "UART1", "UART2", "USB", "SPI", "USER0", "USER1", ""};
//static const char *status[2]		  = {"Disabled", "Enabled"};
static const char *timeRef[6] 		= {"UTC", "GPS", "GLONASS", "Beidou", "Galieo", "NavIC"};

static sat_stats_t stats;



const char *getFixName (const uint8_t type)
{
	return fixType[type];
}

sat_stats_t *getSats ()
{
	return &stats;
}


// ######################
// ######################
// ######################
// ######################


int nav_sat (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	//printf("nav_sat %i\n", msg_len);
	
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
	printf(" iTow:    %i\n", sat->iTow);
	printf(" version: %i\n", sat->version);
	printf(" numSvs:  %i\n", sat->numSvs);

	for (int i = 0; i < sat->numSvs; i++){
		const nav_sat_sv_t *sv = &sat->sv[i];
		printf("  %i:\n", i);
		printf("   gnssId: %s\n", GNSSIDs[sv->gnssId&0x07]);
		printf("   svId:   %i\n", sv->svId);
		printf("   cno:    %i\n", sv->cno);
		printf("   elev:   %i\n", sv->elev);
		printf("   azim:   %i\n", sv->azim);
		printf("   prRes:  %.1f\n", sv->prRes/10.0f);
		printf("   flags:  %X\n", sv->flags);
	}
	printf("\n");
	printf("\n");
#endif
	
	return CBFREQ_NONE;
}

int nav_timebds (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 0
	printf("nav_timebds %i\r\n", msg_len);

	const nav_timebds_t *bds = (nav_timebds_t*)payload;
	

	printf(" iTow:       %i\r\n", bds->iTow);
	printf(" SOW:        %i\r\n", bds->SOW);
	printf(" fSOW:       %ui\r\n", bds->fSOW);
	printf(" week:       %i\r\n", bds->week);
	printf(" leapS:      %i\r\n", bds->leapS);
	printf(" valid:      %i\r\n", bds->valid);
	printf(" tAcc:       %i\r\n", bds->tAcc);
	printf("\r\n");
#endif
	return CBFREQ_NONE;
}

int nav_svinfo (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 0
	printf("nav_svinfo %i\r\n", msg_len);


	const nav_svinfo_t *svinfo = (nav_svinfo_t*)payload;

	printf(" iTow:        %i\r\n", svinfo->iTow);
	printf(" numCh:       %i\r\n", svinfo->numCh);
	printf(" globalFlags: %X\r\n", svinfo->globalFlags);
	//printf("  0: %i\r\n", svinfo->reserverd1[0]);
	//printf("  2: %i\r\n", svinfo->reserverd1[1]);
	
	for (int i = 0; i < svinfo->numCh; i++){
		const nav_svinfo_chn_t *sat = &svinfo->sats[i];
		
		printf(" Sv: %i\r\n", i);
		printf("  chn:     %i\r\n", sat->chn);
		printf("  svid:    %i\r\n", sat->svid);
		printf("  flags:   %X\r\n", sat->flags);
		printf("  quality: %i\r\n", sat->quality);
		printf("  elev:    %i\r\n", sat->elev);
		printf("  azim:    %i\r\n", sat->azim);
		printf("  prRes:   %.2f\r\n", dec32flt2(sat->prRes));
	}
	printf("\r\n");
#endif
	return CBFREQ_NONE;
}

int nav_posecef (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	const nav_posecef_t *cef = (nav_posecef_t*)payload;
	
	gpsdata_t *gps = (gpsdata_t*)opaque;
	gps->fix.pAcc = cef->pAcc;

#if 0
	printf("iTow: %u\r\n", cef->iTow);
	printf("ecefX: %i\r\n", cef->ecefX);
	printf("ecefY: %i\r\n", cef->ecefY);
	printf("ecefZ: %i\r\n", cef->ecefZ);
	printf("pAcc: %.2f\r\n", dec32flt2(cef->pAcc));
#endif	
	
	return CBFREQ_HIGH;
}

static int recPos = 0;
static pos_rec_t posRecLLH[40];

static inline void navAddSum (gpsdata_t *gps, pos_rec_t *pos)
{
	posRecLLH[recPos].longitude = pos->longitude;
	posRecLLH[recPos].latitude  = pos->latitude;
	posRecLLH[recPos].altitude  = pos->altitude;
	
	if (++recPos >= 40) recPos = 0;

	double lat = 0.0;
	double lon = 0.0;
	float alt = 0.0f;
	
	for (int i = 0; i < 40; i++){
		lon = (lon + posRecLLH[i].longitude) / 2.0;
		lat = (lat + posRecLLH[i].latitude) / 2.0;
		alt = (alt + posRecLLH[i].altitude) / 2.0f;
	}
	
	gps->navAvg.longitude = lon;
	gps->navAvg.latitude  = lat;
	gps->navAvg.altitude  = alt;
	
	gps->nav.longitude = lon;
	gps->nav.latitude = lat;
	gps->nav.altitude = alt;
}

int nav_pvt (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	//printf("nav_pvt %i\r\n", msg_len);

	const nav_pvt_t *pvt = (nav_pvt_t*)payload;
	gpsdata_t *gps = (gpsdata_t*)opaque;
	


	pos_rec_t pos;
	pos.longitude = dec32dbl7(pvt->lon);
	pos.latitude = dec32dbl7(pvt->lat);
	pos.altitude = dec32flt3(pvt->hMSL);
	
	navAddSum(gps, &pos);
	
/*	gps->nav.longitude = dec32dbl7(pvt->lon);
	gps->nav.latitude = dec32dbl7(pvt->lat);
	gps->nav.altitude = dec32flt3(pvt->hMSL);
*/

    if (pvt->flags&PVT_FLAGS_GNSSFIXOK)			// test for gnssFixOK
    	gps->fix.type = PVT_FIXTYPE_3D;
    else
    	gps->fix.type = 0;
    gps->fix.sats = pvt->numSv;
    gps->fix.hAcc = pvt->hAcc/10.0f;
    gps->fix.vAcc = pvt->vAcc/10.0f;

	gps->iTow = pvt->iTow;

    gps->date.year = pvt->year;
	gps->date.month = pvt->month;
	gps->date.day = pvt->day;
	
	gps->time.hour = pvt->hour;
	gps->time.min = pvt->min;
	gps->time.sec = pvt->sec;
	gps->time.ms = dec32flt7(pvt->nano);
	
	gps->timeAdjusted = 0;
	
	gps->misc.speed = dec32flt3(pvt->gSpeed)*3.60f;
	gps->misc.heading = dec32flt5(pvt->headMot);

#if 0
	printf(" Lon: %f\r\n", dec32flt7(pvt->lon));
	printf(" Lat: %f\r\n", dec32flt7(pvt->lat));
	printf(" Alt: %f\r\n", dec32flt3(pvt->hMSL));
	printf(" SVs: %i\r\n", pvt->numSv);
	
#elif 0
	printf(" iTow:    %i\r\n", pvt->iTow);
	printf(" year:    %i\r\n", pvt->year);
	printf(" month:   %i\r\n", pvt->month);
	printf(" day:     %i\r\n", pvt->day);
	printf(" hour:    %i\r\n", pvt->hour);
	printf(" min:     %i\r\n", pvt->min);
	printf(" sec:     %i\r\n", pvt->sec);
	printf(" valid:   %i\r\n", pvt->valid);
	printf(" tAcc:    %u\r\n", pvt->tAcc);
	printf(" nano:    %i\r\n", pvt->nano);
	printf(" fixType: %s\r\n", fixType[pvt->fixType]);
	printf(" flags:   %X\r\n", pvt->flags);
	printf(" flags2:  %X\r\n", pvt->flags2);
	printf(" numSv:   %i\r\n", pvt->numSv);
	printf(" lon:     %.8f\r\n", dec32flt7(pvt->lon));
	printf(" lat:     %.8f\r\n", dec32flt7(pvt->lat));
	printf(" height:  %f\r\n", dec32flt3(pvt->height));
	printf(" hMSL:    %f\r\n", dec32flt3(pvt->hMSL));
	printf(" hAcc:    %f\r\n", dec32flt3(pvt->hAcc));
	printf(" vAcc:    %f\r\n", dec32flt3(pvt->vAcc));
	printf(" velN:    %f\r\n", dec32flt3(pvt->velN));
	printf(" velE:    %f\r\n", dec32flt3(pvt->velE));
	printf(" velD:    %f\r\n", dec32flt3(pvt->velD));
	printf(" gSpeed:  %f\r\n", dec32flt3(pvt->gSpeed));
	printf(" headMot: %f\r\n", dec32flt5(pvt->headMot));
	
	printf(" sAcc:    %f\r\n", dec32flt3(pvt->sAcc));
	printf(" headAcc: %f\r\n", dec32flt5(pvt->headAcc));
	printf(" pDop:    %f\r\n", dec32flt2(pvt->pDop));
	
	//for (int i = 0; i < sizeof(pvt->reserved1); i++)
		//printf("  %i: %i\r\n", i, pvt->reserved1[i]);
	
	printf(" headVeh: %f\r\n", dec32flt5(pvt->headVeh));
	printf(" magDec:  %f\r\n", dec32flt2(pvt->magDec));
	printf(" magAcc:  %f\r\n", dec32flt2(pvt->magAcc));
	printf("\r\n");
#endif	

	return CBFREQ_HIGH;
}

int ack_ack (const uint8_t *payload, uint16_t msg_len, void *opaque)
{   
	//printf("ack_ack %i\r\n", msg_len);
#if 0
	const ack_ack_t *ack = (ack_ack_t*)payload;
	
	printf("\r\nack_ack: %.2X/%.2X\r\n\r\n", ack->clsId, ack->msgId);
#endif
	return CBFREQ_NONE;
}

int ack_nak (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	//printf("ack_nak %i\r\n", msg_len);
#if 0
	const ack_nak_t *nak = (ack_nak_t*)payload;
	
	printf("\r\nack_nak: %.2X/%.2X\r\n\r\n", nak->clsId, nak->msgId);
#endif
	return CBFREQ_NONE;
}

int nav_status (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 0
	printf("nav_status %i\r\n", msg_len);
	const nav_status_t*status = (nav_status_t*)payload;

	printf(" iTow:    %u\r\n", status->iTow);
	printf(" fixType: %s\r\n", fixType[status->gpsFix]);
	printf(" flags:   %X\r\n", status->flags);
	printf(" fixStat: %X\r\n", status->fixStat);
	printf(" flags2 - psmState:      %s\r\n", psmState[status->flags2&STATUS_FLAGS2_PSMSTATE]);
	printf(" flags2 - spoofDetState: %s\r\n", spoofDetState[(status->flags2&STATUS_FLAGS2_SPOOFDETSTATE)>>3]);
	printf(" ttff:    %ims\r\n", status->ttff);
	printf(" msss:    %ims\r\n", status->msss);
	printf("\r\n");
#endif
	return CBFREQ_NONE;
}

int nav_eoe (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	//printf("nav_eoe %i\r\n", msg_len);
	//const nav_eoe_t *eoe = (nav_eoe_t*)payload;	

	return CBFREQ_HIGH;
}

int aid_eph (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 0
	printf("aid_eph %i\r\n", msg_len);
	
	const aid_eph_t *eph = (aid_eph_t*)payload;
	
	printf(" SvId: %i\r\n", eph->svid);
	printf(" how:  %X\r\n", eph->how);
	
	if (msg_len == 104){	// 104 as per ublox8 ubx spec PDF (33.9.3.3)
		for (int i = 0; i < 8; i++) printf(" %.8X", eph->sf1d[i]);
		printf("\r\n");
		for (int i = 0; i < 8; i++) printf(" %.8X", eph->sf2d[i]);
		printf("\r\n");
		for (int i = 0; i < 8; i++) printf(" %.8X", eph->sf3d[i]);
		printf("\r\n");
	}
	printf("\r\n");	
#endif
	return CBFREQ_NONE;
}

int aid_alm (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 0
	printf("aid_alm %i\r\n", msg_len);

	const aid_alm_t *alm = (aid_alm_t*)payload;
	
	printf(" SvId: %i\r\n", alm->svid);
	printf(" week: %i\r\n", alm->week);
	
	if (msg_len == 40){
		for (int i = 0; i < 8; i++) printf(" %.8X", alm->dwrd[i]);
		printf("\r\n");
	}
	printf("\r\n");
#endif	
	return CBFREQ_NONE;
}


int aid_aop (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 0
	printf("aid_aop %i\r\n", msg_len);

	const aid_aop_t *aop = (aid_aop_t*)payload;
	
	printf(" gnssId: %s\r\n", GNSSIDs[aop->gnssId&0x07]);
	printf(" svId:   %i\r\n", aop->svId);
	
	if (msg_len == 68){
		for (int i = 0; i < 64; i++) printf(" %.2X", aop->data[i]);
		printf("\r\n");
	}
	printf("\r\n");
#endif	
	return CBFREQ_NONE;
}

int nav_dop (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	//printf("nav_dop %i\r\n", msg_len);

	const nav_dop_t *dop = (nav_dop_t*)payload;
	gpsdata_t *gps = (gpsdata_t*)opaque;
	
	gps->dop.horizontal = dop->hDOP;
	gps->dop.vertical = dop->vDOP;
	gps->dop.position = dop->pDOP;
	gps->dop.geometric = dop->gDOP;

#if 0
	printf(" iTow: %i\r\n", dop->iTow);
	printf(" gDOP: %.2f\r\n", dec32flt2(dop->gDOP));
	printf(" pDOP: %.2f\r\n", dec32flt2(dop->pDOP));
	printf(" tDOP: %.2f\r\n", dec32flt2(dop->tDOP));
	printf(" vDOP: %.2f\r\n", dec32flt2(dop->vDOP));
	printf(" hDOP: %.2f\r\n", dec32flt2(dop->hDOP));
	printf(" nDOP: %.2f\r\n", dec32flt2(dop->nDOP));
	printf(" eDOP: %.2f\r\n", dec32flt2(dop->eDOP));
	printf("\r\n");
#endif
	
	return CBFREQ_NONE;
}

int nav_posllh (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	//printf("nav_posllh %i\r\n", msg_len);
	
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


#if 0
	printf(" iTow:   %u\r\n", posllh->iTow);
	printf(" lon:    %.8f\r\n", dec32flt7(posllh->lon));
	printf(" lat:    %.8f\r\n", dec32flt7(posllh->lat));
	printf(" height: %.3f\r\n", dec32flt3(posllh->height));
	printf(" hMSL:   %.3f\r\n", dec32flt3(posllh->hMSL));
	printf(" hAcc:   %.3f\r\n", dec32flt3(posllh->hAcc));
	printf(" vAcc:   %.3f\r\n", dec32flt3(posllh->vAcc));
	printf("\r\n");
#endif

	return CBFREQ_NONE;
}

int mon_ver (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	//printf("mon_ver %i\r\n", msg_len);
	//addDebugLine(" mon_ver: ");
	
	if (msg_len < 40 || (msg_len%30 != 10)){
		//printf("mon_ver: msg corrupt\r\n");
		return CBFREQ_INVALID;
	}

	const int32_t tInfo = (msg_len - 40) / 30;
	if (tInfo > 0){
		const mon_ver_t *ver = (mon_ver_t*)payload;
		//printf("'%s'\r\n", ver->swVersion);
		//printf("'%s'\r\n", ver->hwVersion);
		addDebugLine(ver->swVersion);
		addDebugLine(ver->hwVersion);

		uint8_t *str = (uint8_t*)ver->extension;
		for (int i = 0; i < tInfo; i++){
			//printf("  %i: '%s'\r\n", i, str);
			addDebugLine(str);
			str += 30;
		}
	}
	return CBFREQ_NONE;
}

int mon_io (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 0
	printf("mon_io %i\r\n", msg_len);
	
	const mon_io_t *io = (mon_io_t*)payload;
	const int tPorts = msg_len / 20;
	
	if (tPorts < 1 || (msg_len%20)){		// according to ubx protocol 18
		printf("mon_io: msg corrupt\r\n");
		return CBFREQ_INVALID;
	}
	
	static size_t txBytesPre = 0;	
	const mon_io_port_t *port = &io->port1;
	
	for (int i = 0; i < tPorts; i++, port++){
		if (port->rxBytes || port->txBytes){
			printf("  port:        %s\r\n", portId[i&0x07]);
			
			printf("  rxBytes:     %i\r\n", port->rxBytes);
			printf("  txBytes:     %i\r\n", port->txBytes);
			printf("  parityErrs:  %i\r\n", port->parityErrs);
			printf("  framingErrs: %i\r\n", port->framingErrs);
			printf("  overrunErrs: %i\r\n", port->overrunErrs);
			printf("  breakCond:   %i\r\n", port->breakCond);
			printf("  rxBusy:      %i\r\n", port->rxBusy);
			printf("  txBusy:      %i\r\n", port->txBusy);
			
			printf("  txBytes per second: %i\r\n", port->txBytes - txBytesPre);
			txBytesPre = port->txBytes;
			printf("\r\n");
		}
	}
#endif
	return CBFREQ_MEDIUM;
}

int rxm_sfrbx (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 0
	//printf("rxm_sfrbx %i\r\n", msg_len);
	
	const rxm_sfrbx_t *sfrbx = (rxm_sfrbx_t*)payload;
	
	printf("gnssId:     %i\r\n", sfrbx->gnssId);
	printf("svId:       %i\r\n", sfrbx->svId);
	printf("reserved1:  %i\r\n", sfrbx->reserved1);
	printf("freqId:     %i\r\n", sfrbx->freqId);
	printf("numWords:   %i\r\n", sfrbx->numWords);
	printf("chn:        %i\r\n", sfrbx->chn);
	printf("version:    %i\r\n", sfrbx->version);
	printf("reserved2:  %i\r\n", sfrbx->reserved2);
	
	for (int i = 0; i < sfrbx->numWords; i++)
		printf("%X ", sfrbx->dwrd[i]);
	printf("\r\n\r\n");
#endif	
	return CBFREQ_NONE;
}

int cfg_geofence (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 0
	printf("\r\ncfg_geofence %i\r\n", msg_len);
	
	const cfg_geofence_t *geo = (cfg_geofence_t*)payload;
	
	printf(" version:     %X\r\n", geo->version);
	printf(" numFences:   %i\r\n", geo->numFences);
	printf(" confLvl:     %i - %s\r\n", geo->confLvl, geoConfidence[geo->confLvl]);
	printf(" pioEnabled:  %i\r\n", geo->pioEnabled);
	printf(" pinPolarity: %i\r\n", geo->pinPolarity);
	printf(" pin: %i\r\n", geo->pin);

	
	for (int i = 0; i < geo->numFences; i++){
		printf("  fence %i: lat:    %.8f\r\n", i+1, dec32flt7(geo->fence[i].lat));
		printf("  fence %i: lon:    %.8f\r\n", i+1, dec32flt7(geo->fence[i].lon));
		printf("  fence %i: radius: %.2fm\r\n",i+1, dec32flt2(geo->fence[i].radius));
	}
	printf("\r\n");
#endif		
	return CBFREQ_NONE;
}

int nav_geofence (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 0
	printf("\r\nnav_geofence %i\r\n", msg_len);
	
	const nav_geofence_t *geo = (nav_geofence_t*)payload;
	
	printf("iTow:      %u\r\n", geo->iTow);
	printf("version:   %X\r\n", geo->version);
	printf("status:    %i\r\n", geo->status);
	printf("numFences: %i\r\n", geo->numFences);
	printf("combState: %i\r\n", geo->combState);
	
	for (int i = 0; i < geo->numFences; i++)
		printf("  fence %i: state: %i - %s\r\n",i+1, geo->fence[i].state, geoState[geo->fence[i].state&0x03]);
	printf("\r\n");
#endif	
	return CBFREQ_NONE;
}	

int inf_debug (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 1
	//printf("\r\ninf_debug %i\r\n", msg_len);

	uint8_t msg[msg_len+1];
	memcpy(msg, payload, msg_len);
	msg[msg_len] = 0;
	
	//printf("## \"%s\"\r\n\r\n", msg);
#endif

	addDebugLine(msg);
	return CBFREQ_NONE;	
}

int cfg_rate (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	//printf("\ncfg_rate %i\n", msg_len);
	
	const cfg_rate_t *rate = (cfg_rate_t*)payload;
	char str[64] = {0};

	char *tStd;
	if (rate->timeRef < sizeof(timeRef) / sizeof(*timeRef))
		tStd = (char *)timeRef[rate->timeRef];
	else
		tStd = "Invalid";
	snprintf(str, sizeof(str), "mRate %i, nRate %i, tRef %s", rate->measRate, rate->navRate, tStd);

	addDebugLine((uint8_t*)str);
	
	return CBFREQ_NONE;	
}

int cfg_inf (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 0
	printf("\r\ncfg_inf %i\r\n", msg_len);
	
	const cfg_inf_t *inf = (cfg_inf_t*)payload;

	printf(" protocolID: %i\r\n", inf->protocolID);
	printf(" infMsgMask I2C:   %X\r\n", inf->infMsgMask[CFG_PORTID_I2C]);
	printf(" infMsgMask UART1: %X\r\n", inf->infMsgMask[CFG_PORTID_UART1]);
	printf(" infMsgMask UART2: %X\r\n", inf->infMsgMask[CFG_PORTID_UART2]);
	printf(" infMsgMask USB:   %X\r\n", inf->infMsgMask[CFG_PORTID_USB]);
	printf(" infMsgMask SPI:   %X\r\n", inf->infMsgMask[CFG_PORTID_SPI]);
	
	printf("\r\n");
#endif
	return CBFREQ_NONE;	
}

int cfg_nav5 (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	//printf("\ncfg_nav5 %i\n", msg_len);
	
	const cfg_nav5_t *nav = (cfg_nav5_t*)payload;
	char str[64] = {0};	

#if 0	
	printf(" mask:              %X\n", nav->mask);
	printf(" dynModel:          %s\n", dynModel[nav->dynModel]);
	printf(" fixMode:           %s\n", fixMode[nav->fixMode&0x03]);
	printf(" fixedAlt:          %.2f\n", dec32flt2(nav->fixedAlt));
	printf(" fixedAltVar:       %.4f\n", dec32flt4(nav->fixedAltVar));
	printf(" minElv:            %i\n", nav->minElv);
	printf(" drLimit:           %i\n", nav->drLimit);
	printf(" pDop:              %.1f\n", nav->pDop/10.0f);
	printf(" tDop:              %.1f\n", nav->tDop/10.0f);
	printf(" pAcc:              %i\n", nav->pAcc);
	printf(" tAcc:              %i\n", nav->tAcc);
	printf(" staticHoldThresh:  %i\n", nav->staticHoldThresh);
	printf(" dynssTimeout:      %i\n", nav->dynssTimeout);
	printf(" cnoThreshNumSVs:   %i\n", nav->cnoThreshNumSVs);
	printf(" cnoThresh:         %i\n", nav->cnoThresh);
	printf(" pAccADR:           %i\n", nav->pAccADR);
	printf(" staticHoldMaxDist: %i\n", nav->staticHoldMaxDist);
	printf(" utcStandard:       %s\n", UTCStandard[nav->utcStandard&0x07]);
	printf("\n");
#endif

	//snprintf(str, sizeof(str), "Model: %s", dynModel[nav->dynModel]);
	//addDebugLine((uint8_t*)str);
	//snprintf(str, sizeof(str), "FixMode: %s", fixMode[nav->fixMode&0x03]);
	//addDebugLine((uint8_t*)str);
	
	snprintf(str, sizeof(str), "Model: %s, FixMode: %s", fixMode[nav->fixMode&0x03], dynModel[nav->dynModel]);
	addDebugLine((uint8_t*)str);
	
	return CBFREQ_NONE;	
}

int cfg_navx5 (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 0
	printf("\r\ncfg_navx5 %i\r\n", msg_len);
	
	const cfg_navx5_t *nav = (cfg_navx5_t*)payload;

	printf(" version:          %i\r\n", nav->version);
	printf(" mask1:            %X\r\n", nav->mask1);
	printf(" mask2:            %X\r\n", nav->mask2);
	printf(" minSVs:           %i\r\n", nav->minSVs);
	printf(" maxSVs:           %i\r\n", nav->maxSVs);
	printf(" minCNO:           %i\r\n", nav->minCNO);
	printf(" reserved2:        %i\r\n", nav->reserved2);
	printf(" iniFix3D:         %i\r\n", nav->iniFix3D);
	printf(" ackAiding:        %i\r\n", nav->ackAiding);
	printf(" wknRollover:      %i\r\n", nav->wknRollover);
	printf(" sigAttenCompMode: %i\r\n", nav->sigAttenCompMode);
	printf(" reserved4:        %i\r\n", nav->reserved4);
	printf(" usePPP:           %i\r\n", nav->usePPP);
	printf(" aopCfg:           %X\r\n", nav->aopCfg);
	printf(" aopOrbMaxErr:     %i\r\n", nav->aopOrbMaxErr);
	printf(" useAdr:           %i\r\n", nav->useAdr);
	printf("\r\n");
#endif	
	return CBFREQ_NONE;	
}

int cfg_gnss (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	//printf("\ncfg_gnss %i\n", msg_len);
	
	const cfg_gnss_t *gnss = (cfg_gnss_t*)payload;
	char str[64] = {0};
	
	for (int i = 0; i < gnss->numConfigBlocks; i++){
		const cfg_cfgblk_t *blk = &gnss->cfgblk[i];
		//printf("%i: %s - %s\n", blk->gnssId, GNSSIDs[blk->gnssId&0x07], status[(blk->flags&GNSS_CFGBLK_ENABLED&0x01)]);

		if (blk->flags&GNSS_CFGBLK_ENABLED){
			if (str[0])
				strcat(str, ", ");
			else
				strcat(str, "Enabled: ");
			strcat(str, GNSSIDs[blk->gnssId&0x07]);
		}
		
	}
	addDebugLine((uint8_t*)str);
	
	return CBFREQ_NONE;	
}

int cfg_prt (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
#if 0
	printf("\r\ncfg_prt %i\r\n", msg_len);
	
	const cfg_prt_t *prt = (cfg_prt_t*)payload;
	printf(" portId:       %s\r\n", portId[prt->port.id&0x07]);

	if (prt->port.id == CFG_PORTID_UART1 || prt->port.id == CFG_PORTID_UART2){
		const cfg_prt_uart_t *uart = (cfg_prt_uart_t*)payload;	// or use 'prt.uart.'

		printf(" reserved1:    %i\r\n", uart->reserved1);
		printf(" txReady:      %.4X - en=%i, pol=%i, pin=%i, thres=%i\r\n", uart->txReady.flags, 
			uart->txReady.bits.en, uart->txReady.bits.pol, uart->txReady.bits.pin, uart->txReady.bits.thres);
		printf(" mode:         %.8X - charLen=%X, partity=%X, nStopBits=%X, bitOrder=%X\r\n", uart->mode.flags, 
			uart->mode.bits.charLen, uart->mode.bits.partity, uart->mode.bits.nStopBits, uart->mode.bits.bitOrder);

		printf(" baudRate:     %i\r\n", uart->baudRate);
		printf(" inProtoMask:  %.4X - ", uart->inProtoMask);
		if (uart->inProtoMask&CFG_PROTO_UBX)   printf("UBX ");
		if (uart->inProtoMask&CFG_PROTO_NMEA)  printf("NMEA ");
		if (uart->inProtoMask&CFG_PROTO_RTCM2) printf("RTCM2 ");
		if (uart->inProtoMask&CFG_PROTO_RTCM3) printf("RTCM3 ");
		printf("\r\n");

		printf(" outProtoMask: %.4X - ", uart->outProtoMask);
		if (uart->outProtoMask&CFG_PROTO_UBX)   printf("UBX ");
		if (uart->outProtoMask&CFG_PROTO_NMEA)  printf("NMEA ");
		if (uart->outProtoMask&CFG_PROTO_RTCM2) printf("RTCM2 ");
		if (uart->outProtoMask&CFG_PROTO_RTCM3) printf("RTCM3 ");
		printf("\r\n");
		
		printf(" flags:        %.4X\r\n", uart->flags);
	}
	printf("\r\n");
#endif	
	return CBFREQ_NONE;	
}

int cfg_usb (const uint8_t *payload, uint16_t msg_len, void *opaque)
{
	//printf("\r\ncfg_usb %i\r\n", msg_len);
	//addDebugLine(" cfg_usb: ");
	
	const cfg_usb_t *usb = (cfg_usb_t*)payload;
	
	if (msg_len == 108){		// 108 as per ubx proto18 spec
		/*printf(" vendorID:         %.4X\r\n", usb->vendorID);
		printf(" productID:        %.4X\r\n", usb->productID);
		printf(" powerConsumption: %ima\r\n", usb->powerConsumption);
		printf(" flags:            %.4X\r\n", usb->flags);
		printf(" vendorString:    '%s'\r\n",  usb->vendorString);
		printf(" productString :  '%s'\r\n",  usb->productString);
		printf(" serialNumber:    '%s'\r\n",  usb->serialNumber);*/
		
		addDebugLine(usb->vendorString);
		addDebugLine(usb->productString);
		//addDebugLine(usb->serialNumber);
	}

	return CBFREQ_NONE;	
}
