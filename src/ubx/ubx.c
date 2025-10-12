
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

#include <Arduino.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ubx.h"
#include "ubxcb.h"
#include "../gps.h"


#define ALERT_DISABLED		0
#define ALERT_BEFORE		1
#define ALERT_AFTER			2
#define ALERT_SINGLE		4




static ubx_msg_t ubxRegTable = {0};
static gpsdata_t userData = {0};
const uint32_t baudRates[] = {9600, 9600*2, 9600*4, 9600*6, 115200, 115200*2, 115200*4, 115200*8, 0};



#if 0
static void payloadDumpCt ()
{
	printf("rx msg count:\r\n");

	for (int i = 0; i < MAX_REGMSG; i++){
		ubx_func_t *handler = &ubxRegTable.msg[i];
		if (handler->ct){
			printf(" %2i %.2X/%.2X - %i\r\n", i, handler->uClass, handler->uId, handler->ct);
			handler->ct = 0;
		}
	}
	printf("\r\n");
}
#endif

// less than once per second
FLASHMEM void msgPostLowCb (void *opaque, const intptr_t unused)
{
	//gpsdata_t *data = (gpsdata_t*)opaque;
	//printf("msgPostLowCb\r\n");
}

// once per second, is called through mon_io
FLASHMEM void msgPostMedCb (void *opaque, const intptr_t unused)
{
	msgPostMed(opaque, unused);
}

FLASHMEM void gps_requestUpdate ()
{
	userData.rates.epochPerRead = userData.rates.epoch;
	msgPostMed(&userData, 0);
	userData.rates.epoch = 0;
}

// more than once per second
FLASHMEM void msgPostHighCb (void *opaque, const intptr_t unused)
{
	//printf("msgPostHighCb\r\n");

	gpsdata_t *data = (gpsdata_t*)opaque;
	if (data->fix.type == PVT_FIXTYPE_3D){
		if (0){		// do something with the data
			//lat = data->nav.latitude;
			//lon = data->nav.longitude;
			//alt = data->nav.altitude;
		}
	}
}

static uint16_t calcChkSum (const uint8_t *hex, const uint32_t htotal, uint8_t *c1, uint8_t *c2)
{
	*c1 = 0; *c2 = 0;
	
	for (int i = 0; i < htotal; i++){
		*c1 += hex[i];
		*c2 += *c1;
	}
	return (*c1<<8)|*c2;
}

#if 0
static void bufferDump (const uint8_t *buffer, const int32_t bufferSize)
{
	for (int i = 0; i < bufferSize; i++)
		printf("0x%02X ", buffer[i]);

	printf("\r\n");
}
#endif

static ubx_func_t *payloadHandlerFuncGet (uint8_t msg_class, uint8_t msg_id)
{
	for (int i = 0; i < MAX_REGMSG; i++){
		ubx_func_t *handler = &ubxRegTable.msg[i];
		if (!handler) continue;
		if (msg_class == handler->uClass && msg_id == handler->uId)
			return handler;
	}
	return NULL;
}

static int payloadHandlerFuncSet (const char *name, uint8_t msg_class, uint8_t msg_id, void *payloadFunc, const uint32_t status)
{
	for (int i = 0; i < MAX_REGMSG; i++){
		ubx_func_t *handler = &ubxRegTable.msg[i];
		if (!handler->uClass && !handler->uId){
			handler->name = name;
			handler->uClass = msg_class;
			handler->uId = msg_id;
			handler->func = payloadFunc;
			handler->enabled = status&0x01;
			return 1;
		}
	}
	return 0;
}

static void payloadPostCbSet (const uint8_t type, void *postCbFunc, const intptr_t unused)
{
	if (!type || type > CBFREQ_TOTAL){
		//printf("payloadPostCbSet: invalid type (%i)\r\n", type);
		return;
	}
	ubxRegTable.postCb[type-1].func = postCbFunc;
	return;
}

static int payloadHandlerSet (const char *name, uint8_t msg_class, uint8_t msg_id, void *payloadFunc, const uint32_t status)
{
	if (!payloadHandlerFuncGet(msg_class, msg_id))
		return payloadHandlerFuncSet(name, msg_class, msg_id, payloadFunc, status);
	//else
	//	printf("payloadHandlerSet() failed: %X/%X\r\n", msg_class, msg_id);
	return 0;
}

static inline ubx_func_t *payloadHandlerGet (uint8_t msg_class, uint8_t msg_id)
{
	return payloadHandlerFuncGet(msg_class, msg_id);
}

static void payloadOpaqueSet (void *ptr)
{
	ubxRegTable.userDataPtr = ptr;
}

static void *payloadOpaqueGet ()
{
	return ubxRegTable.userDataPtr;
}

static inline int payloadEnable (uint8_t msg_class, uint8_t msg_id)
{
	ubx_func_t *handler = payloadHandlerFuncGet(msg_class, msg_id);
	if (handler){
		handler->enabled = 1;
		return 1;
	}
	return 0;
}

static inline int payloadDisable (uint8_t msg_class, uint8_t msg_id)
{
	ubx_func_t *handler = payloadHandlerFuncGet(msg_class, msg_id);
	if (handler){
		handler->enabled = 0;
		return 1;
	}
	return 0;
}

static int payloadHandlerFunc (ubx_func_t *handler, const uint8_t *payload, const uint16_t msg_len)
{
	//if (handler->enabled){
		const int32_t type = handler->func(payload, msg_len, ubxRegTable.userDataPtr);
		if (type == CBFREQ_INVALID){		// payload wasn't what was expected
			// do nothing
			return 0;

		}else if (type != CBFREQ_NONE){
			ubxRegTable.postCb[type-1].func(payloadOpaqueGet(), 0);
		}
		return 1;
	//}
	//return 0;
}

