#pragma once
#ifndef _ENCODERS_H_
#define _ENCODERS_H_

#if ENABLE_ENCODERS




#define ENCODER_TOTAL			3


#define IN_ROTARY_1				1001
#define IN_ROTARY_2				1002
#define IN_ROTARY_3				1003

#define IN_SWITCH_1				1011
#define IN_SWITCH_2				1012
#define IN_SWITCH_3				1013

#define ENCODER1_PIN_CLK		30
#define ENCODER1_PIN_DT			31
#define ENCODER1_PIN_SW			32

#define ENCODER2_PIN_CLK		2
#define ENCODER2_PIN_DT			3
#define ENCODER2_PIN_SW			4

#define ENCODER3_PIN_CLK		5
#define ENCODER3_PIN_DT			6
#define ENCODER3_PIN_SW			7

#define ENCODER_SW_DEBOUNCE		110






typedef struct _encrd {
	int16_t buttonPress;		// number of times switch was pressed since last read. can be zero.
	int16_t positionChange;	    // cumulative distance travelled since last read. can be zero.
}encoderrd_t;

typedef struct _encsrd {
	uint16_t size;		// size (in bytes) of this struct
	uint16_t total;		// number of encoders described (ie; ENCODER_TOTAL). does not change between hardware.
	uint32_t changed;	// what changed (packed)
	encoderrd_t encoder[ENCODER_TOTAL];
}encodersrd_t;




void encoders_init ();
int encoders_isReady ();
int encoders_read (encodersrd_t *encoders);



#endif


#endif


