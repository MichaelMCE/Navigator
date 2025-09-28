

#include <Arduino.h>
#include <string.h>
#include <inttypes.h>

#include "ubx/ubx.h"
#include "ubx/ubxcb.h"
#include "gps.h"


//#define GPS_PASSTHROUGH_ENABLED		(0)


int GPS_PASSTHROUGH_ENABLED = 0;


ubx_device_t dev = {0};
extern const uint32_t baudRates[];
static uint8_t bufferReadX[2024];
static uint8_t bufferWriteX[2024];
const uint32_t bufferSize = 2048*1;
static uint8_t buffer[bufferSize+16];
static int32_t bOffset = 0;
static uint8_t serBuffer[8];




void ms_delay (const uint32_t timeMs)
{
	delay(timeMs);
}

int gps_serialWrite (uint8_t *buffer, uint32_t bufferSize)
{
	return Serial1.write(buffer, bufferSize);
}

static inline void reciever_baudReset (ubx_device_t *dev)
{
	for (int i = 0; baudRates[i]; i++){
		Serial1.begin(baudRates[i]);
		Serial1.clear();

		delay(50);	
		gps_configurePorts(dev);
		
		delay(200);
		Serial1.clear();
		Serial1.end();
		delay(200);
	}
}

static void gps_setup (ubx_device_t *dev)
{
	Serial1.addMemoryForRead(bufferReadX, sizeof(bufferReadX));
	Serial1.addMemoryForWrite(bufferWriteX, sizeof(bufferWriteX));
	
	reciever_baudReset(dev);
	delay(100);
	Serial1.begin(SERIAL_RATE);
	delay(100);
	Serial1.clear();
	gps_configure(dev);
}

void gps_init ()
{
	memset(&dev, 0, sizeof(dev));
	gps_setup(&dev);
}

void serialEvent1 ()
{
	static uint8_t len = 0;

	if (GPS_PASSTHROUGH_ENABLED)
		return;
	
    while (Serial1.available()) {
        serBuffer[len++] = Serial1.read();
        
        if (len == 8){
        	/*if (Serial.dtr()){
				Serial.write(serBuffer, len);
       			Serial.flush();
       		}*/
        	
			gps_ubxMsgRun(&dev, buffer, bufferSize, &bOffset, serBuffer, len);
			len = 0;
		}
    }
}

void gps_task ()
{
	if (!GPS_PASSTHROUGH_ENABLED)
		return;

	if (Serial.available())     		// If anything comes in Serial (USB),
		Serial1.write(Serial.read());   // read it and send it out Serial1 (pins 0 & 1)

	if (Serial1.available())
		Serial.write(Serial1.read());   // read it and send it out Serial (USB)
}