static void ubxPayloadDispatch (uint8_t msg_class, uint8_t msg_id, uint16_t msg_len, const uint8_t *payload)
{
	ubx_func_t *handler = payloadHandlerFuncGet(msg_class, msg_id);
	if (handler){
		handler->ct++;
		
		if (handler->enabled){
			if (handler->alert&ALERT_BEFORE){
				// fire off alert
				// do something
			}

			if (payloadHandlerFunc(handler, payload, msg_len)){
				if (handler->alert&ALERT_AFTER){
					// fire off alert
					// do something here
					//printf("#### ubxPayloadDispatch: Alert signaled for %X/%X\r\n", msg_class, msg_id);
				}
			}
			
			if (handler->alert&ALERT_SINGLE){
				handler->alert = ALERT_DISABLED;
			}
		}
	}else{
		//printf("ubxPayloadDispatch: unknown class/id: %.2X/%.2X\r\n", msg_class, msg_id);	
	}
}

int ubxBufferFrameProcess (uint8_t *buffer, const int32_t bufferSize)
{
	const uint8_t msg_class = buffer[0];
	const uint8_t msg_id = buffer[1];
	const uint16_t msg_len = (buffer[3]<<8) | buffer[2];
	const uint16_t msg_crc = (buffer[3+msg_len+1]<<8) | buffer[3+msg_len+2];

	int frameEnd = 4+msg_len+2; 
	if (frameEnd > bufferSize){
		//printf("multipart frame: %i %i\r\n", (int)frameEnd, (int)bufferSize);
		return -1;
	}

	uint8_t c1 = 0;
	uint8_t c2 = 0;
	const uint16_t crc = calcChkSum(buffer, msg_len+4, &c1, &c2);
	if (msg_crc != crc){
		//printf("class: %.2X/%.2X\r\n", msg_class, msg_id);
		//printf("len: %i\r\n", (int)msg_len);
		//printf("crc mismatch: %X, %X\r\n", msg_crc, crc);
		return 0;
	}	

	//we have a valid frame
	ubxPayloadDispatch(msg_class, msg_id, msg_len, &buffer[4]);
	userData.rates.msgCt++;
	
	return msg_len+4+2;
}

static inline int writeBin (ubx_device_t *dev, uint8_t *buffer, const uint32_t bufferSize)
{
	userData.rates.tx += bufferSize;
	return gps_serialWrite(buffer, bufferSize);
}

int ubx_write (ubx_device_t *dev, uint8_t *buffer, const uint32_t bufferSize)
{
	return writeBin(dev, buffer, bufferSize);
}

static int ubx_send (ubx_device_t *dev, const uint8_t *buffer, const uint16_t len)
{
	if (len < 3){
		//printf("ubx_send: invalid msg length (%i)\r\n", (int)len);
		return 0;
	}
	
	uint8_t ubxMsg[len+4];
	memset(ubxMsg, 0, sizeof(ubxMsg));
	
	ubxMsg[0] = MSG_UBX_B1;
	ubxMsg[1] = MSG_UBX_B2;
	memcpy(&ubxMsg[2], buffer, len);
	calcChkSum(buffer, len, &ubxMsg[sizeof(ubxMsg)-2], &ubxMsg[sizeof(ubxMsg)-1]);
	
	int ret = writeBin(dev, ubxMsg, sizeof(ubxMsg));
	ms_delay(1);
	return ret; 
}

static int ubx_sendEx (ubx_device_t *dev, const uint16_t waitMs, const uint8_t clsId, const uint8_t msgId, const void *buffer, const uint16_t len)
{
	if (len < 1){
		//printf("ubx_sendEx: invalid msg length (%i)\r\n", (int)len);
		return 0;
	}
	
	size_t blen = len + 8;	// include space for ubx ident, class/msg Id and CRC
	uint8_t ubxMsg[blen];
	//memset(ubxMsg, 0, sizeof(ubxMsg));
	
	ubxMsg[0] = MSG_UBX_B1;
	ubxMsg[1] = MSG_UBX_B2;
	ubxMsg[2] = clsId;
	ubxMsg[3] = msgId;
	ubxMsg[4] = (len&0x00FF);
	ubxMsg[5] = (len&0xFF00)>>8;
	memcpy(&ubxMsg[6], buffer, len);
	
	// crc includes class/msgId through to data [buffer] end
	// crc excludes ubx ident and crc byte space
	calcChkSum(&ubxMsg[2], blen-4, &ubxMsg[sizeof(ubxMsg)-2], &ubxMsg[sizeof(ubxMsg)-1]);
	//serialBufferDump(ubxMsg, sizeof(ubxMsg));
	
	int ret = writeBin(dev, ubxMsg, sizeof(ubxMsg));
	if (waitMs) ms_delay(waitMs);
	return ret;
}

FLASHMEM void ubx_msgPoll (ubx_device_t *dev, const uint8_t clsId, const uint8_t msgId)
{
	const uint8_t buffer[] = {clsId, msgId, 0x00, 0x00};
	ubx_send(dev, buffer, sizeof(buffer));
}

FLASHMEM void ubx_msgEnableEx (ubx_device_t *dev, const uint8_t clsId, const uint8_t msgId, const uint8_t rate)
{
	const uint8_t buffer[] = {UBX_CFG, UBX_CFG_MSG, 0x03, 0x00, clsId, msgId, rate};
	ubx_send(dev, buffer, sizeof(buffer));
}

static void inline ubx_msgEnable (ubx_device_t *dev, const uint8_t clsId, const uint8_t msgId)
{
	ubx_msgEnableEx(dev, clsId, msgId, 1);
}

