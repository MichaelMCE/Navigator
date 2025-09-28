
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

#ifndef _UBX_H_
#define _UBX_H_



/*
UBX
• Every Frame starts with a 2-byte Preamble consisting of two synchronization characters: 0xB5 0x62.
• A 1-byte Message Class field follows. A Class is a group of messages that are related to each other.
• A 1-byte Message ID field defines the message that is to follow.
• A 2-byte Length field follows. The length is defined as being that of the payload only. It does not include
	the Preamble, Message Class, Message ID, Length, or CRC fields. The number format of the length field is a
	Little-Endian unsigned 16-bit integer.
• The Payload field contains a variable number of bytes.
• The two 1-byte CK_A and CK_B fields hold a 16-bit checksum whose calculation is defined below. 
	The checksum is calculated over the Message, starting and including the CLASS field, up until, but excluding,

*/



#define MSG_UBX_B1				0xB5
#define MSG_UBX_B2				0x62
#define MSG_UBX					((MSG_UBX_B1<<16)|MSG_UBX_B2)		// µb	(micro u)
#define MSG_NEMA				0x24								// $	(dollar)


#define UBX_NAV					0x01			// Navigation Results Messages: Position, Speed, Time, Acceleration, Heading, DOP, SVs used
#define UBX_RXM					0x02			// Receiver Manager Messages: Satellite Status, RTC Status
//#define UBX_					0x03			// 03/09, 03/11
#define UBX_INF					0x04			// Information Messages: Printf-Style Messages, with IDs such as Error, Warning, Notice
#define UBX_ACK					0x05			// Ack/Nak Messages: Acknowledge or Reject messages to CFG input messages
#define UBX_CFG					0x06			// Configuration Input Messages: Set Dynamic Model, Set DOP Mask, Set Baud Rate, etc.
#define UBX_UPD					0x09			// Firmware Update Messages: Memory/Flash erase/write, Reboot, Flash identification, etc.
#define UBX_MON					0x0A			// Monitoring Messages: Communication Status, CPU Load, Stack Usage, Task Status
#define UBX_AID					0x0B			// AssistNow Aiding Messages: Ephemeris, Almanac, other A-GPS data input
//#define UBX_					0x0C			// 0C/10, 0C/1A, 0C/31, 0C/34, 0C/35
#define UBX_TIM					0x0D			// Timing Messages: Time Pulse Output, Time Mark Results
#define UBX_ESF					0x10			// External Sensor Fusion Messages: External Sensor Measurements and Status Information
#define UBX_MGA					0x13			// Multiple GNSS Assistance Messages: Assistance data for various GNSS
#define UBX_LOG					0x21			// Logging Messages: Log creation, deletion, info and retrieval
#define UBX_SEC					0x27			// Security Feature Messages
#define UBX_HNR					0x28			// High Rate Navigation Results Messages: High rate time, position, speed, heading

#define UBX_ACK_NAK				0x00
#define UBX_ACK_ACK				0x01

#define UBX_AID_REQ				0x00
#define UBX_AID_INI				0x01
#define UBX_AID_HUI				0x02
#define UBX_AID_DATA			0x10
#define UBX_AID_ALM				0x30
#define UBX_AID_EPH				0x31
#define UBX_AID_ALPSRV			0x32
#define UBX_AID_AOP				0x33
#define UBX_AID_ALP				0x50

#define UBX_LOG_ERASE			0x03
#define UBX_LOG_STRING			0x04
#define UBX_LOG_CREATE			0x07
#define UBX_LOG_INFO			0x08
#define UBX_LOG_RETRIEVE		0x09
#define UBX_LOG_RETRIEVEPOS		0x0B
//#define UBX_LOG_RETRIEVEPOSEXTRA
#define UBX_LOG_RETRIEVESTRING	0x0D
#define UBX_LOG_FINDTIME		0x0E
//#define UBX_LOG_BATCH				
#define UBX_LOG_RETRIEVEBATCH	0x10

#define UBX_INF_ERROR			0x00
#define UBX_INF_WARNING			0x01
#define UBX_INF_NOTICE			0x02
#define UBX_INF_TEST			0x03
#define UBX_INF_DEBUG			0x04

#define UBX_CFG_PRT 			0x00
#define UBX_CFG_MSG 			0x01
#define UBX_CFG_INF				0x02
#define UBX_CFG_RST				0x04
#define UBX_CFG_DAT				0x06
#define UBX_CFG_TP				0x07
#define UBX_CFG_RATE			0x08
#define UBX_CFG_CFG				0x09
#define UBX_CFG_FXN				0x0E
#define UBX_CFG_RXM 			0x11
#define UBX_CFG_EKF				0x12
#define UBX_CFG_ANT				0x13
#define UBX_CFG_SBAS			0x16
#define UBX_CFG_NMEA			0x17
#define UBX_CFG_USB 			0x1B
#define UBX_CFG_TMODE			0x1D
#define UBX_CFG_ODO 			0x1E
#define UBX_CFG_NVS				0x22
#define UBX_CFG_NAVX5			0x23
#define UBX_CFG_NAV5			0x24
#define UBX_CFG_ESFGWT			0x29
#define UBX_CFG_TP5 			0x31
#define UBX_CFG_PM	 			0x32
#define UBX_CFG_RINV			0x34
#define UBX_CFG_ITFM			0x39
#define UBX_CFG_PM2 			0x3B
#define UBX_CFG_TMODE2 			0x3D
#define UBX_CFG_GNSS			0x3E
#define UBX_CFG_LOGFILTER		0x47
#define UBX_CFG_TXSLOT 			0x53
#define UBX_CFG_HNR				0x5C
#define UBX_CFG_PWR 			0x57
#define UBX_CFG_ESRC			0x60
#define UBX_CFG_DOSC			0x61
#define UBX_CFG_SMGR 			0x62
#define UBX_CFG_GEOFENCE		0x69
#define UBX_CFG_DGNSS			0x70
#define UBX_CFG_TMODE3 			0x71
#define UBX_CFG_FIXSEED			0x84
#define UBX_CFG_DYNSEED			0x85
#define UBX_CFG_PMS 			0x86
#define UBX_CFG_VALSET			0x8A
#define UBX_CFG_VALGET			0x8B
#define UBX_CFG_VALDEL			0x8C
#define UBX_CFG_SLAS			0x8D
#define UBX_CFG_BATCH			0x93

