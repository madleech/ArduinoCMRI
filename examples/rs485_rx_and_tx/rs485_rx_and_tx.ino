#include <Auto485.h>
#include <CMRI.h>

#define CMRI_ADDR 0

#define    DE_PIN 2
#define   LED_PIN 3

Auto485 bus(DE_PIN); // Arduino pin 2 -> MAX485 DE and RE pins
CMRI cmri(CMRI_ADDR, 24, 48, bus); // defaults to a SMINI with address 0. SMINI = 24 inputs, 48 outputs

void setup() {
  bus.begin(9600); // open RS485 bus at 9600bps
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  // 1: main processing node of cmri library
  cmri.process();
  
  // 2: update output. Reads bit 0 of T packet and sets the LED to this
  digitalWrite(LED_PIN, cmri.get_bit(0));
  
  // 3: update input. Flips a bit back and forth every second or so
  cmri.set_bit(0, (millis() / 1000) % 2 == 0);
}