static inline void ubx_msgDisable (ubx_device_t *dev, const uint8_t clsId, const uint8_t msgId)
{
	const uint8_t buffer[] = {UBX_CFG, UBX_CFG_MSG, 0x03, 0x00, clsId, msgId, 0x00};
	ubx_send(dev, buffer, sizeof(buffer));
}

FLASHMEM void ubx_msgDisableAll (ubx_device_t *dev)
{
	const uint8_t buffer[] = {UBX_CFG, UBX_CFG_MSG, 0x03, 0x00, 0xFF, 0xFF, 0x00};
	ubx_send(dev, buffer, sizeof(buffer));
}

static inline void ubx_msgEnableAll (ubx_device_t *dev)
{
	const uint8_t buffer[] = {UBX_CFG, UBX_CFG_MSG, 0x03, 0x00, 0xFF, 0xFF, 0x01};
	ubx_send(dev, buffer, sizeof(buffer));
}

static inline void disableUnknowns (ubx_device_t *dev)
{
#if 0
	ubx_msgDisable(dev, 0x03, 0x09);
	ubx_msgDisable(dev, 0x03, 0x11);
	ubx_msgDisable(dev, 0x09, 0x13);
	ubx_msgDisable(dev, 0x0C, 0x10);
	ubx_msgDisable(dev, 0x0C, 0x1A);
	ubx_msgDisable(dev, 0x0C, 0x31);
	ubx_msgDisable(dev, 0x0C, 0x34);
	ubx_msgDisable(dev, 0x0C, 0x35);
	ubx_msgDisable(dev, 0x27, 0x00);
	ubx_msgDisable(dev, UBX_SEC, 0x00);
	ubx_msgDisable(dev, UBX_RXM, 0x23);
	ubx_msgDisable(dev, UBX_RXM, 0x51);
	ubx_msgDisable(dev, UBX_RXM, 0x52);
	ubx_msgDisable(dev, UBX_RXM, 0x57);
	ubx_msgDisable(dev, UBX_NAV, 0x36);
	ubx_msgDisable(dev, UBX_MON, 0x0D);
	ubx_msgDisable(dev, UBX_MON, 0x11);
	ubx_msgDisable(dev, UBX_MON, 0x1B);
	ubx_msgDisable(dev, UBX_MON, 0x1D);
	ubx_msgDisable(dev, UBX_MON, 0x1E);
	ubx_msgDisable(dev, UBX_MON, 0x1F);
	ubx_msgDisable(dev, UBX_MON, 0x22);
	ubx_msgDisable(dev, UBX_MON, 0x23);
	ubx_msgDisable(dev, UBX_MON, 0x25);
	ubx_msgDisable(dev, UBX_MON, 0x26);
	ubx_msgDisable(dev, UBX_MON, 0x2B);
#endif
}

int searchSync (uint8_t *buffer, uint32_t len)
{
	if (len > 1){
		for (int i = 0; i < len-1; i++){
			if (buffer[i] == MSG_UBX_B1 && buffer[i+1] == MSG_UBX_B2)
				return i;
		}
	}
	return -1;
}

int searchFrame (uint8_t *buffer, const int32_t bufferSize)
{
	//const uint8_t msg_class = buffer[0];
	//const uint8_t msg_id = buffer[1];
	const uint16_t msg_len = (buffer[3]<<8) | buffer[2];
	const uint16_t msg_crc = (buffer[3+msg_len+1]<<8) | buffer[3+msg_len+2];

	int frameEnd = 4+msg_len+2; 
	if (frameEnd > bufferSize)
		return 0;
	
	uint8_t c1 = 0;
	uint8_t c2 = 0;
	const uint16_t crc = calcChkSum(buffer, msg_len+4, &c1, &c2);
	if (msg_crc != crc)
		return -(msg_len+4+2);
	
	return msg_len+4+2;
}

uint32_t gps_ubxMsgRun (ubx_device_t *dev, uint8_t *inBuffer, uint32_t bufferSize, int32_t *writePos, uint8_t *serBuffer, uint8_t serLen)
{
	uint32_t writeLength = 0;
	uint32_t bytesRead = 0;
	static uint8_t readBuffer[8] = {0}; // Should be no larger than the smallest [message+control bytes]


	if (*writePos > bufferSize){
		*writePos = 0;
		memset(inBuffer, 0, bufferSize);
	}
	
	memcpy(readBuffer, serBuffer, serLen);
	bytesRead = serLen;
	if (bytesRead > 0){
		userData.rates.rx += bytesRead;

		for (int i = 0; i < bytesRead; i++)
			inBuffer[(*writePos)++] = readBuffer[i];

		writeLength = *writePos;
		if (writeLength < 6) return 0;
		
		// check if there is a complete message
		int foundSync = searchSync(inBuffer, writeLength);
		if (foundSync < 0){
			//printf("sync not found %i %i\r\n", foundSync, writeLength);
			//bufferDump(inBuffer, writeLength);
			return 0;
		}

		// advance past sync bytes
		foundSync += 2;

		int foundFrame = searchFrame(&inBuffer[foundSync], writeLength-foundSync);
		if (foundFrame > 0){
			int process = ubxBufferFrameProcess(&inBuffer[foundSync], writeLength-foundSync);

			if (foundSync+foundFrame < writeLength){
				for (int i = 0; i < sizeof(readBuffer); i++)
					inBuffer[i] = inBuffer[foundSync+foundFrame+i];

				*writePos = writeLength - (foundSync+foundFrame);
				memset(inBuffer+sizeof(readBuffer), 0, bufferSize-sizeof(readBuffer));
			}else{
				memset(inBuffer, 0, bufferSize);
				*writePos = 0;
			}
			return (process > 0);

		}else if (foundFrame < 0){
			//printf("mismatch crc %i, %i %i\r\n", -foundFrame, *writePos, foundSync);
			//bufferDump(inBuffer, 32);
			memset(inBuffer, 0, bufferSize);
			*writePos = 0;
		}
	}
	return 0;
}