#define UBX_TIM_TP				0x01
#define UBX_TIM_TM2				0x03
#define UBX_TIM_SVIN			0x04
#define UBX_TIM_VRFY			0x06
#define UBX_TIM_DOSG			0x11
#define UBX_TIM_TOS				0x12
#define UBX_TIM_SMEAS			0x13
#define UBX_TIM_VCOCAL			0x15
#define UBX_TIM_FCHG			0x16
#define UBX_TIM_HOC				0x17

#define UBX_ESF_MEAS			0x02
#define UBX_ESF_RAW				0x03
#define UBX_ESF_STATUS			0x10
#define UBX_ESF_INS				0x15

#define UBX_HNR_PVT				0x00

#define UBX_MON_IO				0x02
#define UBX_MON_VER				0x04
#define UBX_MON_EXCEPT			0x05
#define UBX_MON_MSGPP			0x06
#define UBX_MON_RXBUF			0x07
#define UBX_MON_TXBUF			0x08
#define UBX_MON_HW				0x09
#define UBX_MON_HW2				0x0B
//#define UBX_MON_				0x0D
//#define UBX_MON_				0x11
//#define UBX_MON_				0x1B
//#define UBX_MON_				0x1D
//#define UBX_MON_				0x1E
//#define UBX_MON_				0x1F
#define UBX_MON_RXR				0x21
//#define UBX_MON_				0x22
//#define UBX_MON_				0x23
//#define UBX_MON_				0x25
//#define UBX_MON_				0x26
#define UBX_MON_PATCH			0x27
#define UBX_MON_GNSS			0x28
//#define UBX_MON_				0x2B
#define UBX_MON_SMGR			0x2E
#define UBX_MON_BATCH			0x32
#define UBX_MON_HW3				0x37
#define UBX_MON_RF				0x38

#define UBX_NAV_POSECEF			0x01
#define UBX_NAV_POSLLH			0x02
#define UBX_NAV_STATUS			0x03
#define UBX_NAV_DOP				0x04
#define UBX_NAV_ATT				0x05
#define UBX_NAV_SOL				0x06
#define UBX_NAV_PVT				0x07
#define UBX_NAV_ODO				0x09
#define UBX_NAV_RESETODO		0x10
#define UBX_NAV_VELECEF			0x11
#define UBX_NAV_VELNED			0x12
#define UBX_NAV_HPPOSECEF		0x13
#define UBX_NAV_HPPOSLLH		0x14
#define UBX_NAV_TIMEGPS			0x20
#define UBX_NAV_TIMEUTC			0x21
#define UBX_NAV_CLOCK			0x22
#define UBX_NAV_TIMEGLO			0x23
#define UBX_NAV_TIMEBDS			0x24
#define UBX_NAV_TIMEGAL			0x25
#define UBX_NAV_TIMELS			0x26
#define UBX_NAV_SVINFO			0x30
#define UBX_NAV_DGPS			0x31
#define UBX_NAV_SBAS			0x32
#define UBX_NAV_ORB				0x34
#define UBX_NAV_SAT				0x35
//#define UBX_NAV_				0x36
#define UBX_NAV_GEOFENCE		0x39
#define UBX_NAV_SVIN			0x3B
#define UBX_NAV_RELPOSNED		0x3C
#define UBX_NAV_EXFSTATUS		0x40
#define UBX_NAV_SLAS			0x42
#define UBX_NAV_SIG				0x43
#define UBX_NAV_AOPSTATUS		0x60
#define UBX_NAV_EOE				0x61

#define UBX_RXM_RAW				0x10
#define UBX_RXM_SFRB			0x11
#define UBX_RXM_SFRBX			0x13
#define UBX_RXM_MEASX			0x14
#define UBX_RXM_RAWX			0x15
#define UBX_RXM_SVSI			0x20
//#define UBX_RXM_				0x23
#define UBX_RXM_ALM				0x30
#define UBX_RXM_EPH				0x31
#define UBX_RXM_RTCM			0x32
#define UBX_RXM_PMREQ			0x41
//#define UBX_RXM_				0x51
//#define UBX_RXM_				0x52
//#define UBX_RXM_				0x57
#define UBX_RXM_RLM				0x59
#define UBX_RXM_IMES			0x61

//#define UBX_UPD_				0x13
#define UBX_UPD_SOS				0x14

//#define UBX_SEC_				0x00
#define UBX_SEC_SIGN			0x01
#define UBX_SEC_UNIQID			0x03


// convert position (lat/lon) between dec and float
#define UBX2FLT(n)				((int32_t)(n)/10000000.0)
#define FLT2UBX(n)				((double)(n)*10000000.0)

#define dec32flt9(n)			((n)/1000000000.0)
#define dec32flt8(n)			((n)/100000000.0)
#define dec32flt7(n)			((n)/10000000.0f)
#define dec32flt6(n)			((n)/1000000.0f)
#define dec32flt5(n)			((n)/100000.0f)
#define dec32flt4(n)			((n)/10000.0f)
#define dec32flt3(n)			((n)/1000.0f)
#define dec32flt2(n)			((n)/100.0f)

#define dec32dbl7(n)			((n)/10000000.0)

#define USB_FLAGS_REENUM		(1<<0)		// force re-enumeration
#define USB_FLAGS_POWERMODE		(1<<1)		// 1=self-powered, 0=bus-powered
#define USB_FLAGS_AUTOCONN		(1<<2)		// Auto re-connect/enumerate
#define USB_FLAGS_HIDSTARTSTOP	(1<<15)		// Allow HID to start/stop GPS


typedef struct {
	uint16_t vendorID;				// VID
	uint16_t productID;				// PID
	uint8_t reserved1[2];
	uint8_t reserved2[2];
	uint16_t powerConsumption;		// ma
	uint16_t flags;					// USB_FLAGS_
	uint8_t vendorString[32];
	uint8_t productString[32];
	uint8_t serialNumber[32];
}__attribute__((packed))cfg_usb_t;




#define ITFM_CONFIG_BBTHRESHOLD		(0x0000000F)
#define ITFM_CONFIG_CWTHRESHOLD		(0x000001F0)
#define ITFM_CONFIG_ALGORITHMBITS	(0x7FFFFE00)	// Reserved algorithm settings. As per PDF, set to 0x16B156 for 'correct settings'
#define ITFM_CONFIG_ENABLE			(0x80000000)

