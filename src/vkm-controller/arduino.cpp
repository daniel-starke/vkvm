/**
 * @file arduino.cpp
 * @author Daniel Starke
 * @copyright Copyright 2020-2022 Daniel Starke
 * @date 2020-08-23
 * @version 2023-10-25
 *
 * USB-UART bridge. Only supports 8N1.
 */
#include <Arduino.h>


/** Pin for USB1 VBUS sense. */
#define PIN_USB1_SENSE PA_4
/** Pin for USB2 VBUS sense. */
#define PIN_USB2_SENSE PA_5


/** Maximum buffer size for both directions. */
#define MAX_BUFFER USB_EP_SIZE /* bytes */
/** Maximum data latency in milliseconds. */
#define MAX_LATENCY 8 /* ms */


uint8_t usbInBuf[MAX_BUFFER]; /**< USB to UART buffer. */
uint8_t usbOutBuf[MAX_BUFFER]; /**< UART to USB buffer. */
uint32_t usbInLen; /**< Number of bytes in `usbInBuf`. */
uint32_t usbOutLen; /**< Number of bytes in `usbOutBuf`. */
uint32_t baudRate; /**< Baudrate used for the UART (taken from USB interface). */
uint32_t lastOut; /**< Timestamp at which the most recent data was sent to USB. */
bool hasLastOut; /**< True if `lastOut` is valid, else false. */


/**
 * Sends the buffered data from the UART to the USB interface.
 *
 * @param[in] now - current time stamp in milliseconds
 */
static void sendToUsb(const uint32_t now) {
	if (usbOutLen <= 0) return;
	const uint8_t * ptr = usbOutBuf;
	for (uint32_t rem = usbOutLen; rem > 0; ) {
		const size_t written = Serial.write(ptr, rem);
		ptr += written;
		rem = uint32_t(rem - written);
	}
	usbOutLen = 0;
	lastOut = now;
	hasLastOut = true;
}


/**
 * Sends the buffered data from the USB interface to the UART.
 */
static void sendToUart() {
	if (usbInLen <= 0) return;
	const uint8_t * ptr = usbInBuf;
	for (uint32_t rem = usbInLen; rem > 0; ) {
		const size_t written = Serial1.write(ptr, rem);
		ptr += written;
		rem = uint32_t(rem - written);
	}
	usbInLen = 0;
}


/**
 * Initializes the application environment.
 */
void setup() {
	baudRate = 115200;
	Serial.begin(baudRate); /* the Baud rate here is just the initial value and may be set by the client */
	Serial1.begin(baudRate);
	while ( ! Serial1 ); /* wait for UART to be ready */
	hasLastOut = false;
}


/**
 * Main processing loop.
 */
void loop() {
	const uint32_t now = millis();

	/* handle Baud rate changes */
	const uint32_t newBaudRate = Serial.baud();
	if (newBaudRate != baudRate) {
		baudRate = newBaudRate;
		Serial1.end();
		Serial1.begin(baudRate);
		while ( ! Serial1 ); /* wait for UART to be ready */
	}

	/* handle latency constraints */
	if (Serial.dtr() && hasLastOut && uint32_t(now - lastOut) >= MAX_LATENCY) {
		sendToUsb(now);
	}

	/* handle data from UART */
	if ( Serial1 ) {
		while (usbOutLen < MAX_BUFFER && Serial1.available() > 0) {
			const int value = Serial1.read();
			if (value >= 0) {
				usbOutBuf[usbOutLen++] = uint8_t(value);
			}
		}
		if (hasLastOut == false && usbOutLen > 0) {
			lastOut = now;
			hasLastOut = true;
		}
		if (Serial.dtr() && usbOutLen >= MAX_BUFFER) {
			sendToUsb(now);
		}
	}

	/* handle data from USB */
	if ( Serial.dtr() ) {
		while (usbInLen < MAX_BUFFER && Serial.available() > 0) {
			const int value = Serial.read();
			if (value >= 0) {
				usbInBuf[usbInLen++] = uint8_t(value);
			}
		}
		if ( Serial1 ) {
			sendToUart();
		}
	}
}
