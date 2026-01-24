
/*
		Port A = configured with all NAV MSG's bar Odo
		Port B = configured with only POSLLH and Odo
*/


#include <Arduino.h>
#include <string.h>
#include <inttypes.h>

#include "ubx/ubx.h"
#include "ubx/ubxcb.h"
#include "commonGlue.h"



int gnssReceiver_PassthroughEnabled = 0;
static ubx_device_t dev = {0};
extern const uint32_t baudRates[];

#if (!RECEIVER_SINGLE)
static uint8_t bufferReadX[2][16384];
static uint8_t bufferWriteX[2][512];
#else
static uint8_t bufferReadX[1][32768];
static uint8_t bufferWriteX[1][1024];
#endif


FLASHMEM void gps_updateReceiverBaudMenu (const uint32_t baud)
{
	for (int i = UI_ID_BUTTON_BAUD_9600; i <= UI_ID_BUTTON_BAUD_921600; i++)
		ui_setHighlight(i, 0);
		
	if (baud == 9600) ui_setHighlight(UI_ID_BUTTON_BAUD_9600, 1);
	else if (baud == 19200) ui_setHighlight(UI_ID_BUTTON_BAUD_19200, 1);
	else if (baud == 38400) ui_setHighlight(UI_ID_BUTTON_BAUD_38400, 1);
	else if (baud == 57600) ui_setHighlight(UI_ID_BUTTON_BAUD_57600, 1);
	else if (baud == 115200) ui_setHighlight(UI_ID_BUTTON_BAUD_115200, 1);
	else if (baud == 230400) ui_setHighlight(UI_ID_BUTTON_BAUD_230400, 1);
	else if (baud == 460800) ui_setHighlight(UI_ID_BUTTON_BAUD_460800, 1);
	else if (baud == 921600) ui_setHighlight(UI_ID_BUTTON_BAUD_921600, 1);
}

FLASHMEM void gps_updateReceiverRateMenu (const uint32_t measRate)
{
	for (int i = UI_ID_BUTTON_RATE1; i <= UI_ID_BUTTON_RATE7; i++)
		ui_setHighlight(i, 0);
		
	int id = (UI_ID_BUTTON_RATE1-35)+measRate;
	if (id >= UI_ID_BUTTON_RATE1 && id <= UI_ID_BUTTON_RATE7)
		ui_setHighlight(id, 1);
}

FLASHMEM void gps_updateReceiverGNSSMenu (const cfg_gnss_t *gnss)
{
	for (int i = 0; i < gnss->numConfigBlocks; i++){
		const cfg_cfgblk_t *blk = &gnss->cfgblk[i];
		
		const int id = blk->gnssId&0x07;
		const int isEnabled = blk->flags&GNSS_CFGBLK_ENABLED&0x01;
		
		if (id == GNSSID_GPS){
			if (isEnabled)
				ui_enableReady(0, UI_ID_BUTTON_GPS);
			else
				ui_enableNotReady(0, UI_ID_BUTTON_GPS);
		}else if (id == GNSSID_GALILEO){
			if (isEnabled)
				ui_enableReady(0, UI_ID_BUTTON_GALILEO);
			else
				ui_enableNotReady(0, UI_ID_BUTTON_GALILEO);
		}else if (id == GNSSID_BEIDOU){
			if (isEnabled)
				ui_enableReady(0, UI_ID_BUTTON_BEIDOU);
			else
				ui_enableNotReady(0, UI_ID_BUTTON_BEIDOU);
		}else if (id == GNSSID_GLONASS){
			if (isEnabled)
				ui_enableReady(0, UI_ID_BUTTON_GLONASS);
			else
				ui_enableNotReady(0, UI_ID_BUTTON_GLONASS);
		}else if (id == GNSSID_SBAS){
			if (isEnabled)
				ui_enableReady(0, UI_ID_BUTTON_SBAS);
			else
				ui_enableNotReady(0, UI_ID_BUTTON_SBAS);
		}else if (id == GNSSID_QZSS){
			if (isEnabled)
				ui_enableReady(0, UI_ID_BUTTON_QZSS);
			else
				ui_enableNotReady(0, UI_ID_BUTTON_QZSS);
		}else if (id == GNSSID_IMES){
			if (isEnabled)
				ui_enableReady(0, UI_ID_BUTTON_IMES);
			else
				ui_enableNotReady(0, UI_ID_BUTTON_IMES);
		}
	}
}

FLASHMEM uint8_t gps_getPortActive ()
{
	if (dev.uart == dev.uartPort[0])
		return 1;
	else if (dev.uart == dev.uartPort[1])
		return 2;

	return 1;
}

