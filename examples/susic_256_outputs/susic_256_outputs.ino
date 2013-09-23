#include <CMRI.h>

CMRI cmri(0, 0, 256); // address 0, 0 inputs, 256 outputs

void setup() {
  Serial.begin(9600); // make sure this matches your speed set in JMRI
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
}

void loop() {
  // 1: main processing node of cmri library
  cmri.process();
  
  // 2: update output. Reads bit 0 of T packet and sets the LED to this
  digitalWrite(2, cmri.get_bit(0));
  digitalWrite(3, cmri.get_bit(255));
}