static inline void ubx_rst_hotStart (ubx_device_t *dev)
{
	cfg_rst_t rst = {0};
	
	rst.navBbrMask = RST_BBRMASK_HOTSTART;
	rst.resetMode = RST_RESETMODE_SWGNSSONLY;
	
	ubx_sendEx(dev, 10, UBX_CFG, UBX_CFG_RST, &rst, sizeof(rst));	
}

static inline void ubx_rst_warmStart (ubx_device_t *dev)
{
	cfg_rst_t rst = {0};
	
	rst.navBbrMask = RST_BBRMASK_WARMSTART;
	rst.resetMode = RST_RESETMODE_SWGNSSONLY;
	
	ubx_sendEx(dev, 10, UBX_CFG, UBX_CFG_RST, &rst, sizeof(rst));	
}

static inline void ubx_rst_coldStart (ubx_device_t *dev)
{
	cfg_rst_t rst = {0};
	
	rst.navBbrMask = RST_BBRMASK_COLDSTART;
	rst.resetMode = RST_RESETMODE_SWGNSSONLY;
	
	ubx_sendEx(dev, 10, UBX_CFG, UBX_CFG_RST, &rst, sizeof(rst));	
}

static void configureGNSS (ubx_device_t *dev)
{

	const int cfgBlks = 7;
	const int glen = CFG_GNSS_SIZE(cfgBlks);
	uint8_t _gnss[glen];
	cfg_gnss_t *gnss = (cfg_gnss_t*)_gnss;

	cfg_cfgblk_t *cfg = &gnss->cfgblk[0];
	cfg->gnssId = GNSSID_GPS;
	cfg->resTrkCh = 8;
	cfg->maxTrkCh = 24;
	cfg->flags = GNSS_CFGBLK_ENABLED | GNSS_CFGBLK_SIGENABLED;
	
	cfg = &gnss->cfgblk[1];
	cfg->gnssId = GNSSID_GLONASS;
	cfg->resTrkCh = 8;
	cfg->maxTrkCh = 24;
	cfg->flags = GNSS_CFGBLK_ENABLED | GNSS_CFGBLK_SIGENABLED;
	
	cfg = &gnss->cfgblk[2];
	cfg->gnssId = GNSSID_GALILEO;
	cfg->resTrkCh = 6;
	cfg->maxTrkCh = 10;
	cfg->flags = GNSS_CFGBLK_DISABLED | GNSS_CFGBLK_SIGENABLED;

	cfg = &gnss->cfgblk[3];
	cfg->gnssId = GNSSID_BEIDOU;
	cfg->resTrkCh = 6;
	cfg->maxTrkCh = 20;
	cfg->flags = GNSS_CFGBLK_DISABLED | GNSS_CFGBLK_SIGENABLED;
	
	cfg = &gnss->cfgblk[4];
	cfg->gnssId = GNSSID_IMES;
	cfg->resTrkCh = 0;
	cfg->maxTrkCh = 8;
	cfg->flags = GNSS_CFGBLK_DISABLED | GNSS_CFGBLK_SIGENABLED;
	
	cfg = &gnss->cfgblk[5];
	cfg->gnssId = GNSSID_QZSS;
	cfg->resTrkCh = 1;
	cfg->maxTrkCh = 8;
	cfg->flags = GNSS_CFGBLK_DISABLED | GNSS_CFGBLK_SIGENABLED;
	
	cfg = &gnss->cfgblk[6];
	cfg->gnssId = GNSSID_SBAS;
	cfg->resTrkCh = 1;
	cfg->maxTrkCh = 4;
	cfg->flags = GNSS_CFGBLK_DISABLED | GNSS_CFGBLK_SIGENABLED;

	gnss->msgVer = 0;
	gnss->numTrkChHw = 72;
	gnss->numTrkChUse = 32;
	gnss->numConfigBlocks = cfgBlks;

	ubx_sendEx(dev, 100, UBX_CFG, UBX_CFG_GNSS, gnss, glen);
}

static void configurePorts (ubx_device_t *dev)
{
	cfg_prt_uart_t prt = {0};
	
	prt.portId = CFG_PORTID_UART1;
	prt.mode.bits.charLen = CFG_CHARLEN_8BIT;
	prt.mode.bits.nStopBits = CFG_STOPBITS_1;
	prt.mode.bits.partity = CFG_PARTITY_NONE;
	prt.mode.bits.bitOrder = CFG_BITORDER_LSB;
	prt.baudRate = BAUDRATE(COM_BAUD);
	prt.inProtoMask = CFG_PROTO_UBX;
	prt.outProtoMask = CFG_PROTO_UBX;

	ubx_sendEx(dev, 10, UBX_CFG, UBX_CFG_PRT, &prt, sizeof(prt));
}

void gps_configurePorts (ubx_device_t *dev)
{
	configurePorts(dev);
}

static void configureRate (ubx_device_t *dev)
{
	cfg_rate_t rate = {0};
	
	rate.measRate = 57;		// ms. 53ms = ~18-19hz
	rate.navRate = 1;		// 1 measurement per navigation
	rate.timeRef = CFG_TIMEREF_UTC;
	
	ubx_sendEx(dev, 10, UBX_CFG, UBX_CFG_RATE, &rate, sizeof(rate));
}

