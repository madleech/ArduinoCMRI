/**
 * An example of C/MRI inputs and outputs
 * ======================================
 * Sets up pins 2-5 as an outputs with LEDs, and pins 6-9 as inputs with pullups.
 * 
 * 1: Set up a JMRI connection, see hello_world, steps 1-4
 * 2: Open Tools > Tables > Lights and click 'Add'
 * 3: Add a new light at hardware address 1, then click 'Create'.
 * 4: Repeat for hardware address 2, 3, 4 and close the window. Ignore the save message.
 * 5: Click on 'Sensors' and set up new sensors for hardware address 1, 2, 3 and 4.
 * 6: You'll notice the TX and RX LEDs burst into life. This is JMRI polling the state of our sensors.
 * 7: Ground pin 6, you'll see sensor #1 go Active, while the rest are Inactive.
 * 8: Switch to Lights and play around with the State buttons. Congratulations!
 * 
 * Debugging:
 * Open the CMRI > CMRI Monitor window to check what is getting sent and received.
 * With 'Show raw data' turned on the output looks like:
 *    [41 50]  Poll ua=0
 *    [41 52 01 00 00]  Receive ua=0 IB=1 0 0 
 * 
 * 0x41 = 65 = A = address 0
 * 0x50 = 80 = P = poll, i.e. PC asking C/MRI to transmit its state back to PC
 * 
 * 0x41 = 65 = A = address 0
 * 0x52 = 82 = R = receive, i.e. PC receiving state data from C/MRI 
 * 0x01 = 0b00000001 = 1st bit is high
 * 0x00 = 0b00000000 = all other bits off
 */

#include <CMRI.h>

CMRI cmri; // defaults to a SMINI with address 0. SMINI = 24 inputs, 48 outputs

void setup() {
  Serial.begin(9600); // make sure this matches your speed set in JMRI
  for (int i=2; i<=5; i++) { pinMode(i, OUTPUT); }
  for (int i=6; i<=9; i++) { pinMode(i, INPUT); digitalWrite(i, HIGH); }
}

void loop() {
  // 1: build up a packet
  cmri.process();
  
  // 2: update outputs
  digitalWrite(2, cmri.get_bit(0));
  digitalWrite(3, cmri.get_bit(1));
  digitalWrite(4, cmri.get_bit(2));
  digitalWrite(5, cmri.get_bit(3));
  
  // 3: update inputs (invert digitalRead due to the pullups)
  cmri.set_bit(0, !digitalRead(6));
  cmri.set_bit(1, !digitalRead(7));
  cmri.set_bit(2, !digitalRead(8));
  cmri.set_bit(3, !digitalRead(9));
}



