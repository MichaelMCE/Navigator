
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


#ifndef _UBXCB_H_
#define _UBXCB_H_





int ack_nak (const uint8_t *payload, uint16_t msg_len, void *opaque);
int ack_ack (const uint8_t *payload, uint16_t msg_len, void *opaque);

int aid_aop (const uint8_t *payload, uint16_t msg_len, void *opaque);
int aid_alm (const uint8_t *payload, uint16_t msg_len, void *opaque);
int aid_eph (const uint8_t *payload, uint16_t msg_len, void *opaque);

int mon_ver (const uint8_t *payload, uint16_t msg_len, void *opaque);
int mon_io (const uint8_t *payload, uint16_t msg_len, void *opaque);

int cfg_nav5 (const uint8_t *payload, uint16_t msg_len, void *opaque);
int cfg_navx5 (const uint8_t *payload, uint16_t msg_len, void *opaque);
int cfg_gnss (const uint8_t *payload, uint16_t msg_len, void *opaque);
int cfg_rate (const uint8_t *payload, uint16_t msg_len, void *opaque);
int cfg_inf (const uint8_t *payload, uint16_t msg_len, void *opaque);
int cfg_prt (const uint8_t *payload, uint16_t msg_len, void *opaque);
int cfg_usb (const uint8_t *payload, uint16_t msg_len, void *opaque);
int cfg_geofence (const uint8_t *payload, uint16_t msg_len, void *opaque);

int nav_dop (const uint8_t *payload, uint16_t msg_len, void *opaque);
int nav_eoe (const uint8_t *payload, uint16_t msg_len, void *opaque);
int nav_pvt (const uint8_t *payload, uint16_t msg_len, void *opaque);
int nav_sat (const uint8_t *payload, uint16_t msg_len, void *opaque);
int nav_svinfo (const uint8_t *payload, uint16_t msg_len, void *opaque);
int nav_status (const uint8_t *payload, uint16_t msg_len, void *opaque);
int nav_posllh (const uint8_t *payload, uint16_t msg_len, void *opaque);
int nav_posecef (const uint8_t *payload, uint16_t msg_len, void *opaque);
int nav_geofence (const uint8_t *payload, uint16_t msg_len, void *opaque);
int nav_timebds (const uint8_t *payload, uint16_t msg_len, void *opaque);


int rxm_sfrbx (const uint8_t *payload, uint16_t msg_len, void *opaque);

int inf_debug (const uint8_t *payload, uint16_t msg_len, void *opaque);









#endif