static void configureNav5 (ubx_device_t *dev)
{
	cfg_nav5_t nav;
	memset(&nav, 0, sizeof(nav));
	
	nav.mask  = NAV5_MASK_DYN | NAV5_MASK_MINEL | NAV5_MASK_POSFIXMODE | NAV5_MASK_DRLIM;
	nav.mask |= NAV5_MASK_POSMASK | NAV5_MASK_TIMEMASK | NAV5_MASK_STATICHOLDMASK;
	nav.mask |= NAV5_MASK_DGPSMASK | NAV5_MASK_CNOTHRESHOLD | NAV5_MASK_UTC;
		
		
	nav.dynModel = NAV5_DYNMODEL_WRIST;	// STATIONARY PORTABLE WRIST PEDESTRIAN;
	nav.fixMode = NAV5_FIXMODE_AUTO;
	nav.fixedAlt = 37.0f * 100;				// meters, when using NAV5_FIXMODE_2D
	nav.fixedAltVar = 0.5f * 10000;			// deviation,  ^^^ 
	nav.minElv = 5;
	nav.drLimit = 0;
	nav.pDop = 25.0f * 10;
	nav.tDop = 25.0f * 10;
	nav.pAcc = 100;
	nav.tAcc = 350;
	nav.pAccADR = 0;						// also known as reserved1;
	nav.dynssTimeout = 60;
	nav.cnoThreshNumSVs = 0;
	nav.cnoThresh = 0;
	nav.staticHoldThresh = 50;				// 50 cm/s 
	nav.staticHoldMaxDist = 2;				// 2 meters
	nav.utcStandard = NAV5_UTCSTD_AUTO;
	
	ubx_sendEx(dev, 10, UBX_CFG, UBX_CFG_NAV5, &nav, sizeof(nav));
}

static void configureNavX5 (ubx_device_t *dev)
{
	cfg_navx5_t nav;
	memset(&nav, 0, sizeof(nav));
	
	nav.version = NAVX5_VERSION_PROTO18;
	nav.mask1  = NAVX5_MASK1_MINMAX | NAVX5_MASK1_MINCNO | NAVX5_MASK1_INITIAL3DFIX;
	nav.mask1 |= NAVX5_MASK1_WKNROLL | NAVX5_MASK1_ACKAID | NAVX5_MASK1_PPP | NAVX5_MASK1_AOP;
	nav.mask2 = NAVX5_MASK2_ADR;
	nav.minSVs = 6;
	nav.maxSVs = 32;
	nav.minCNO = 6;
	nav.iniFix3D = 0;
	nav.ackAiding = 0;
	nav.wknRollover = 1867;						// 0 = firmware default.
	nav.sigAttenCompMode = NAVX5_SACM_AUTO;		
	nav.usePPP = 0;
	nav.aopCfg = NAVX5_AOPCFG_USEAOP;
	nav.aopOrbMaxErr = 100;
	nav.useAdr = 0;
	
	ubx_sendEx(dev, 10, UBX_CFG, UBX_CFG_NAVX5, &nav, sizeof(nav));
}

FLASHMEM void ubx_msgInfPoll (ubx_device_t *dev, const uint8_t protocolID)
{
	cfg_inf_poll_t inf = {0};
	inf.protocolID = protocolID;
	
	ubx_sendEx(dev, 1, UBX_CFG, UBX_CFG_INF, &inf, sizeof(inf));
}

static inline void ubx_msgInfEnable (ubx_device_t *dev, const uint8_t portId, const uint8_t infmsg)
{
	cfg_inf_t inf = {0};
	inf.protocolID = portId;
	inf.infMsgMask[CFG_PORTID_I2C] = 0;
	inf.infMsgMask[CFG_PORTID_UART1] = infmsg;
	inf.infMsgMask[CFG_PORTID_UART2] = 0;
	inf.infMsgMask[CFG_PORTID_USB] = 0;
	inf.infMsgMask[CFG_PORTID_SPI] = 0;
	
	ubx_sendEx(dev, 1, UBX_CFG, UBX_CFG_INF, &inf, sizeof(inf));
}

static inline void ubx_msgInfDisable (ubx_device_t *dev, const uint8_t portId)
{
	
	cfg_inf_t inf;
	memset(&inf, 0, sizeof(inf));
	
	inf.protocolID = portId;
	inf.infMsgMask[CFG_PORTID_I2C] = 0;
	inf.infMsgMask[CFG_PORTID_UART1] = 0;
	inf.infMsgMask[CFG_PORTID_UART2] = 0;
	inf.infMsgMask[CFG_PORTID_USB] = 0;
	inf.infMsgMask[CFG_PORTID_SPI] = 0;
	
	ubx_sendEx(dev, 10, UBX_CFG, UBX_CFG_INF, &inf, sizeof(inf));
}

static void ubx_msgInfDisableAll (ubx_device_t *dev)
{
	
	cfg_inf_t inf;
	memset(&inf, 0, sizeof(inf));
	
	inf.infMsgMask[CFG_PORTID_I2C] = 0;
	inf.infMsgMask[CFG_PORTID_UART1] = 0;
	inf.infMsgMask[CFG_PORTID_UART2] = 0;
	inf.infMsgMask[CFG_PORTID_USB] = 0;
	inf.infMsgMask[CFG_PORTID_SPI] = 0;

	inf.protocolID = INF_PROTO_UBX;
	ubx_sendEx(dev, 1, UBX_CFG, UBX_CFG_INF, &inf, sizeof(inf));

	inf.protocolID = INF_PROTO_NEMA;
	ubx_sendEx(dev, 1, UBX_CFG, UBX_CFG_INF, &inf, sizeof(inf));
	
	inf.protocolID = INF_PROTO_RAW;
	ubx_sendEx(dev, 1, UBX_CFG, UBX_CFG_INF, &inf, sizeof(inf));
	
	inf.protocolID = INF_PROTO_RTCM3;
	ubx_sendEx(dev, 1, UBX_CFG, UBX_CFG_INF, &inf, sizeof(inf));
	
	inf.protocolID = INF_PROTO_USER0;
	ubx_sendEx(dev, 1, UBX_CFG, UBX_CFG_INF, &inf, sizeof(inf));
	
	inf.protocolID = INF_PROTO_USER1;
	ubx_sendEx(dev, 1, UBX_CFG, UBX_CFG_INF, &inf, sizeof(inf));
	
	inf.protocolID = INF_PROTO_USER2;
	ubx_sendEx(dev, 1, UBX_CFG, UBX_CFG_INF, &inf, sizeof(inf));
	
	inf.protocolID = INF_PROTO_USER3;
	ubx_sendEx(dev, 10, UBX_CFG, UBX_CFG_INF, &inf, sizeof(inf));
	
}