#define ITFM_CONFIG2_GENERALBITS	(0x0FFF)		// General settings. Should be set to 0x031E (as per PDF) in hex for correct setting
#define ITFM_CONFIG2_ANTSETTING		(0x3000)		// Antenna setting. 0=unknown, 1=passive, 2=active
#define ITFM_CONFIG2_ENABLE2		(0x4000)		// Set to 1 to scan auxiliary bands (u-blox 8 / u-blox M8 only, otherwise ignored)

typedef struct {
	uint32_t config;			// ITFM_CONFIG_. Interference config word.
	uint32_t config2;			// ITFM_CONFIG2_. Settings for jamming/interference monitor
}__attribute__((packed))cfg_itfm_t;



#define RST_BBRMASK_HOTSTART		(0x0000)		// Battery Backed Ram sections to clear
#define RST_BBRMASK_WARMSTART		(0x0001)		// Only Ephemeris is cleared
#define RST_BBRMASK_COLDSTART		(0xFFFF)		// All sections cleared

#define RST_BBRMASK_EPH				(1<<0)			// User-defined BBR sections to clear.
#define RST_BBRMASK_ALM				(1<<1)
#define RST_BBRMASK_HEALTH			(1<<2)
#define RST_BBRMASK_KLOB			(1<<3)
#define RST_BBRMASK_POS				(1<<4)
#define RST_BBRMASK_CLKD			(1<<5)
#define RST_BBRMASK_OSC				(1<<6)
#define RST_BBRMASK_UTC				(1<<7)
#define RST_BBRMASK_RTC				(1<<8)
#define RST_BBRMASK_AOP				(1<<15)

#define RST_RESETMODE_HWBEFORE		(0x00)			// Hardware reset (Watchdog) immediately
#define RST_RESETMODE_SOFTWARE		(0x01)			// Controlled Software reset
#define RST_RESETMODE_SWGNSSONLY	(0x02)			// Controlled Software reset (GNSS only)
#define RST_RESETMODE_HWAFTER		(0x04)			// Hardware reset (Watchdog) after shutdown
#define RST_RESETMODE_CGNSSSTOP		(0x08)			// Controlled GNSS stop
#define RST_RESETMODE_CGNSSSTART	(0x09)			// Controlled GNSS start

typedef struct {
	uint16_t navBbrMask;		// RST_BBRMASK_
	uint8_t resetMode;			// RST_RESETMODE_
	uint8_t reserved1;
}__attribute__((packed))cfg_rst_t;

typedef struct {
	uint8_t highNavRate;
	uint8_t reserved[3];
}__attribute__((packed))cfg_hnr_t;

typedef struct {				// aop = Autonomous Orbit Parameters
	uint8_t gnssId;				// GNSS identifier
	uint8_t svId;				// Satellite identifier
	uint8_t reserved1[2];
	
	uint8_t data[64];			// assistance data
}__attribute__((packed))aid_aop_t;


typedef struct {				// eph = Ephemeris
	uint32_t svid;
	uint32_t how;				// Hand-Over Word of first Subframe. 0 = that no Ephemeris Data is following.
	
	uint32_t sf1d[8];			// Subframe 1 Words 3..10 (SF1D0..SF1D7)
	uint32_t sf2d[8];			// Subframe 2 Words 3..10 (SF2D0..SF2D7)
	uint32_t sf3d[8];			// Subframe 3 Words 3..10 (SF3D0..SF3D7)
}__attribute__((packed))aid_eph_t;

typedef struct {				// alm = Almanac
	uint32_t svid;
	uint32_t week;				// GPS (week number) issue date of almanac
	
	uint32_t dwrd[8];			// Almanac Words
}__attribute__((packed))aid_alm_t;



#define NAV5_MASK_DYN				(1<<0)		// Apply dynamic model settings
#define NAV5_MASK_MINEL				(1<<1)		// Apply minimum elevation settings
#define NAV5_MASK_POSFIXMODE		(1<<2)		// Apply fix mode settings
#define NAV5_MASK_DRLIM				(1<<3)		// reserved
#define NAV5_MASK_POSMASK			(1<<4)		// Apply position mask settings
#define NAV5_MASK_TIMEMASK			(1<<5)		// Apply time mask settings
#define NAV5_MASK_STATICHOLDMASK	(1<<6)		// Apply static hold settings
#define NAV5_MASK_DGPSMASK			(1<<7)		// Apply DGPS settings.
#define NAV5_MASK_CNOTHRESHOLD		(1<<8)		// Apply CNO threshold settings (cnoThresh, cnoThreshNumSVs).
#define NAV5_MASK_UTC				(1<<10)		// Apply UTC settings.

#define NAV5_DYNMODEL_PORTABLE		0
#define NAV5_DYNMODEL_STATIONARY	2
#define NAV5_DYNMODEL_PEDESTRIAN	3
#define NAV5_DYNMODEL_AUTOMOTIVE	4
#define NAV5_DYNMODEL_SEA			5
#define NAV5_DYNMODEL_AIRBORNE1G	6			// airborne with <1g acceleration
#define NAV5_DYNMODEL_AIRBORNE2G	7			// airborne with <2g acceleration
#define NAV5_DYNMODEL_AIRBORNE4G	8			// airborne with <4g acceleration
#define NAV5_DYNMODEL_WRIST			9			// wrist worn watch, auto corrects for arm movement. v18.0+
#define NAV5_DYNMODEL_MBIKE			10			// supported in protocol versions 19.2+
#define NAV5_DYNMODEL_LAWNMOWER		11			// robotic mower. supported in protocol versions 19.2+
#define NAV5_DYNMODEL_KICKSCOOTER	12			// electric. supported in protocol versions 19.2+

#define NAV5_FIXMODE_2D				1			// 2D only
#define NAV5_FIXMODE_3D				2			// 3D only
#define NAV5_FIXMODE_AUTO			3			// auto 2d/3d

#define NAV5_UTCSTD_AUTO			0			// Automatic selection based upon GNSS configuration
#define NAV5_UTCSTD_GPS				3			// USNO
#define NAV5_UTCSTD_GALILEO			5			// TBD
#define NAV5_UTCSTD_GLONASS			6			// SU
#define NAV5_UTCSTD_BEIDOU			7			// NTSC
#define NAV5_UTCSTD_NAVIC			8			// NPLI - NavIc (India)

