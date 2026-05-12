/**
 * C/MRI INIT handler example
 * ===========================
 * Demonstrates how to register an INIT callback to inspect the
 * configuration payload sent by JMRI at startup.
 *
 * The INIT message body follows NMRA LCS-9.10.1:
 *   byte 0:   NDP (Node Definition Parameter)
 *   bytes 1-2: DLH/DLL (transmit delay, high & low bytes)
 *   bytes 3+:  node-type-specific options
 *
 * The transmit delay is computed as (DLH * 256 + DLL) * 10 microseconds.
 * This example reads the delay and prints the configuration to Serial.
 *
 * To set up in JMRI:
 * 1: Create a new C/MRI connection (Serial, 9600 baud)
 * 2: Configure a node with address 0 — JMRI sends an INIT at startup
 * 3: Open Tools > Tables > Lights and add a light at address 1
 * 4: Open the C/MRI Monitor to watch the INIT message
 *    Raw format: [41 49 43 00 0A ...] = UA 'A', cmd 'I', NDP='C', DL=10
 */

#include <CMRI.h>

CMRI cmri; // defaults to a SMINI with address 0

// ---------------------------------------------------------------------------
// INIT handler callback
// Called automatically when an INIT ('I') packet is received.
// data  – pointer to the raw INIT payload bytes
// len   – number of bytes in the payload
// ---------------------------------------------------------------------------
void on_init(const uint8_t *data, int len)
{
	Serial.print(F("INIT received ("));

	if (len >= 1)
	{
		Serial.print(F("NDP="));
		Serial.write(data[0]);
		Serial.print(F(" "));
	}

	if (len >= 3)
	{
		int delay_us = (data[1] * 256 + data[2]) * 10;
		Serial.print(F("DL="));
		Serial.print(delay_us);
		Serial.print(F("us "));
	}

	if (len > 3)
	{
		Serial.print(F("options="));
		for (int i = 3; i < len; i++)
		{
			if (i > 3)
				Serial.print(F(" "));
			if (data[i] < 16)
				Serial.print(F("0"));
			Serial.print(data[i], HEX);
		}
		Serial.print(F(" "));
	}

	Serial.print(len);
	Serial.println(F(" bytes"));
}

void setup()
{
	Serial.begin(9600, SERIAL_8N2);
	cmri.set_init_handler(on_init);
	pinMode(13, OUTPUT);
}

void loop()
{
	cmri.process();
	digitalWrite(13, cmri.get_bit(0));
}
