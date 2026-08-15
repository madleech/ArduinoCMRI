/**
 * C/MRI INIT handler with transmit delay example
 * ================================================
 * Demonstrates how to register an INIT callback that parses the
 * transmit delay (dH/dL) from the INIT payload and applies it
 * using set_transmit_delay().
 *
 * The INIT message body follows NMRA LCS-9.10.1:
 *   byte 0:   NDP (Node Definition Parameter)
 *   bytes 1-2: dH/dL (transmit delay, high & low bytes)
 *   bytes 3+:  node-type-specific options
 *
 * The transmit delay is computed as (dH * 256 + dL) * 10 microseconds.
 * Modern hosts set dH/dL to zero; non-zero values are for legacy
 * compatibility. This delay is applied before each GET (R) reply to
 * allow RS-485 transceiver turnaround.
 *
 * To set up in JMRI:
 * 1: Create a new C/MRI connection (Serial, 9600 baud)
 * 2: Configure a node with address 0 — JMRI sends an INIT at startup
 * 3: Open Tools > Tables > Lights and add a light at address 1
 * 4: Open the C/MRI Monitor to watch the INIT message
 *
 * Wiring:
 *   Serial (pins 0/1) -> RS-485 transceiver -> host (JMRI)
 *   SoftwareSerial (pins 10/11) -> Serial Monitor for debug output
 */

#include <CMRI.h>
#include <SoftwareSerial.h>

SoftwareSerial console(10, 11); // RX, TX for debug output
CMRI cmri;                      // defaults to a SMINI with address 0, using Serial

// ---------------------------------------------------------------------------
// INIT handler callback
// Parses dH/dL and applies the transmit delay via set_transmit_delay().
// ---------------------------------------------------------------------------
void on_init(const uint8_t *data, int len)
{
	console.print(F("INIT received ("));

	if (len >= 1)
	{
		console.print(F("NDP="));
		console.write(data[0]);
		console.print(F(" "));
	}

	if (len >= 3)
	{
		unsigned int dH = data[1];
		unsigned int dL = data[2];
		unsigned int delay_us = (dH * 256 + dL) * 10;

		cmri.set_transmit_delay(delay_us);

		console.print(F("DL="));
		console.print(delay_us);
		console.print(F("us applied "));
	}

	console.print(len);
	console.println(F(" bytes"));
}

void setup()
{
	Serial.begin(9600, SERIAL_8N2); // CMRI bus
	console.begin(9600);            // debug output
	cmri.set_init_handler(on_init);
	pinMode(13, OUTPUT);
}

void loop()
{
	cmri.process();
	digitalWrite(13, cmri.get_bit(0));
}