static void configureInf (ubx_device_t *dev)
{
	//ubx_msgInfPoll(dev, INF_PROTO_UBX);
	//ubx_msgInfPoll(dev, INF_PROTO_NEMA);
	ubx_msgInfDisableAll(dev);
	//ubx_msgInfEnable(dev, INF_PROTO_UBX, INF_MSG_WARNING|INF_MSG_NOTICE|INF_MSG_DEBUG);
}

FLASHMEM void ubx_odo_reset (ubx_device_t *dev)
{
	const uint8_t buffer[] = {UBX_NAV, UBX_NAV_RESETODO, 0, 0};
	ubx_send(dev, buffer, sizeof(buffer));
	ms_delay(10);
}

FLASHMEM void ubx_odo_start (ubx_device_t *dev)
{
	cfg_odo_t odo = {0};
	memset(&odo, 0, sizeof(odo));
	
	odo.version = 0;
	odo.flags = ODO_FLAGS_USEODO;

	ubx_sendEx(dev, 10, UBX_CFG, UBX_CFG_ODO, &odo, sizeof(odo));
}

FLASHMEM void ubx_odo_stop (ubx_device_t *dev)
{
	cfg_odo_t odo = {0};
	memset(&odo, 0, sizeof(odo));
	
	odo.version = 0;
	odo.flags = 0;

	ubx_sendEx(dev, 10, UBX_CFG, UBX_CFG_ODO, &odo, sizeof(odo));
}

FLASHMEM static void configureOdo (ubx_device_t *dev)
{
	cfg_odo_t odo;
	memset(&odo, 0, sizeof(odo));
	
	odo.version = 0;
	odo.flags = ODO_FLAGS_USEODO|ODO_FLAGS_OUTLPVEL; //|ODO_FLAGS_USECOG|ODO_FLAGS_OUTLPCOG;
	odo.odoCfg = ODO_PROFILE_RUNNING;
	odo.cogMaxSpeed = 10 * 5.0f;
	odo.cogMaxPosAcc = 15;
	odo.velLpGain = 255 * 0.60f;
	odo.cogLpGain = 255 * 0.70f;
	
	ubx_sendEx(dev, 10, UBX_CFG, UBX_CFG_ODO, &odo, sizeof(odo));
}

static void configureGeofence (ubx_device_t *dev)
{
	const int geoFences = 2;
	const int glen = CFG_GEOFENCE_SIZE(geoFences);
	uint8_t _geo[glen];
	cfg_geofence_t *geo = (cfg_geofence_t*)_geo;
	
	
	geo->version = 0;
	geo->numFences = geoFences;
	geo->confLvl = GEOFENCE_CONFIDIENCE_99999;
	geo->pioEnabled = 0;
	geo->pinPolarity = 0;
	geo->pin = 0;
	
	// example
	geo->fence[0].lat = FLT2UBX(54.6161497);
	geo->fence[0].lon = FLT2UBX(-5.9366867);
	geo->fence[0].radius = 25 * 100;

	geo->fence[1].lat = FLT2UBX(54.619988);
	geo->fence[1].lon = FLT2UBX(-6.502265);
	geo->fence[1].radius = 25 * 100;	
	
	ubx_sendEx(dev, 10, UBX_CFG, UBX_CFG_GEOFENCE, geo, glen);
}

void setHNR (ubx_device_t *dev, uint8_t rate)
{
	cfg_hnr_t hnr = {0};

	hnr.highNavRate = rate;
	ubx_sendEx(dev, 10, UBX_CFG, UBX_CFG_HNR, &hnr, sizeof(hnr));
}

static void configureHNR (ubx_device_t *dev)
{
	setHNR(dev, 17);	// set rate to 17hz
}

FLASHMEM void ubx_printVersions (ubx_device_t *dev)
{
	ubx_msgPoll(dev, UBX_MON, UBX_MON_VER);
	ubx_msgPoll(dev, UBX_CFG, UBX_CFG_USB);
}

FLASHMEM void ubx_printStatus (ubx_device_t *dev)
{
	ubx_msgPoll(dev, UBX_CFG, UBX_CFG_NAV5);

	ubx_msgPoll(dev, UBX_CFG, UBX_CFG_INF);
	ubx_msgPoll(dev, UBX_CFG, UBX_CFG_GEOFENCE);
	ubx_msgPoll(dev, UBX_CFG, UBX_CFG_NAVX5);
	ubx_msgPoll(dev, UBX_CFG, UBX_CFG_PRT);
	ubx_msgPoll(dev, UBX_NAV, UBX_NAV_GEOFENCE);
	ubx_msgPoll(dev, UBX_NAV, UBX_NAV_STATUS);
	ubx_msgPoll(dev, UBX_CFG, UBX_CFG_GNSS);
	ubx_msgPoll(dev, UBX_CFG, UBX_CFG_RATE);
}

FLASHMEM void ubx_hotStart (ubx_device_t *dev)
{
	ubx_rst_hotStart(dev);
}

