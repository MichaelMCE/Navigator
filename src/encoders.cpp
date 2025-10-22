


#include "config.h"

#if ENABLE_ENCODERS

#include "encoder/encoder.h"
#include "commonGlue.h"



typedef struct {
	Encoder *enc;
	volatile int posNew;
	volatile int swChange;
	int pos;
}dial_t;


static dial_t dial1;
static dial_t dial2;
static dial_t dial3;


void enc1Update (const int value)
{
	if (!(value&0x03)){
		if (value != dial1.pos){
			dial1.posNew = dial1.pos - value;
			dial1.pos = value;
		}
	}
}

void enc2Update (const int value)
{
	if (!(value&0x03)){
		if (value != dial2.pos){
			dial2.posNew = dial2.pos - value;
			dial2.pos = value;
		}
	}
}

void enc3Update (const int value)
{
	if (!(value&0x03)){
		if (value != dial3.pos){
			dial3.posNew = dial3.pos - value;
			dial3.pos = value;
		}
	}
}

static void enc1Reset ()
{
	dial1.posNew = 0;
	dial1.swChange = 0;
}

static void enc2Reset ()
{
	dial2.posNew = 0;
	dial2.swChange = 0;
}

static void enc3Reset ()
{
	dial3.posNew = 0;
	dial3.swChange = 0;
}

void enc1SwCB ()
{
	static uint32_t lastPressTime;
	
	uint32_t currentPressTime = millis();
	uint32_t t1 = currentPressTime - lastPressTime;
	
	if (t1 > ENCODER_SW_DEBOUNCE){
		lastPressTime = currentPressTime;
		dial1.swChange++;
	}

	//printf(CS("1: %i %i"), t1, dial1.swChange);
}

void enc2SwCB ()
{
	static uint32_t lastPressTime;
	
	uint32_t currentPressTime = millis();
	uint32_t t1 = currentPressTime - lastPressTime;
	
	if (t1 > ENCODER_SW_DEBOUNCE){
		lastPressTime = currentPressTime;
		dial2.swChange++;
	}
	
	//printf(CS("2: %i %i"), t1, dial2.swChange);
}

void enc3SwCB ()
{
	static uint32_t lastPressTime;
	
	uint32_t currentPressTime = millis();
	uint32_t t1 = currentPressTime - lastPressTime;

	if (t1 > ENCODER_SW_DEBOUNCE){
		lastPressTime = currentPressTime;
		dial3.swChange++;
	}
	
	//printf(CS("3: %i %i"), t1, dial3.swChange);
}

int encoders_isReady ()
{
	return  dial1.swChange || dial1.posNew!=0 ||
			dial2.swChange || dial2.posNew!=0 ||
			dial3.swChange || dial3.posNew!=0;
}

int encoders_read (encodersrd_t *encoders)
{
	encoders->encoder[0].buttonPress    = dial1.swChange;
	encoders->encoder[0].positionChange = dial1.posNew;
	enc1Reset();
	
	encoders->encoder[1].buttonPress    = dial2.swChange;
	encoders->encoder[1].positionChange = dial2.posNew;
	enc2Reset();
	
	encoders->encoder[2].buttonPress    = dial3.swChange;
	encoders->encoder[2].positionChange = dial3.posNew;
	enc3Reset();

	uint16_t somethingHappenedSW = 0;
	uint16_t somethingHappenedPS = 0;

	for (int i = 0; i < ENCODER_TOTAL; i++){
		somethingHappenedSW +=  encoders->encoder[i].buttonPress;
		somethingHappenedPS += (encoders->encoder[i].positionChange != 0);
	}

	encoders->changed = ((somethingHappenedSW<<16) | (somethingHappenedPS&0xFFFF));
	return encoders->changed;
	//return somethingHappenedSW | somethingHappenedPS;
}

void encoders_dials_init ()
{
	dial1.pos = -1;
	dial1.posNew = 0;
	dial1.swChange = 0;
	dial1.enc = new Encoder(ENCODER1_PIN_CLK, ENCODER1_PIN_DT, enc1Update);
	
	dial2.pos = -1;
	dial2.posNew = 0;
	dial2.swChange = 0;
	dial2.enc = new Encoder(ENCODER2_PIN_CLK, ENCODER2_PIN_DT, enc2Update);
	
	dial3.pos = -1;
	dial3.posNew = 0;
	dial3.swChange = 0;
	dial3.enc = new Encoder(ENCODER3_PIN_CLK, ENCODER3_PIN_DT, enc3Update);
}

void encoders_pins_init ()
{
	pinMode(LED_BUILTIN, OUTPUT);
	
	pinMode(ENCODER1_PIN_SW, INPUT_PULLDOWN);
	attachInterrupt(ENCODER1_PIN_SW, enc1SwCB, RISING);

	pinMode(ENCODER2_PIN_SW, INPUT_PULLDOWN);
	attachInterrupt(ENCODER2_PIN_SW, enc2SwCB, RISING);
	
	pinMode(ENCODER3_PIN_SW, INPUT_PULLDOWN);
	attachInterrupt(ENCODER3_PIN_SW, enc3SwCB, RISING);
}

void encoders_init ()
{
	encoders_pins_init();
	encoders_dials_init();
}

#endif