static inline HardwareSerialIMXRT *gps_setPort (const uint8_t port)
{
	dev.portNumber = port;
	if (port == 1){
		dev.uart = (HardwareSerialIMXRT*)dev.uartPort[0];
		return (HardwareSerialIMXRT*)dev.uart;
	}else if (port == 2){
		dev.uart = (HardwareSerialIMXRT*)dev.uartPort[1];
		return (HardwareSerialIMXRT*)dev.uart;
	}
	return (HardwareSerialIMXRT*)dev.uart;
}

static inline void gps_setBuffers (const uint8_t port)
{
	if (port == 1){
		gps_setPort(port);
		((HardwareSerialIMXRT*)dev.uart)->addMemoryForRead(bufferReadX[0], sizeof(bufferReadX[0]));
		((HardwareSerialIMXRT*)dev.uart)->addMemoryForWrite(bufferWriteX[0], sizeof(bufferWriteX[0]));
	}else if (port == 2 && (RECEIVER_SINGLE == 0)){
		gps_setPort(port);
		((HardwareSerialIMXRT*)dev.uart)->addMemoryForRead(bufferReadX[1], sizeof(bufferReadX[1]));
		((HardwareSerialIMXRT*)dev.uart)->addMemoryForWrite(bufferWriteX[1], sizeof(bufferWriteX[1]));
	}
}

static const inline uint32_t gps_getDefaultConfig (const uint8_t port)
{
	if (port == 1){
		uint32_t portA = RECEIVER_CFG_DEVPORTA;

		portA |= RECEIVER_CFG_CLEAN|RECEIVER_CFG_OPAQUE|RECEIVER_CFG_CALLBACK|RECEIVER_CFG_HANDLER;
		portA |= RECEIVER_CFG_Ports|RECEIVER_CFG_Inf|RECEIVER_CFG_Rate|RECEIVER_CFG_GNSS;
		portA |= RECEIVER_CFG_Nav5|RECEIVER_CFG_NavX5;
		portA |= RECEIVER_CFG_MSG_DISABLEALL;
		portA |= RECEIVER_CFG_MSG_POSLLH|RECEIVER_CFG_MSG_PVT|RECEIVER_CFG_MSG_DOP;
		portA |= RECEIVER_CFG_MSG_POSECEF|RECEIVER_CFG_MSG_SAT|RECEIVER_CFG_MSG_STATUS;
		portA |= RECEIVER_CFG_POLL;
		
		if (RECEIVER_SINGLE)
			portA |= RECEIVER_CFG_ODO_RESET|RECEIVER_CFG_MSG_ODO|RECEIVER_CFG_Odo;
		return portA;
	}

	if (port == 2){
		uint32_t portB = RECEIVER_CFG_DEVPORTB;

		portB |= RECEIVER_CFG_Ports|RECEIVER_CFG_Inf|RECEIVER_CFG_Rate|RECEIVER_CFG_GNSS;
		portB |= RECEIVER_CFG_Nav5|RECEIVER_CFG_NavX5|RECEIVER_CFG_Odo;
		portB |= RECEIVER_CFG_ODO_RESET|RECEIVER_CFG_MSG_DISABLEALL;
		portB |= RECEIVER_CFG_MSG_POSLLH|RECEIVER_CFG_MSG_ODO;
		portB |= RECEIVER_CFG_MSG_STATUS;
		
		return portB;
	}
	
	return 0;
}

int isSerialConsoleConnected ()
{
	return (int)Serial.dtr();
}

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
#if (!RECEIVER_SINGLE)
	gps_setPort(2);
	ubx_sos_poll(&dev);
#endif
	
	gps_setPort(1);
	ubx_sos_poll(&dev);
}

void gps_sosClearFlash ()
{
#if (!RECEIVER_SINGLE)
	gps_setPort(2);
	ubx_sos_clear(&dev);
#endif

	gps_setPort(1);
	ubx_sos_clear(&dev);
}

void gps_sosCreateBackup ()
{
#if (!RECEIVER_SINGLE)
	gps_setPort(2);
	ubx_sos_backup(&dev);
#endif
	gps_setPort(1);
	ubx_sos_backup(&dev);
}

void gps_printVersions ()
{
#if (!RECEIVER_SINGLE)
	gps_setPort(2);
	ubx_printVersions(&dev);
#endif
	gps_setPort(1);
	ubx_printVersions(&dev);
}

void gps_printPositionAlt ()
{
	pos_rec_t loc = log_getLastPosition();
	printf(CS("Longitude: %.8f"), loc.longitude);
	printf(CS("Latitude:  %.8f"), loc.latitude);
	printf(CS("Altitude:  %.1f"), loc.altitude);
}