typedef struct {
	uint16_t mask;								// NAV5_MASK_
	uint8_t dynModel;							// NAV5_DYNMODEL_
	uint8_t fixMode;							// NAV5_FIXMODE_
	int32_t fixedAlt;
	uint32_t fixedAltVar;
	int8_t minElv;
	uint8_t drLimit;
	uint16_t pDop;
	uint16_t tDop;
	uint16_t pAcc;
	uint16_t tAcc;
	uint8_t staticHoldThresh;
	uint8_t dynssTimeout;
	uint8_t cnoThreshNumSVs;
	uint8_t cnoThresh;
	uint16_t pAccADR;							// proto 19, for proto 18:uint8_t reserved1[2];
	uint16_t staticHoldMaxDist;
	uint8_t utcStandard;						// NAV5_UTCSTD_
	uint8_t reserved5[5];
}__attribute__((packed))cfg_nav5_t;


#define NAVX5_VERSION_PROTO1517		1				//	Protocol versions: 15, 15.01, 16 and 17
#define NAVX5_VERSION_PROTO18		2
#define NAVX5_VERSION_PROTO19		3

#define NAVX5_MASK1_MINMAX			(1<<2)
#define NAVX5_MASK1_MINCNO			(1<<3)
#define NAVX5_MASK1_INITIAL3DFIX	(1<<6)
#define NAVX5_MASK1_WKNROLL			(1<<9)
#define NAVX5_MASK1_ACKAID			(1<<10)
#define NAVX5_MASK1_PPP				(1<<13)
#define NAVX5_MASK1_AOP				(1<<14)

#define NAVX5_MASK2_ADR				(1<<6)
#define NAVX5_MASK2_SIGATTENCOMP	(1<<7)

#define NAVX5_SACM_DISABLED			0
#define NAVX5_SACM_MASK				0x3F			// only C/N0 values 1 to 63 are valid
#define NAVX5_SACM_AUTO				255

#define NAVX5_AOPCFG_DISABLED		0				// AssistNow Autonomous configuration
#define NAVX5_AOPCFG_USEAOP			(1<<0)


// protocol version 18
typedef struct {
	uint16_t version;				// NAVX5_VERSION_
	uint16_t mask1;					// NAVX5_MASK1_
	uint32_t mask2;					// NAVX5_MASK2_
	uint8_t reserved1[2];
	uint8_t minSVs;
	uint8_t maxSVs;
	uint8_t minCNO;
	uint8_t reserved2;
	uint8_t iniFix3D;				// 1 = initial fix must be 3D
	uint8_t reserved3[2];
	uint8_t ackAiding;				// 1 = issue acknowledgements for assistance message input
	uint16_t wknRollover;			// 0 = revert to firmware default
	uint8_t sigAttenCompMode;		// NAVX5_SACM_, 1..63 = maximum expected C/N0 value
	uint8_t reserved4;
	uint8_t reserved5[2];
	uint8_t reserved6[2];
	uint8_t usePPP;
	uint8_t aopCfg;					// NAVX5_AOPCFG_
	uint8_t reserved7[2];
	uint16_t aopOrbMaxErr;
	uint8_t reserved8[4];
	uint8_t reserved9[3];
	uint8_t useAdr;					// Enable/disable ADR/UDR sensor fusion.
}__attribute__((packed))cfg_navx5_t;


#define CFG_PORTID_I2C			0
#define CFG_PORTID_UART1		1
#define CFG_PORTID_UART2		2
#define CFG_PORTID_USB			3
#define CFG_PORTID_SPI			4
//#define CFG_PORTID_USER0
//#define CFG_PORTID_USER1

#define INF_PROTO_UBX			0
#define INF_PROTO_NEMA			1
#define INF_PROTO_RAW			3
#define INF_PROTO_RTCM3			5
#define INF_PROTO_USER0			12
#define INF_PROTO_USER1			13
#define INF_PROTO_USER2			14
#define INF_PROTO_USER3			15

#define INF_MSG_ERROR			(1<<0)
#define INF_MSG_WARNING			(1<<1)
#define INF_MSG_NOTICE			(1<<2)
#define INF_MSG_TEST			(1<<3)
#define INF_MSG_DEBUG			(1<<4)
#define INF_MSG_RES5			(1<<5)
#define INF_MSG_RES6			(1<<6)
#define INF_MSG_ALL				0xFF


typedef struct {
	uint8_t protocolID;						// INF_PROTO_
}__attribute__((packed))cfg_inf_poll_t;

typedef struct {
	uint8_t protocolID;						// INF_PROTO_
	uint8_t reserved1[3];
	uint8_t infMsgMask[6];					// masks of INF_MSG_, indexed by CFG_PORTID_
}__attribute__((packed))cfg_inf_t;


#define CFG_CHARLEN_5BIT		0b00
#define CFG_CHARLEN_6BIT		0b01
#define CFG_CHARLEN_7BIT		0b10
#define CFG_CHARLEN_8BIT		0b11

#define CFG_PARTITY_EVEN		0b000
#define CFG_PARTITY_ODD			0b001
#define CFG_PARTITY_NONE		0b100
#define CFG_PARTITY_RESERVED	0b010

#define CFG_STOPBITS_1			0b00
#define CFG_STOPBITS_15			0b01		// 1.5
#define CFG_STOPBITS_2			0b10
#define CFG_STOPBITS_05			0b11		// 0.5

#define CFG_BITORDER_LSB		0b0
#define CFG_BITORDER_MSB		0b1

#define CFG_PROTO_UBX			(1 << 0)
#define CFG_PROTO_NMEA			(1 << 1)
#define CFG_PROTO_RTCM2			(1 << 2)
#define CFG_PROTO_RTCM3			(1 << 5)
#define CFG_PROTO_USER0			(1 << 12)
#define CFG_PROTO_USER1			(1 << 13)
#define CFG_PROTO_USER2			(1 << 14)
#define CFG_PROTO_USER3			(1 << 15)

#define CFG_PRTFLAGS_EXTTXTO	0x02			// Extended TX timeout


typedef struct {
	uint8_t portId;					// CFG_PORTID_
	uint8_t reserved1;

	union{
		uint16_t flags;
		struct {
			uint16_t en:1;
			uint16_t pol:1;
			uint16_t pin:5;			// (PIO to use)
			uint16_t thres:9;
		}bits;
	}txReady;
	
	union{
		uint32_t flags;
		struct {
			uint32_t pad1:6;
			uint32_t charLen:2;		// CFG_CHARLEN_
			uint32_t pad2:1;
			uint32_t partity:3;		// CFG_PARTITY_
			uint32_t nStopBits:2;	// CFG_STOPBITS_
			uint32_t pad3:2;
			uint32_t bitOrder:1;	// CFG_BITORDER_
			uint32_t pad4:15;
		}bits;
	}mode;
	
	uint32_t baudRate;				// bits per second
	uint16_t inProtoMask;			// CFG_PROTO_ | CFG_PROTO_, etc..
	uint16_t outProtoMask;			// ^^
	uint16_t flags;					// CFG_PRTFLAGS_
	uint8_t reserved2[2];
}__attribute__((packed))cfg_prt_uart_t;

