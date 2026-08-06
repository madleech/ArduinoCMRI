/*
    Hardware smoke-test for CMRI.

    Emulates a default SMINI node (address 0, 24 inputs, 48 outputs) over the
    hardware Serial port and mirrors the first output bit on the onboard LED:

      - digital pin 12 (with pull-up) is reported as input bit 0
      - output bit 0 (light CS0001 in JMRI) drives LED_BUILTIN

    Connect the board to a JMRI host configured for a C/MRI serial connection at
    9600 baud (SERIAL_8N2), then toggle light CS0001 to see the LED follow, and
    ground pin 12 to see the sensor go active. Without a JMRI host this simply
    confirms the sketch builds, uploads and boots against the local source.

    Upload + monitor:
      pio run -d extras/hardware_test -e uno -t upload
      pio device monitor -b 9600
*/

#include <Arduino.h>
#include <CMRI.h>

const uint8_t INPUT_PIN = 12;

CMRI cmri; // default SMINI: address 0, 24 inputs, 48 outputs, on Serial

void setup()
{
	Serial.begin(9600, SERIAL_8N2); // must match the speed set in JMRI
	pinMode(INPUT_PIN, INPUT_PULLUP);
	pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
	// Process any pending serial traffic (auto-replies to POLLs).
	cmri.process();

	// Mirror output bit 0 on the LED.
	digitalWrite(LED_BUILTIN, cmri.get_bit(0));

	// Report pin 12 as input bit 0 (inverted: grounded == active).
	cmri.set_bit(0, !digitalRead(INPUT_PIN));
}
