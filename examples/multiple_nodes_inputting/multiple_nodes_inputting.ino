/**
 * An example of driving multiple C/MRI nodes on a single Arduino
 * ==============================================================
 * Sets up pins 2 and 3 as outputs, and attaches each to a different SMINI node.
 * 
 * To set up in JMRI:
 * 1: Create a new connection, 
 *    - type = C/MRI, 
 *    - connection = Serial, 
 *    - port = <arduino's port>, 
 *    - speed = 9600
 * 2: Click 'Configure C/MRI nodes' and click 'Add Node' to create a new SMINI node
 * 3: In the same window, type in '1' as the address and click 'Add Node' to add a second SMINI node
 * 4: Click 'Done'
 * 5: Restart J/MRI and it should say "Serial: using Serial on COM<x>" - congratulations!
 * 6: Open Tools > Tables > Lights and click 'Add'
 * 7: Add a new light at hardware address 0001, then click 'Create'
 * 8: Enter hardware address 1001 and click 'Create'. This will create a second light.
 * 8: Click the 'Off' state button to turn each LED on. Congratulations!
 */

#include <CMRI.h>

CMRI cmri0(0); // first SMINI, 24 inputs, 48 outputs
CMRI cmri1(1); // second SMINI, another 24 inputs and another 48 outputs

void setup() {
  Serial.begin(9600); // make sure this matches your speed set in JMRI
  
  cmri0.set_bit(0, HIGH);  // system name CS0001
  cmri0.set_bit(22, LOW);  // system name CS0023
  cmri0.set_bit(23, HIGH); // system name CS0024
  
  cmri1.set_bit(0, HIGH);  // system name CS1001
  cmri1.set_bit(1, HIGH);  // system name CS1002
  cmri1.set_bit(22, HIGH); // system name CS10023
}

char c;
void loop() {
  // 1: main processing node of cmri library
  while (Serial.available() > 0)
  {
    c = Serial.read();
    cmri0.process_char(c);
    cmri1.process_char(c);
  }
}