typedef struct {
	uint8_t portId;					// CFG_PORTID_
	uint8_t reserved1;
	
	union{
		uint16_t flags;
		struct {
			uint16_t en:1;
			uint16_t pol:1;
			uint16_t pin:5;			// (PIO to use)
			uint16_t thres:9;
		}bits;
	}txReady;

	uint32_t mode;					// SPI flags
	uint8_t reserved2[4];
	uint16_t inProtoMask;			// CFG_PROTO_ | CFG_PROTO_, etc..
	uint16_t outProtoMask;			// ^^	
	uint16_t flags;					// CFG_PRTFLAGS_
	uint8_t reserved3[2];	
}__attribute__((packed))cfg_prt_spi_t;

typedef struct {
	uint8_t portId;					// CFG_PORTID_
	uint8_t reserved1;

	union{
		uint16_t flags;
		struct {
			uint16_t en:1;
			uint16_t pol:1;
			uint16_t pin:5;			// (PIO to use)
			uint16_t thres:9;
		}bits;
	}txReady;
	
	union{
		uint32_t flags;
		struct {
			uint32_t pad1:1;
			uint32_t slaveAddr:7;
			uint32_t pad2:24;
		}bits;
	}mode;
	
	uint8_t reserved2[4];
	uint16_t inProtoMask;			// CFG_PROTO_ | CFG_PROTO_, etc..
	uint16_t outProtoMask;			// ^^
	uint16_t flags;					// CFG_PRTFLAGS_
	uint8_t reserved3[2];
}__attribute__((packed))cfg_prt_i2c_t;

typedef struct {
	uint8_t portId;					// CFG_PORTID_
	uint8_t reserved1;

	union{
		uint16_t flags;
		struct {
			uint16_t en:1;
			uint16_t pol:1;
			uint16_t pin:5;			// (PIO to use)
			uint16_t thres:9;
		}bits;
	}txReady;

	uint8_t reserved2[8];
	uint16_t inProtoMask;			// CFG_PROTO_ | CFG_PROTO_, etc..
	uint16_t outProtoMask;			// ^^
	uint8_t reserved3[2];
	uint8_t reserved4[2];
}__attribute__((packed))cfg_prt_usb_t;

typedef struct {
	union{
		uint8_t id;					// CFG_PORTID_
		
		cfg_prt_i2c_t  i2c;
		cfg_prt_uart_t uart;
		cfg_prt_usb_t  usb;
		cfg_prt_spi_t  spi;
	}port;
}__attribute__((packed))cfg_prt_t;


#define CFG_TIMEREF_UTC			0
#define CFG_TIMEREF_GPS			1
#define CFG_TIMEREF_GLONASS		2
#define CFG_TIMEREF_BEIDOU		3		// v18+
#define CFG_TIMEREF_GALIEO		4		// v18+
#define CFG_TIMEREF_NAVIC		5		// v29+

typedef struct {
	uint16_t measRate;
	uint16_t navRate;
	uint16_t timeRef;				// CFG_TIMEREF_
}__attribute__((packed))cfg_rate_t;


#define GNSSID_GPS				0
#define GNSSID_SBAS				1
#define GNSSID_GALILEO			2
#define GNSSID_BEIDOU			3
#define GNSSID_IMES				4
#define GNSSID_QZSS				5
#define GNSSID_GLONASS			6

#define GNSS_CFGBLK_DISABLED	(0x00)
#define GNSS_CFGBLK_ENABLED		(0x01)
#define GNSS_CFGBLK_SIGENABLED	((0x01<<16)|(0x01 << 24))

#define CFG_GNSS_SIZE(nBlks)	(sizeof(cfg_gnss_t)+((nBlks)*sizeof(cfg_cfgblk_t)))

typedef struct {
	uint8_t gnssId;					// GNSSID_
	uint8_t resTrkCh;
	uint8_t maxTrkCh;
	uint8_t reserved1;
	uint32_t flags;					// GNSS_CFGBLK_
}__attribute__((packed))cfg_cfgblk_t;

typedef struct {
	uint8_t msgVer;
	uint8_t numTrkChHw;
	uint8_t numTrkChUse;
	uint8_t numConfigBlocks;
	
	cfg_cfgblk_t cfgblk[];
}__attribute__((packed))cfg_gnss_t;


#define GEOFENCE_CONFIDIENCE_NONE		0		// no confidence required
#define GEOFENCE_CONFIDIENCE_68			1		// 68%
#define GEOFENCE_CONFIDIENCE_95			2		// 95%
#define GEOFENCE_CONFIDIENCE_997		3		// 99.7%
#define GEOFENCE_CONFIDIENCE_9999		4		// 99.9999%
#define GEOFENCE_CONFIDIENCE_99999		5		// 99.999999%

#define CFG_GEOFENCE_SIZE(nBlks)		(sizeof(cfg_geofence_t)+((nBlks)*sizeof(cfg_geofence_fence_t)))

typedef struct {
	int32_t lat;
	int32_t lon;
	uint32_t radius;
}__attribute__((packed))cfg_geofence_fence_t;

typedef struct {
	uint8_t version;
	uint8_t numFences;
	uint8_t confLvl;				// GEOFENCE_CONFIDIENCE_
	uint8_t reserved1[1];
	
	uint8_t pioEnabled;				// 1 = enable PIO combined fence state output, 0 = disable
	uint8_t pinPolarity;			// 0 = low means inside, 1 = low means outside
	uint8_t pin;
	uint8_t reserved2[1];
	
	cfg_geofence_fence_t fence[];
}__attribute__((packed))cfg_geofence_t;

typedef struct {
	uint8_t clsId;
	uint8_t msgId;
}__attribute__((packed))ack_ack_t;

typedef struct {
	uint8_t clsId;
	uint8_t msgId;
}__attribute__((packed))ack_nak_t;


#define SVINFO_GFLAGS_CHIPGEN			(0b111 << 0)

