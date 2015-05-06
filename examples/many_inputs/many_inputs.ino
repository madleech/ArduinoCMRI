#include <CMRI.h>
#include <SPI.h>

// pins for a 168/368 based Arduino
#define SS    10
#define MOSI  11 /* not used */
#define MISO  12
#define CLOCK 13

CMRI cmri; // defaults to a SMINI with address 0. SMINI = 24 inputs, 48 outputs

void setup() {
  Serial.begin(9600, SERIAL_8N2); // make sure this matches your speed set in JMRI
  SPI.begin();
}

void loop() {
  // 1: main processing node of cmri library
  cmri.process();
  
  // 2: toggle the SS pin
  digitalWrite(SS, HIGH);
  delay(1); // wait while data CD4021 loads in data
  digitalWrite(SS, LOW);
  
  // 3: update input status in CMRI, will get sent to PC next time we're asked
  cmri.set_byte(0, SPI.transfer(0x00 /* dummy output value */));
}


