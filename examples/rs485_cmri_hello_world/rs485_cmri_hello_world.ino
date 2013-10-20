/**
 * C/MRI -> JMRI via RS485
 * =======================
 * Uses an RS485 bus to transparently talk support multiple ArduinoCMRI nodes on one bus.
 * By passing in an Auto485 object to the CMRI constructor, we are able to automatically
 * control the DE and RE pins on our RS485 bus transceiver.
 * 
 * Sets up pin 13 (LED) as an output, pin 12 as an input, and attaches both to 
 * the first output/input bits of the emulated SMINI interface.
 * 
 * To set up in JMRI, follow instructions in hello_world.
 * 
 */

#include <Auto485.h>
#include <CMRI.h>

Auto485 bus(2); // Arduino pin 2 -> MAX485 DE and RE pins
CMRI cmri(0, 24, 48, bus); // sets up an SMINI with address 0. SMINI = 24 inputs, 48 outputs

void setup() {
  pinMode(12, INPUT); digitalWrite(12, HIGH);
  pinMode(13, OUTPUT);
}

void loop() {
  // 1: main processing node of cmri library
  cmri.process();
  
  // 2: update output. Reads bit 0 of T packet and sets the LED to this
  digitalWrite(13, cmri.get_bit(0));
  
  // 3: update inputs
  cmri.set_bit(0, !digitalRead(12));
}