#define SVINFO_FLAGS_SVUSED				(1 << 0)
#define SVINFO_FLAGS_DIFFCORR			(1 << 1)
#define SVINFO_FLAGS_ORBITAVAIL			(1 << 2)
#define SVINFO_FLAGS_ORBITEPH			(1 << 3)
#define SVINFO_FLAGS_UNHEALTHY			(1 << 4)
#define SVINFO_FLAGS_ORBITALM			(1 << 5)
#define SVINFO_FLAGS_ORBITAOP			(1 << 6)
#define SVINFO_FLAGS_SMOOTHED			(1 << 7)

#define SVINFO_QUALITY_QUALITYIND		(0b1111 << 7)

typedef struct {
	uint8_t chn;
	uint8_t svid;
	uint8_t flags;				// SVINFO_FLAGS_
	uint8_t quality;			// SVINFO_QUALITY_
	uint8_t cno;
	int8_t elev;
	int16_t azim;
	int32_t prRes;
}__attribute__((packed))nav_svinfo_chn_t;


typedef struct {
	uint32_t iTow;
	uint8_t numCh;
	uint8_t globalFlags;		// SVINFO_GFLAGS_ (0: Antaris, Antaris4, 1: u-blox5, 2: u-blox6, 3: u-blox7, 4: u-blox8/u-blox M8)
	uint8_t reserverd1[2];
	
	nav_svinfo_chn_t sats[];
}__attribute__((packed))nav_svinfo_t;

typedef struct {
	uint32_t iTow;				// Time of Week
	uint16_t gDOP;
	uint16_t pDOP;
	uint16_t tDOP;
	uint16_t vDOP;
	uint16_t hDOP;
	uint16_t nDOP;
	uint16_t eDOP;
}__attribute__((packed))nav_dop_t;



#define PVT_VALID_VALIDDATE				(0b001 << 0)
#define PVT_VALID_VALIDTIME				(0b001 << 1)
#define PVT_VALID_FULLYRESOLVED			(0b001 << 2)
#define PVT_VALID_VALIDMAG				(0b001 << 3)

#define PVT_FIXTYPE_NOFIX				0				// no fix                
#define PVT_FIXTYPE_DEADRECK			1				// dead reckoning only   
#define PVT_FIXTYPE_2D					2				// 2D-fix                
#define PVT_FIXTYPE_3D					3				// 3D-fix                
#define PVT_FIXTYPE_GNSSDR				4				// GNSS + dead reckoning 
#define PVT_FIXTYPE_TIME				5				// time only fix         

#define PVT_FLAGS_GNSSFIXOK				(0b001 << 0)	// 1 = valid fix (i.e within DOP & accuracy masks)
#define PVT_FLAGS_DIFFSONL				(0b001 << 1)	// 1 = differential corrections were applied
#define PVT_FLAGS_PSMSTATE				(0b111 << 2)	// Power Save Mode. 0: PSM notactive, 1: Enabled, 2: Acquisition, 3: Tracking, 4: Power Optimized Tracking, 5: Inactive
#define PVT_FLAGS_HEADVEHVALID			(0b001 << 5)	// 1 = heading of vehicle is valid
#define PVT_FLAGS_CARRSONL				(0b011 << 6)	// Carrier phase range solution status: 0: no carrier phase range solution, 1: float solution, 2: fixed solution

#define PVT_FLAGS2_CONFIRMEDAVAI		(0b001 << 5)	// 1 = information about UTC Date and Time of Day validity confirmation is available
#define PVT_FLAGS2_CONFIRMEDDATE		(0b001 << 6)	// 1 = UTC Date validity could be confirmed
#define PVT_FLAGS2_CONFIRMEDTIME		(0b001 << 7)	// 1 = UTC Time of Day could be confirmed


typedef struct {
	uint32_t iTow;			// time of week
	uint16_t year;              
	uint8_t month;              
	uint8_t day;
	uint8_t hour;               
	uint8_t min;                
	uint8_t sec;                
	uint8_t valid;  		// PVT_VALID_
	
	uint32_t tAcc;
	int32_t nano;
	uint8_t fixType;		// PVT_FIXTYPE_
	uint8_t flags;			// PVT_FLAGS_
	uint8_t flags2;			// PVT_FLAGS2_
	uint8_t numSv;
	int32_t lon;
	int32_t lat;
	int32_t height;
	int32_t hMSL;
	uint32_t hAcc;
	
	uint32_t vAcc;
	int32_t velN;
	int32_t velE;
	int32_t velD;
	int32_t gSpeed;
	int32_t headMot;
	uint32_t sAcc;
	uint32_t headAcc;
	uint16_t pDop;
	uint8_t  reserved1[6];
	int32_t headVeh;
	int16_t magDec;
	uint16_t magAcc;
}__attribute__((packed))nav_pvt_t;


#define STATUS_FLAGS_GPSFIXOK			(1 << 0)
#define STATUS_FLAGS_DIFFSOLN			(1 << 1)
#define STATUS_FLAGS_WKNSET				(1 << 2)
#define STATUS_FLAGS_TOWSET				(1 << 3)

#define STATUS_FIXSTAT_DIFFCORR			(0b01 << 0)
#define STATUS_FIXSTAT_MAPMATCHING		(0b11 << 6)

#define STATUS_FLAGS2_PSMSTATE			(0b11 << 0)
#define STATUS_FLAGS2_SPOOFDETSTATE		(0b11 << 3)

typedef struct {
	uint32_t iTow;
	uint8_t gpsFix;						// fix type. this value alone does not qualify a fix as valid
	uint8_t flags;						// STATUS_FLAGS_
	uint8_t fixStat;					// STATUS_FIXSTAT_
	uint8_t flags2;						// STATUS_FLAGS2_
	uint32_t ttff;						// time to first fix
	uint32_t msss;						// Milliseconds since Startup / Reset
}__attribute__((packed))nav_status_t;

typedef struct {
	uint32_t iTow;
	int32_t ecefX;
	int32_t ecefY;
	int32_t ecefZ;
	uint32_t pAcc;
}__attribute__((packed))nav_posecef_t;

typedef struct {
	uint32_t iTow;
	int32_t lon;
	int32_t lat;
	int32_t height;
	int32_t hMSL;
	uint32_t hAcc;
	uint32_t vAcc;
}__attribute__((packed))nav_posllh_t;

typedef struct {
	uint32_t iTow;
	uint8_t version;
	uint8_t reserved1[3];
	int32_t roll;
	int32_t pitch;
	int32_t heading;
	uint32_t accRoll;
	uint32_t accPitch;
	uint32_t accHeading;
}__attribute__((packed))nav_att_t;

