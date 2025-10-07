

#include <Arduino.h>
#include <string.h>
#include <inttypes.h>

#include "ubx/ubx.h"
#include "ubx/ubxcb.h"
#include "gps.h"
#include "cmd.h"



int gnssReceiver_PassthroughEnabled = 0;


ubx_device_t dev = {0};
extern const uint32_t baudRates[];
static uint8_t bufferReadX[2024];
static uint8_t bufferWriteX[2024];
const uint32_t bufferSize = 2048*1;
static uint8_t buffer[bufferSize+16];
static int32_t bOffset = 0;
static uint8_t serBuffer[8];


void gps_pollInf (const uint8_t protocolID)
{
	ubx_msgInfPoll(&dev, protocolID);
}

int gps_pollMsg (const char *name)
{
	return ubx_msgPollName(&dev, name);
}

void gps_sosPoll ()
{
	ubx_sos_poll(&dev);
}

void gps_sosClearFlash ()
{
	ubx_sos_clear(&dev);
}

void gps_sosCreateBackup ()
{
	ubx_sos_backup(&dev);
}

void gps_printVersions ()
{
	ubx_printVersions(&dev);
}

void gps_printStatus ()
{
	ubx_printStatus(&dev);
}

void gps_coldStart ()
{
	ubx_coldStart(&dev);
}

void gps_warmStart ()
{
	ubx_warmStart(&dev);
}

void gps_hotStart ()
{
	ubx_hotStart(&dev);
}

void gps_resetOdo ()
{
	ubx_odo_reset(&dev);
}

void gps_startOdo ()
{
	ubx_odo_start(&dev);
}

void gps_stopOdo ()
{
	ubx_odo_stop(&dev);
}

int gps_writeUbx (void *buffer, const uint32_t bufferSize)
{
	//return ubx_write(&dev, (uint8_t*)buffer, bufferSize);
	return Serial1.write((uint8_t*)buffer, bufferSize);
}

void ms_delay (const uint32_t timeMs)
{
	delay(timeMs);
}

int gps_serialWrite (uint8_t *buffer, uint32_t bufferSize)
{
	return Serial1.write(buffer, bufferSize);
}

FLASHMEM static void reciever_baudReset (ubx_device_t *dev)
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

FLASHMEM static void gps_setup (ubx_device_t *dev)
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

void gps_loadOfflineAssist (const int printInfo)
{
	if (cmdLoadUbx(ASSISTNOW_FILENAME)){
		if (printInfo)
			addDebugLine((const uint8_t*)("AssistNow Offline: " ASSISTNOW_FILENAME " imported"));
	}else{
		if (printInfo)
			addDebugLine((const uint8_t*)("AssistNow Offline: " ASSISTNOW_FILENAME " import failed"));
	}
}

void gps_init ()
{
	memset(&dev, 0, sizeof(dev));
	gps_setup(&dev);
	gps_loadOfflineAssist(1);
}

void serialEvent1 ()
{
	static uint8_t len = 0;

	if (gnssReceiver_PassthroughEnabled)
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
	if (!gnssReceiver_PassthroughEnabled)
		return;

	if (Serial.available())     		// If anything comes in Serial (USB),
		Serial1.write(Serial.read());   // read it and send it out Serial1 (pins 0 & 1)

	if (Serial1.available())
		Serial.write(Serial1.read());   // read it and send it out Serial (USB)
}

