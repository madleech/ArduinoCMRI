/**
 * A trivial C/MRI -> JMRI interface
 * =================================
 * Sets up pin 13 (LED) as an output, and attaches it to the first output bit
 * of the emulated SMINI interface.
 * 
 * To set up in JMRI:
 * 1: Create a new connection, 
 *    - type = C/MRI, 
 *    - connection = Serial, 
 *    - port = <arduino's port>, 
 *    - speed = 9600
 * 2: Click 'Configure C/MRI nodes' and create a new SMINI node
 * 3: Click 'Add Node' and then 'Done'
 * 4: Restart J/MRI and it should say "Serial: using Serial on COM<x>" - congratulations!
 * 5: Open Tools > Tables > Lights and click 'Add'
 * 6: Add a new light at hardware address 1, then click 'Create' and close the window. Ignore the save message.
 * 7: Click the 'Off' state button to turn the LED on. Congratulations!
 * 
 * Debugging:
 * Open the CMRI > CMRI Monitor window to check what is getting sent.
 * With 'Show raw data' turned on the output looks like:
 *    [41 54 01 00 00 00 00 00]  Transmit ua=0 OB=1 0 0 0 0 0 
 * 
 * 0x41 = 65 = A = address 0
 * 0x54 = 84 = T = transmit, i.e. PC -> C/MRI
 * 0x01 = 0b00000001 = turn on the 1st bit
 * 0x00 = 0b00000000 = all other bits off
 */

#include <CMRI.h>

CMRI cmri; // defaults to a SMINI with address 0. SMINI = 24 inputs, 48 outputs

void setup() {
  Serial.begin(9600); // make sure this matches your speed set in JMRI
  pinMode(13, OUTPUT);
}

void loop() {
  // 1: main processing node of cmri library
  cmri.process();
  
  // 2: update output. Reads bit 0 of T packet and sets the LED to this
  digitalWrite(13, cmri.get_bit(0));
}