typedef struct {
	uint32_t iTow;
	int32_t fTow;
	int16_t week;
	int8_t gpsFix;
	int8_t flags;
	int32_t ecefX;
	int32_t ecefY;
	int32_t ecefZ;
	uint32_t pAcc;
	int32_t ecefVX;
	int32_t ecefVY;
	int32_t ecefVZ;
	uint32_t sAcc;
	uint16_t pDOP;
	uint8_t reserved1;
	uint8_t numSV;
	uint8_t reserved2[4];
}__attribute__((packed))nav_sol_t;

typedef struct {
	uint8_t version;
	uint8_t reserved1[3];
	uint32_t iTow;
	uint32_t distance;
	uint32_t totalDistance;
	uint32_t distanceStd;
}__attribute__((packed))nav_odo_t;

typedef struct {		// is a command, there's no payload
}__attribute__((packed))nav_resetodo_t;

typedef struct {
	uint8_t version;
	uint8_t reserved1[3];
	uint32_t iTow;
	int32_t ecefX;
	int32_t ecefY;
	int32_t ecefZ;
	int8_t ecefXHp;
	int8_t ecefYHp;
	int8_t ecefZHp;
	uint8_t reserved2;
	uint32_t pAcc;
}__attribute__((packed))nav_hpposecef_t;

typedef struct {
	uint8_t version;
	uint8_t reserved1[3];
	uint32_t iTow;
	
	int32_t lon;
	int32_t lat;
	int32_t height;
	int32_t hMSL;
	int8_t lonHp;
	int8_t latHp;
	int8_t heightHp;
	int8_t hMSLHp;
	uint32_t hAcc;
	uint32_t vAcc;
}__attribute__((packed))nav_hpposllh_t;

typedef struct {
	uint32_t iTow;
	int32_t clkB;
	int32_t clkD;
	uint32_t tAcc;
	uint32_t fAcc;
}__attribute__((packed))nav_clock_t;


#define TIMEBDS_VALID_SOW		(0b001 << 0)
#define TIMEBDS_VALID_WEEK		(0b001 << 0)
#define TIMEBDS_VALID_LEAPS		(0b001 << 0)

typedef struct {
	uint32_t iTow;
	uint32_t SOW;
	int32_t fSOW;
	int16_t week;
	int8_t leapS;
	int8_t valid;			// TIMEBDS_VALID_
	uint32_t tAcc;
}__attribute__((packed))nav_timebds_t;

typedef struct {
	uint32_t iTow;
	uint32_t galTow;
	int32_t fGalTow;
	int16_t galWno;
	int8_t leapS;
	int8_t valid;
	uint32_t tAcc;
}__attribute__((packed))nav_timegal_t;

typedef struct {
	uint32_t iTow;
	uint32_t TOD;
	int32_t fTOD;
	int16_t Nt;
	int8_t N4;
	int8_t valid;
	uint32_t tAcc;
}__attribute__((packed))nav_timeglo_t;

typedef struct {
	uint32_t iTow;
	uint32_t fTOW;
	int16_t week;
	int8_t leapS;
	int8_t valid;
	uint32_t tAcc;
}__attribute__((packed))nav_timegps_t;

typedef struct {
	uint32_t iTow;
	uint8_t version;
	uint8_t reserved1[3];
	int8_t srcOfCurrLs;
	uint8_t currLs;
	uint8_t srcOfLsChange;
	int8_t lsChange;
	int32_t timeToLsEvent;
	uint16_t dateOfLsGpsWn;
	uint16_t dateOfLsGpsDn;
	uint8_t reserved2[3];
	uint8_t valid;
}__attribute__((packed))nav_timels_t;

typedef struct {
	uint32_t iTOW;
	uint32_t tAcc;
	int32_t nano;
	uint16_t year;
	int8_t month;
	int8_t day;
	int8_t hour;
	int8_t min;
	int8_t sec;
	uint8_t valid;
}__attribute__((packed))nav_timeutc_t;

typedef struct {
	uint32_t iTow;
	int32_t ecefVX;
	int32_t ecefVY;
	int32_t ecefVZ;
	uint32_t sAcc;
}__attribute__((packed))nav_velecef_t;

typedef struct {
	uint32_t iTow;
	int32_t velN;
	int32_t velE;
	int32_t velD;
	uint32_t speed;
	uint32_t gSpeed;
	int32_t heading;
	uint32_t sAcc;
	uint32_t cAcc;
}__attribute__((packed))nav_velned_t;

typedef struct {
	uint8_t svid;
	uint8_t flags;
	uint16_t ageC;
	uint32_t prc;
	uint32_t prrc;
}__attribute__((packed))nav_dgps_chn_t;

typedef struct {
	uint32_t iTow;
	int32_t age;
	int16_t baseId;
	int16_t baseHealth;
	uint8_t numCh;
	uint8_t status;
	uint8_t reserved1[2];
	
	nav_dgps_chn_t chn[];
}__attribute__((packed))nav_dgps_t;


#define SBAS_SERVICE_RANGING		(0b001 << 0)
#define SBAS_SERVICE_CORRECTIONS	(0b001 << 1)
#define SBAS_SERVICE_INTEGRITY		(0b001 << 2)
#define SBAS_SERVICE_TESTMODE		(0b001 << 3)
#define SBAS_SERVICE_BAD			(0b001 << 4)

typedef struct {
	uint8_t svid;
	uint8_t flags;
	uint8_t udre;
	uint8_t svSys;
	
	uint8_t svService;
	uint8_t reserved2;
	int16_t prc;
	
	uint8_t reserved3[2];
	int16_t ic;
}__attribute__((packed))nav_sbas_sv_t;

typedef struct {
	uint32_t iTow;
	
	uint8_t geo;
	uint8_t mode;
	int8_t sys;
	uint8_t service;		// SBAS_SERVICE_

	uint8_t cnt;
	uint8_t statusFlags;
	uint8_t reserved1[2];
	
	nav_sbas_sv_t sv[];
}__attribute__((packed))nav_sbas_t;

typedef struct {
	uint8_t gnssId;
	uint8_t svId;
	uint8_t svFlag;
	uint8_t eph;
	
	uint8_t alm;
	uint8_t otherOrb;
}__attribute__((packed))nav_orb_sv_t;

