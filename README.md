ArduinoCMRI
===========

* Author: Michael Adams (<http://www.michael.net.nz>)
* Copyright (C) 2012 Michael D K Adams.
* Released under the MIT license.

ArduinoCMRI is an library for connecting your Arduino to your model railroad. It lets you easily interface lights, switches, servos, and other inputs and outputs with [JMRI][1], the Java Model Railroad Interface. It does this by emulating Bruce Chubb's [Computer/Model Railroad Interface][2] (C/MRI) System, an I/O system designed for interfacing model railroads with computers.

By emulating the C/MRI system you get maximum flexibility to tailor your solution to fit your needs, and you can reuse hardware you already own. A single Arduino can easily control hundreds of signals and points on your railroad!

Features:
* Simple API that handles GET, SET, and POLL requests from JMRI automatically.
* Easy access to input and output data.
* Emulates an SMINI up to a SUSIC with up to 2048 digital lines available.
* Error tolerant.

For documentation and examples please see the main [project blog][3].

[1]: http://jmri.org/
[2]: http://www.jlcenterprises.net/
[3]: http://utrainia.michael.net.nz/tag/cmri

Requirements
------------
* JMRI -- http://jmri.org/
* An Arduino -- http://arduino.cc/ -- **Please note**: Arduinos without a separate FTDI chip will not work. This includes the Arduino Leonardo, Uno, and Mega 2560. This is because the JMRI code opens the serial port with both DTR and RTS off, while the Arduino USB CDC code requires either DTR or RTS to be set before it will communicate; without these signals set, it believes there is no device attached. The easiest way around this limitation is to use a "basic" Arduino with an FTDI or other hardware USB-Serial converter.

Installation
------------
Download the ZIP archive (https://github.com/madleech/ArduinoCMRI/zipball/master) and extract it to your Arduino folder under `libraries/CMRI`.

Restart the Arduino IDE and you should see in File > Examples > CMRI entires for hello world and an input/output example.


Code Examples
-------------
Here is the 'hello\_world' example program, included in the download:

    #include <CMRI.h>
    
    CMRI cmri;
    
    void setup() {
      Serial.begin(9600, SERIAL_8N2); // SERIAL_8N2 to match what JMRI expects CMRI hardware to use
      pinMode(13, OUTPUT);
    }
    
    void loop() {
      cmri.process();
      digitalWrite(13, cmri.get_bit(0));
    }

This connects a Light in JMRI to the built in LED on your Arduino. Toggle the light in JMRI and your Arduino will light up.

The code is pretty simple. `#include <CMRI.h>` includes the library, while `CMRI cmri();` creates a new CMRI object with default values (address = 0, 24 inputs to PC, and 48 outputs from PC). The `Serial.begin` and `pinMode` lines set up our serial port and tell the LED pin that it's going to be an output.

The main loop is where the magic happens. `cmri.process()` receives data from the PC and processes it automatically, responding to the PC if required, and updating it's internal state ready for us to read the data back out.

To access the data we sent it, we use the `cmri.get_bit(n)` function to get the value of bit number *n*.

If you wanted to extend this demo to transmit data back to the PC, all you need to do is connect your digital inputs to `cmri.set_bit(n, value)` calls, and the `cmri.process()` function will take care of the rest.

Documentation
-------------
**CMRI(unsigned int address = 0, unsigned int input\_bits = 24, unsigned int output\_bits = 48)**
Creates a new CMRI object. The default values will create a device that matches the capabilities of an SMINI node. If you want to bind to a different node address, or address more or less inputs, you can alter it here. The maximum combined number of addressable inputs and outputs is 2048 (C/MRI limitation). The library will work fine with any number of inputs and outputs, it will simply ignore out-of-range data.

**void set\_address(unsigned int address)**
Sets the address of the C/MRI node.

**char process()**
Reads in available data from the serial port and acts accordingly:
* For POLL requests, it replies with the current state of the input data.
* For INIT requests, it does nothing.
* For SET/TRANSMIT (T) requests, it updates the output data.

Return value is NULL for no valid packet received, or one of CMRI::INIT, CMRI::SET, CMRI::POLL depending on the packet type received.

**bool process\_char(char c)**
Similar to the CMRI::process method, but lets you manage the serial data yourself. Use this if you are processing more than 1 CMRI node in a system.

Return value is true if a valid packet has been received and processing of it has finished. Otherwise it returns false.

**void transmit()**
Transmits the current state of the input data back to the PC. Creates a CMRI::GET packet.

**bool get\_bit(int n)**
Reads a bit from of the last valid input data received. Use this to update your signals, points, etc.

**char get\_byte(int n)**
Reads an entire byte from the input buffer. Use this with shiftOut and some shift registers to vastly expand your I/O capabilities.

**bool set\_bit(int n, bool b)**
Updates the output buffer to the specified value. Data will be transmitted to the PC either when transmit() is called, or when the next POLL packet is received.

**bool set\_byte(int n, char b)**
Updates an entire byte of the output buffer. Use this with shiftIn and some shift registers to add many extra digital inputs to your system.


Troubleshooting
---------------
**JMRI reports: unrecognized rep: "50 aa 00" etc / no responses to POLL requests**
Make sure your `Serial.begin(...)` line is `Serial.begin(9600, SERIAL_8N2)`. Real C/MRI hardware uses 8 data bits, no parity, and *2* stop bits, which is different to regular serial which only has 1 stop bit. Most (i.e. cheap) USB to serial adapters will usually ignore the differences, however some are more rigorous in their parsing of serial data and won't work unless the Arduino is actually transmitting using 2 stop bits. Hence the `SERIAL_8N2` in the `Serial.begin(...)` line.

Protocol
--------
The C/MRI protocol is fairly simple, however [documentation][4] can be difficult to come across.

The system usually runs over a serial bus (although an ISA bus version was also produced), and uses the slightly obscure protocol of 8 databits, no parity, and 2 stop bits. Luckily we can (usually, see above) get away with just using the default 8n1 @ 9600 setting that Arduino's use by default and USB takes care of the rest.

Each packet is framed as follows: `0xFF 0xFF 0x02` which is two 255 bits, and one STX (start) bit. The data then follows, and is closed by an ETX packet, `0x03`.

The first two bytes of data are always the address, and the packet type.

Address = decimal 65 + address, which corresponds to uppercase `A` for address = 0, `B` = 1, etc.

Packet type = uppercase ASCII `I`, `T`, `R`, or `P`, representing INIT, SET, GET and POLL respectively.

For GET and SET packet types, the data bytes then follow. These are just plain binary, however since 0x03 may represent the ETX end frame, any sequence of 0x03 in the data stream must be escaped by a preceeding 0x10 byte.

So a SET operation counting up in binary would look like:
`[41 54 00 00 00 00 00 00]`
`[41 54 01 00 00 00 00 00]`
`[41 54 02 00 00 00 00 00]`
`[41 54 10 03 00 00 00 00 00]` -- notice the escape byte in this one
`[41 54 04 00 00 00 00 00]`

The only documentation I could find for the STX and ETX byte values was by reading the JMRI source code.

[4]: http://home.roadrunner.com/~jimngage/TRACTRONICS/MicroController/mr89c52f.htm

License
-------
Copyright (c) 2012 Michael D K Adams. http://www.michael.net.nz

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