void gps_printStatus ()
{
#if (!RECEIVER_SINGLE)
	gps_setPort(2);
	ubx_printStatus(&dev);
#endif
	gps_setPort(1);
	ubx_printStatus(&dev);
}

void gps_coldStart ()
{
#if (!RECEIVER_SINGLE)
	gps_setPort(2);
	ubx_coldStart(&dev);
#endif
	gps_setPort(1);
	ubx_coldStart(&dev);
}

void gps_warmStart ()
{
#if (!RECEIVER_SINGLE)
	gps_setPort(2);
	ubx_warmStart(&dev);
#endif
	gps_setPort(1);
	ubx_warmStart(&dev);
}

void gps_hotStart ()
{
#if (!RECEIVER_SINGLE)
	gps_setPort(2);
	ubx_hotStart(&dev);
#endif
	gps_setPort(1);
	ubx_hotStart(&dev);
}

void gps_reinit ()
{
#if (!RECEIVER_SINGLE)
	gps_setPort(2);
	receiver_configure(&dev, gps_getDefaultConfig(2), 0);
#endif
	gps_setPort(1);
	receiver_configure(&dev, gps_getDefaultConfig(1), 0);
}

void gps_resetOdo ()
{
	gps_setPort(2);
	ubx_odo_reset(&dev);
	gps_setPort(1);
}

void gps_startOdo ()
{
	gps_setPort(2);
	ubx_odo_start(&dev);
	gps_setPort(1);
}

void gps_stopOdo ()
{
	gps_setPort(2);
	ubx_odo_stop(&dev);
	gps_setPort(1);
}

void gps_setRate (const uint32_t rate)
{
#if (!RECEIVER_SINGLE)
	gps_setPort(2);
	ubx_setRate(&dev, rate);
#endif
	gps_setPort(1);
	ubx_setRate(&dev, rate);
	
	gps_updateReceiverRateMenu(rate);
}

int gps_writeUbx (void *buffer, const uint32_t bufferSize)
{
	HardwareSerialIMXRT *uart = (HardwareSerialIMXRT*)dev.uart;
	return uart->write((uint8_t*)buffer, bufferSize);
}

void ms_delay (const uint32_t timeMs)
{
	delay(timeMs);
}

int gps_serialWrite (uint8_t *buffer, uint32_t bufferSize)
{
	HardwareSerialIMXRT *uart = (HardwareSerialIMXRT*)dev.uart;
	return uart->write(buffer, bufferSize);
}

void reciever_baudRateDiscover (ubx_device_t *dev)
{
	HardwareSerialIMXRT *uart = (HardwareSerialIMXRT*)dev->uart;

	for (int i = 0; baudRates[i]; i++){
		printf(CS("Baud rate set: %i"), (int)baudRates[i]);
		uart->begin(baudRates[i]);
		uart->clear();
		delay(100);	
		receiver_configurePorts(dev);
		delay(200);
		uart->flush();
		uart->clear();
		uart->end();
		delay(200);
	}
}

void gps_baudDiscover ()
{
	reciever_baudRateDiscover(&dev);
}

uint32_t gps_getBaud ()
{
	return dev.uartBaud[0];
}

void gps_setBaud (const uint32_t baud)
{
	for (int i = 0; baudRates[i]; i++){
		if (baudRates[i] == baud){
			dev.uartBaud[0] = baud;
			dev.uartBaud[1] = baud;
			gps_updateReceiverBaudMenu(baud);
			printf(CS("Baud rate set: %i"), (int)baudRates[i]);
			return;
		}
	}
	cmdSendResponse("Invalid baud rate");
}

void gps_reconnect ()
{
	HardwareSerialIMXRT *uart;
	
#if (!RECEIVER_SINGLE)
	// Port B
	uart = gps_setPort(2);
	
	//reciever_baudReset(&dev);
	//delay(100);
	
	uart->clear();
	uart->end();
	uart->begin(dev.uartBaud[1]);

	gps_setBuffers(1);
	delay(100);
	uart->clear();
	receiver_configure(&dev, gps_getDefaultConfig(2), 0);
	delay(100);
	receiver_configurePorts(&dev);
#endif

	// Port A
	uart = gps_setPort(1);
	
	//reciever_baudReset(&dev);
	//delay(100);
	
	uart->clear();
	uart->end();
	uart->begin(dev.uartBaud[0]);
	
	gps_setBuffers(1);
	delay(100);
	uart->clear();
	receiver_configure(&dev, gps_getDefaultConfig(1), 0);
	delay(100);
	receiver_configurePorts(&dev);
}