typedef struct {
	uint32_t iTow;
	
	uint8_t version;
	uint8_t numSv;
	uint8_t reserved1[2];
	
	nav_orb_sv_t sv[];
}__attribute__((packed))nav_orb_t;


#define SAT_FLAGS_QUALITYIND		(0b111 << 0)		// 3bits
#define SAT_FLAGS_SVUSED			(0b001 << 3)
#define SAT_FLAGS_HEALTH			(0b011 << 4)		// 2 bits
#define SAT_FLAGS_DIFFCORR			(0b001 << 6)
#define SAT_FLAGS_SMOOTHED			(0b001 << 7)
#define SAT_FLAGS_ORBITSOURCE		(0b111 << 8)		// 3bits
#define SAT_FLAGS_EPHAVAIL			(0b001 << 11)
#define SAT_FLAGS_ALMAVAIL			(0b001 << 12)
#define SAT_FLAGS_ANOAVAIL			(0b001 << 13)
#define SAT_FLAGS_AOPAVAIL			(0b001 << 14)
//#define SAT_FLAGS_				(0b001 << 15)
#define SAT_FLAGS_SBASCORRUSED		(0b001 << 16)
#define SAT_FLAGS_RTCMCORRUSED		(0b001 << 17)
#define SAT_FLAGS_SLASCORRUSED		(0b001 << 18)
#define SAT_FLAGS_SPARTNCORRUSED	(0b001 << 19)
#define SAT_FLAGS_PRCORRUSED		(0b001 << 20)
#define SAT_FLAGS_CRCORRUSED		(0b001 << 21)
#define SAT_FLAGS_DOCORRUSED		(0b001 << 22)
#define SAT_FLAGS_CLASCORRUSED		(0b001 << 23)

typedef struct {
	uint8_t gnssId;						// GNSSID_
	uint8_t svId;
	uint8_t cno;
	int8_t elev;
	int16_t azim;
	int16_t prRes;
	uint32_t flags;						// SAT_FLAGS_
}__attribute__((packed))nav_sat_sv_t;

typedef struct {
	uint32_t iTow;
	uint8_t version;
	uint8_t numSvs;
	uint8_t reserved1[2];
	
	nav_sat_sv_t sv[];
}__attribute__((packed))nav_sat_t;


#define GEOFENCE_STATE_UNKNOWN			0
#define GEOFENCE_STATE_INSIDE			1
#define GEOFENCE_STATE_OUTSIDE			2

#define GEOFENCE_STATUS_UNAVAILABLE		0			// geofencing not available or not reliable
#define GEOFENCE_STATUS_ACTIVE			1

typedef struct {
	uint8_t state;					// GEOFENCE_STATE_
	uint8_t reserved[1];
}__attribute__((packed))nav_geofence_fence_t;

typedef struct {
	uint32_t iTow;
	uint8_t version;
	uint8_t status;					// GEOFENCE_STATUS_
	uint8_t numFences;
	uint8_t combState;
	
	nav_geofence_fence_t fence[];
}__attribute__((packed))nav_geofence_t;

typedef struct {
	uint8_t version;
	uint8_t reserved1[3];
	uint32_t iTow;
	uint32_t dur;
	int32_t meanX;
	int32_t meanY;
	int32_t meanZ;
	int8_t meanXHP;
	int8_t meanYHP;
	int8_t meanZHP;
	uint8_t reserved2;
	uint32_t meanAcc;
	uint32_t obs;
	uint8_t valid;
	uint8_t active;
	uint8_t reserved3[2];
}__attribute__((packed))nav_svin_t;

typedef struct {
	uint8_t version;
	uint8_t reserved1;
	uint16_t refStationId;
	uint32_t iTow;
	
	int32_t relPosN;
	int32_t relPosE;
	int32_t relPosD;
	int8_t relPosHPN;
	int8_t relPosHPE;
	int8_t relPosHPD;
	uint8_t reserved2;
	uint32_t accN;
	uint32_t accE;
	uint32_t accD;
	uint32_t flags;
}__attribute__((packed))nav_relposned_t;

typedef struct {
	uint32_t iTow;
	uint8_t aopCfg;
	uint8_t status;
	uint8_t reserved1[10];
}__attribute__((packed))nav_aopstatus_t;

typedef struct {
	uint32_t iTow;
}__attribute__((packed))nav_eoe_t;

typedef struct {
	uint8_t swVersion[30];
	uint8_t hwVersion[10];
	uint8_t extension[];
}__attribute__((packed))mon_ver_t;

typedef struct {
	uint32_t rxBytes;
	uint32_t txBytes;
	uint16_t parityErrs;
	uint16_t framingErrs;
	uint16_t overrunErrs;
	uint16_t breakCond;
	uint8_t rxBusy;
	uint8_t txBusy;
	uint8_t reserved1[2];
}__attribute__((packed))mon_io_port_t;

typedef struct {
	mon_io_port_t port1;
	mon_io_port_t port2[];
}__attribute__((packed))mon_io_t;

typedef struct {
	uint8_t gnssId;
	uint8_t svId;
	uint8_t reserved1;
	uint8_t freqId;
	uint8_t numWords;
	uint8_t chn;
	uint8_t version;
	uint8_t reserved2;

	uint32_t dwrd[];
}__attribute__((packed))rxm_sfrbx_t;



#define MAX_REGMSG				64

#define MSG_STATUS_DISABLED		0x00
#define MSG_STATUS_ENABLED		0x01


typedef struct {
	void (*func)(void *opaque, const intptr_t unused);
}msgRetCb_t;

typedef struct {
	int (*func)(const uint8_t *payload, uint16_t msg_len, void *opaque);
	uint8_t uClass;
	uint8_t uId;
	
	uint8_t enabled:1;		// ready to dispatch. :0 == payload dispatching/handling disabled
	uint8_t alert:4;
	uint8_t padbits:3;
	
	uint8_t ct;
}ubx_func_t;


#define CBFREQ_INVALID			-1
#define CBFREQ_NONE				0
#define CBFREQ_LOW				1
#define CBFREQ_MEDIUM			2
#define CBFREQ_HIGH				3
#define CBFREQ_TOTAL			CBFREQ_HIGH

typedef struct {
	ubx_func_t msg[MAX_REGMSG];
	void *userDataPtr;
	
	msgRetCb_t postCb[CBFREQ_TOTAL];		// post processing cb
}ubx_msg_t;




typedef struct{
	union{
		int stub;
	}u;
}ubx_device_t;



#endif