FLASHMEM void ubx_warmStart (ubx_device_t *dev)
{
	ubx_rst_warmStart(dev);
}

FLASHMEM void ubx_coldStart (ubx_device_t *dev)
{
	ubx_rst_coldStart(dev);
}

static inline int ubxNameToClass (const char *name, uint8_t *uClass, uint8_t *uId)
{
	for (int i = 0; i < MAX_REGMSG; i++){
		ubx_func_t *handler = &ubxRegTable.msg[i];
		if (!handler || !handler->name) continue;
		
		if (!strcmp(handler->name, name)){
			*uClass = handler->uClass;
			*uId = handler->uId;
			return 1;
		}
	}
	return 0;
}

int ubx_msgPollName (ubx_device_t *dev, const char *name)
{
	uint8_t clsId;
	uint8_t msgId;
	
	if (ubxNameToClass(name, &clsId, &msgId)){
		ubx_msgPoll(dev, clsId, msgId);
		return 1;
	}
	return 0;
}

FLASHMEM void ubx_sos_backup (ubx_device_t *dev)
{
	updsos_cmd_t upd;
	upd.cmd = UPDSOS_CMD_CREATE;
	
	ubx_sendEx(dev, 10, UBX_UPD, UBX_UPD_SOS, &upd, sizeof(upd));
}

FLASHMEM void ubx_sos_poll (ubx_device_t *dev)
{
	ubx_msgPoll(dev, UBX_UPD, UBX_UPD_SOS);
}

FLASHMEM void ubx_sos_clear (ubx_device_t *dev)
{
	updsos_cmd_t upd;
	upd.cmd = UPDSOS_CMD_CLEAR;
	
	ubx_sendEx(dev, 10, UBX_UPD, UBX_UPD_SOS, &upd, sizeof(upd));
}

FLASHMEM void ubx_mga_ini_posllh (ubx_device_t *dev, const double lat, const double lon, const float alt_meters, const uint32_t posAcc_cm)
{
	mga_ini_posllh_t posllh;
	memset(&posllh, 0, sizeof(posllh));
	
	posllh.type = 0x01;
	posllh.version = 0x00;
	posllh.latitude = lat * 10000000.0;
	posllh.longitude = lon * 10000000.0;
	posllh.altitude = alt_meters * 100.0f;
	posllh.posAcc = posAcc_cm;
	
	ubx_sendEx(dev, 10, UBX_MGA, UBX_MGA_INI_POSLLH, &posllh, sizeof(posllh));
}

