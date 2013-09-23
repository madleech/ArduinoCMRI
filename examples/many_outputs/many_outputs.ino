#include <CMRI.h>

#define LATCH 8
#define CLOCK 12
#define DATA  11

CMRI cmri; // defaults to a SMINI with address 0. SMINI = 24 inputs, 48 outputs

void setup() {
  Serial.begin(9600); // make sure this matches your speed set in JMRI
  pinMode(LATCH, OUTPUT);
  pinMode(CLOCK, OUTPUT);
  pinMode(DATA,  OUTPUT);
}

void loop() {
  // 1: main processing node of cmri library
  cmri.process();
  
  // 2: update output. Reads bit 0 of T packet and sets the LED to this
  digitalWrite(LATCH, LOW);
  shiftOut(DATA, CLOCK, MSBFIRST, cmri.get_byte(0));
  digitalWrite(LATCH, HIGH);
}