void gps_reconnect_noConfigure ()
{
	HardwareSerialIMXRT *uart;
	
#if (!RECEIVER_SINGLE)
	// Port B

	cmdSendResponse("Port 2..");
	uart = gps_setPort(2);
	
	//reciever_baudReset(&dev);
	//cmdSendResponse("Port 2 Buad reset");
	//delay(100);
	
	uart->clear();
	uart->end();
	uart->begin(dev.uartBaud[1]);

	gps_setBuffers(2);
	delay(100);
	uart->clear();
	delay(100);
	receiver_configurePorts(&dev);
#endif

	// Port A
	cmdSendResponse("Port 1..");
	uart = gps_setPort(1);
	
	//reciever_baudReset(&dev);
	//cmdSendResponse("Port 1 Buad reset");
	//delay(100);
	
	uart->clear();
	uart->end();	
	uart->begin(dev.uartBaud[0]);
	
	gps_setBuffers(1);
	delay(100);
	uart->clear();
	delay(100);
	receiver_configurePorts(&dev);
}

void gps_reconfigure ()
{
	gps_setPort(2);
	receiver_configure(&dev, gps_getDefaultConfig(2), 1);
	gps_setPort(1);
	receiver_configure(&dev, gps_getDefaultConfig(1), 0);
}

FLASHMEM static void gps_setup (ubx_device_t *dev, const uint8_t port, const uint32_t baud)
{
	HardwareSerialIMXRT *uart = gps_setPort(port);
	//reciever_baudReset(dev);
	//delay(100);
	uart->begin(baud);
	delay(100);
	uart->clear();
	receiver_configure(dev, gps_getDefaultConfig(port), 0);
}

static void gps_setIntialPosition (const double lat, const double lon, const float alt_meters, const uint32_t posAcc_cm)
{
	ubx_mga_ini_posllh(&dev, lat, lon, alt_meters, posAcc_cm);
}

void gps_loadOfflineAssist (const int printInfo)
{
	gps_setIntialPosition(MY_LAT, MY_LON, MY_ALT, 200);

	return;

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

	dev.uartBaud[0] = UART_BAUD;
	dev.uartBaud[1] = UART_BAUD;
	
#if (!RECEIVER_SINGLE)
	dev.uartPort[0] = &Serial1;		// Port A
	dev.uartPort[1] = &Serial2;		// Port B

	gps_setBuffers(2);
	gps_setup(&dev, 2, dev.uartBaud[1]);
	gps_loadOfflineAssist(1);
#else

	dev.uartPort[0] = &Serial1;		// Port A
	dev.uartPort[1] = dev.uartPort[0];
#endif

	gps_setBuffers(1);
	gps_setup(&dev, 1, dev.uartBaud[0]);
	gps_loadOfflineAssist(1);
	
	// set default port
	gps_setPort(1);
	
	gps_updateReceiverBaudMenu(UART_BAUD);
}

static void serial_Event1 (ubx_device_t *dev)
{
	HardwareSerialIMXRT *uart;
	
#if (!RECEIVER_SINGLE)
	uart = gps_setPort(2);
    while (uart->available()){
        dev->buffer[1].port[dev->buffer[1].portLen++] = uart->read();
        
        if (dev->buffer[1].portLen == 64 /*sizeof(dev->buffer[1].port)*/){
			ubx_processBlock(dev->buffer[1].port, dev->buffer[1].portLen, dev->buffer[1].compose, &dev->buffer[1].ubx_index, &dev->buffer[1].ubx_fill);
			dev->buffer[1].portLen = 0;
		}
    }
#endif
    
	uart = gps_setPort(1);
    while (uart->available()){
        dev->buffer[0].port[dev->buffer[0].portLen++] = uart->read();
        
        if (dev->buffer[0].portLen == 64/*sizeof(dev->buffer[0].port)*/){
        	/*if (Serial.dtr()){
				Serial.write(dev->bufferPort, dev->bufferPortLen);
       			Serial.flush();
       		}*/
			ubx_processBlock(dev->buffer[0].port, dev->buffer[0].portLen, dev->buffer[0].compose, &dev->buffer[0].ubx_index, &dev->buffer[0].ubx_fill);
			dev->buffer[0].portLen = 0;
		}
    }
}

void gps_task ()
{
	if (!gnssReceiver_PassthroughEnabled){
		serial_Event1(&dev);
		return;
	}
	
	HardwareSerialIMXRT *uart = gps_setPort(gnssReceiver_PassthroughEnabled);
	if (Serial.available())     	  // If anything comes in Serial (USB),
		uart->write(Serial.read());   // read it and send it out Serial1 (pins 0 & 1)

	if (uart->available())
		Serial.write(uart->read());   // read it and send it out Serial (USB)
}