FLASHMEM void gps_configure (ubx_device_t *dev)
{
	memset(&userData, 0, sizeof(userData));
	memset(&ubxRegTable, 0, sizeof(ubxRegTable));
	
	payloadOpaqueSet(&userData);
	
	payloadPostCbSet(CBFREQ_LOW,      msgPostLowCb,  0);
	payloadPostCbSet(CBFREQ_MEDIUM,   msgPostMedCb,  0);
	payloadPostCbSet(CBFREQ_HIGH,     msgPostHighCb, 0);

	payloadHandlerSet("ack_nak",      UBX_ACK, UBX_ACK_NAK,      ack_nak,      MSG_STATUS_DISABLED);
	payloadHandlerSet("ack_ack",      UBX_ACK, UBX_ACK_ACK,      ack_ack,      MSG_STATUS_DISABLED);
	payloadHandlerSet("nav_geofence", UBX_NAV, UBX_NAV_GEOFENCE, nav_geofence, MSG_STATUS_ENABLED);
	payloadHandlerSet("nav_posecef",  UBX_NAV, UBX_NAV_POSECEF,  nav_posecef,  MSG_STATUS_ENABLED);
	payloadHandlerSet("nav_dop",      UBX_NAV, UBX_NAV_DOP,      nav_dop,      MSG_STATUS_ENABLED);
	payloadHandlerSet("nav_pvt",      UBX_NAV, UBX_NAV_PVT,      nav_pvt,      MSG_STATUS_ENABLED);
	payloadHandlerSet("nav_svinfo",   UBX_NAV, UBX_NAV_SVINFO,   nav_svinfo,   MSG_STATUS_ENABLED);
	payloadHandlerSet("nav_status",   UBX_NAV, UBX_NAV_STATUS,   nav_status,   MSG_STATUS_ENABLED);	
	payloadHandlerSet("nav_sat",      UBX_NAV, UBX_NAV_SAT,      nav_sat,      MSG_STATUS_ENABLED);
	payloadHandlerSet("nav_eoe",      UBX_NAV, UBX_NAV_EOE,      nav_eoe,      MSG_STATUS_ENABLED);
	payloadHandlerSet("nav_posllh",   UBX_NAV, UBX_NAV_POSLLH,   nav_posllh,   MSG_STATUS_ENABLED);
	payloadHandlerSet("aid_alm",      UBX_AID, UBX_AID_ALM,      aid_alm,      MSG_STATUS_ENABLED);
	payloadHandlerSet("aid_aop",      UBX_AID, UBX_AID_AOP,      aid_aop,      MSG_STATUS_ENABLED);
	payloadHandlerSet("aid_eph",      UBX_AID, UBX_AID_EPH,      aid_eph,      MSG_STATUS_ENABLED);
	payloadHandlerSet("mon_ver",      UBX_MON, UBX_MON_VER,      mon_ver,      MSG_STATUS_ENABLED);
	payloadHandlerSet("mon_io",       UBX_MON, UBX_MON_IO,       mon_io,       MSG_STATUS_ENABLED);
	payloadHandlerSet("rxm_sfrbx",    UBX_RXM, UBX_RXM_SFRBX,    rxm_sfrbx,    MSG_STATUS_ENABLED);
	payloadHandlerSet("inf_error",    UBX_INF, UBX_INF_ERROR,    inf_debug,    MSG_STATUS_ENABLED);
	payloadHandlerSet("inf_warning",  UBX_INF, UBX_INF_WARNING,  inf_debug,    MSG_STATUS_ENABLED);
	payloadHandlerSet("inf_notice",   UBX_INF, UBX_INF_NOTICE,   inf_debug,    MSG_STATUS_ENABLED);
	payloadHandlerSet("inf_test",     UBX_INF, UBX_INF_TEST,     inf_debug,    MSG_STATUS_ENABLED);
	payloadHandlerSet("inf_debug",    UBX_INF, UBX_INF_DEBUG,    inf_debug,    MSG_STATUS_ENABLED);	
	payloadHandlerSet("cfg_inf",      UBX_CFG, UBX_CFG_INF,      cfg_inf,      MSG_STATUS_ENABLED);
	payloadHandlerSet("cfg_rate",     UBX_CFG, UBX_CFG_RATE,     cfg_rate,     MSG_STATUS_ENABLED);
	payloadHandlerSet("cfg_nav5",     UBX_CFG, UBX_CFG_NAV5,     cfg_nav5,     MSG_STATUS_ENABLED);	
	payloadHandlerSet("cfg_navx5",    UBX_CFG, UBX_CFG_NAVX5,    cfg_navx5,    MSG_STATUS_ENABLED);	
	payloadHandlerSet("cfg_gnss",     UBX_CFG, UBX_CFG_GNSS,     cfg_gnss,     MSG_STATUS_ENABLED);	
	payloadHandlerSet("cfg_geofence", UBX_CFG, UBX_CFG_GEOFENCE, cfg_geofence, MSG_STATUS_ENABLED);
	payloadHandlerSet("cfg_prt",      UBX_CFG, UBX_CFG_PRT,      cfg_prt,      MSG_STATUS_ENABLED);
	payloadHandlerSet("cfg_usb",      UBX_CFG, UBX_CFG_USB,      cfg_usb,      MSG_STATUS_ENABLED);
	payloadHandlerSet("cfg_odo",      UBX_CFG, UBX_CFG_ODO,      cfg_odo,      MSG_STATUS_ENABLED);
	payloadHandlerSet("nav_odo",      UBX_NAV, UBX_NAV_ODO,      nav_odo,      MSG_STATUS_ENABLED);
	payloadHandlerSet("nav_timebds",  UBX_NAV, UBX_NAV_TIMEBDS,  nav_timebds,  MSG_STATUS_ENABLED);
	payloadHandlerSet("nav_sbas",     UBX_NAV, UBX_NAV_SBAS,     nav_sbas,     MSG_STATUS_ENABLED);	
	payloadHandlerSet("upd_sos",      UBX_UPD, UBX_UPD_SOS,      upd_sos,      MSG_STATUS_ENABLED);	
	payloadHandlerSet("nav_velned",   UBX_NAV, UBX_NAV_VELNED,   nav_velned,   MSG_STATUS_ENABLED);	
	

	if (1) configurePorts(dev);
	if (1) configureInf(dev);
	if (1) configureRate(dev);
	if (1) configureGNSS(dev);		// will auto generate a warmStart
	if (1) configureNav5(dev);
	if (1) configureNavX5(dev);
	if (1) configureHNR(dev);
	if (1) configureOdo(dev);
	if (0) configureGeofence(dev);


	ubx_odo_reset(dev);
	ubx_msgDisableAll(dev);
	//ubx_msgEnable(dev, UBX_MON, UBX_MON_IO);
	ubx_msgPoll(dev, UBX_MON, UBX_MON_VER);
	ubx_msgPoll(dev, UBX_CFG, UBX_CFG_USB);
	//ubx_msgPoll(dev, UBX_CFG, UBX_CFG_PRT);
		
	ubx_msgEnableEx(dev, UBX_NAV, UBX_NAV_POSLLH, 1);
	ubx_msgEnableEx(dev, UBX_NAV, UBX_NAV_PVT, 3);
	ubx_msgEnableEx(dev, UBX_NAV, UBX_NAV_DOP, 18);
	ubx_msgEnableEx(dev, UBX_NAV, UBX_NAV_ODO, 10);
	ubx_msgEnableEx(dev, UBX_NAV, UBX_NAV_POSECEF, 18);
	ubx_msgEnableEx(dev, UBX_NAV, UBX_NAV_SAT, 20);

	//ubx_msgEnable(dev, UBX_NAV, UBX_NAV_EOE);
	//ubx_msgEnableEx(dev, UBX_NAV, UBX_NAV_GEOFENCE, 60);
	//ubx_msgPoll(dev, UBX_CFG, UBX_CFG_GEOFENCE);
	//ubx_msgPoll(dev, UBX_CFG, UBX_CFG_NAV5);
	//ubx_msgPoll(dev, UBX_CFG, UBX_CFG_NAVX5);
	
	//ubx_msgEnable(dev, UBX_NAV, UBX_NAV_SVINFO);
	//ubx_msgEnable(dev, UBX_NAV, UBX_NAV_STATUS);
	//ubx_msgPoll(dev, UBX_AID, UBX_AID_EPH);
	//ubx_msgPoll(dev, UBX_AID, UBX_AID_ALM);
	//ubx_msgPoll(dev, UBX_AID, UBX_AID_AOP);

	ubx_msgPoll(dev, UBX_CFG, UBX_CFG_GNSS);
	ubx_msgPoll(dev, UBX_CFG, UBX_CFG_NAV5);
	ubx_msgPoll(dev, UBX_CFG, UBX_CFG_RATE);
	ubx_msgPoll(dev, UBX_CFG, UBX_NAV_SAT);
}


