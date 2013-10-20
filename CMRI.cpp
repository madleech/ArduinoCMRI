/*
	CMRI - a small library for Arduino to interface with the C/MRI
	computer control system for model railroads
	Copyright (C) 2012 Michael Adams (www.michael.net.nz)
	All rights reserved.
	
	Permission is hereby granted, free of charge, to any person obtaining a 
	copy of this software and associated documentation files (the "Software"), 
	to deal in the Software without restriction, including without limitation 
	the rights to use, copy, modify, merge, publish, distribute, sublicense, 
	and/or sell copies of the Software, and to permit persons to whom the 
	Software is furnished to do so, subject to the following conditions:
	
	The above copyright notice and this permission notice shall be included 
	in all copies or substantial portions of the Software.
	
	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
	OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE 
	SOFTWARE.
*/

#include "CMRI.h"
#include <Arduino.h>

CMRI::CMRI(unsigned int address, unsigned int input_bits, unsigned int output_bits, Stream &serial_class)
	// store details
: _address(address)
, _rx_length((output_bits + 7) / 8)
, _tx_length((input_bits + 7) / 8)
, _serial(serial_class)

	// set state
, _rx_buffer((char *) malloc(_rx_length))
, _tx_buffer((char *) malloc(_tx_length))

	// parsing state
, _mode(MODE_INVALID0)
, _ignore_next_byte(false)
, _rx_index(0)
, _have_valid_packet(false)

{
	// clear to zero
	for(int i=0; i<_rx_length; i++)
		_rx_buffer[i] = 0;
	for(int i=0; i<_tx_length; i++)
		_tx_buffer[i] = 0;
}

void CMRI::set_address(unsigned int address)
{
	_address = address;
}

// reads in serial data, decodes packets
// automatically responds to POLL requests
// returns packet type so if we got a SET request you know to update your outputs
char CMRI::process()
{
	while (_serial.available() > 0)
	{
		if (process_char(_serial.read()) != false) // finished decoding a packet, so return its type
		{
			return _rx_packet_type;
		}
	}
	return NULL;
}

bool CMRI::process_char(char c)
{
	// if it's a SET that's fine do nothing
	// if it's an INIT that's also fine, we don't really care
	// if it's a GET, well, do nothing since it must be someone else replying
	// if it's a POLL then reply straight away with our data
	if (_decode(c) == true) // true => we've decoded a packet
	{
		if (_rx_packet_type == POLL) // packet was a poll, so transmit our status
		{
			transmit();
		}
		return true;
	}
	return false;
}



// public methods

bool CMRI::get_bit(int pos)
{
	// 1: divide index by 8 to get byte offset
	char c = get_byte(pos / 8);
	// 2: return bit at that location
	return (bool) ((c >> (pos % 8)) & 0x01);
}

char CMRI::get_byte(int pos)
{
	if (pos > _rx_length)
		return 0; // out of bounds
	else
		return _rx_buffer[pos];
}

bool CMRI::set_bit(int pos, bool bit)
{
	if ((pos + 7) / 8 > _tx_length)
		return false; // out of bounds
	else
	{
		int index = pos / 8;
		_tx_buffer[index] = bit
			? _tx_buffer[index] | 1 << pos % 8		// if bit=1, then OR it
			: _tx_buffer[index] & ~(1 << pos % 8) // if bit=0, then NAND it
		;
		return true;
	}
}

bool CMRI::set_byte(int pos, char b)
{
	if (pos > _tx_length)
		return false; // out of bounds
	else
	{
		_tx_buffer[pos] = b;
		return true;
	}
}


void CMRI::transmit()
{
	delay(50); // tiny delay to let things recover
	_serial.write(255);
	_serial.write(255);
	_serial.write(STX);
	_serial.write(65 + _address);
	_serial.write(GET);
	for (int i=0; i<_tx_length; i++)
	{
		if (_tx_buffer[i] == ETX)
			_serial.write(ESC); // escape because this looks like an STX bit (very basic protocol)
		if (_tx_buffer[i] == ESC)
			_serial.write(ESC); // escape because this looks like an escape bit (very basic protocol)
		_serial.write(_tx_buffer[i]);
	}
	_serial.write(ETX);
	_serial.flush();
}

// Private methods

bool CMRI::_decode(char c)
{
	// first mark bit
	if (_mode == MODE_INVALID0 && c == 0xFF)
	{
		_mode = MODE_INVALID1;
	}
	// second mark bit
	else if (_mode == MODE_INVALID1 && c == 0xFF)
	{
		_mode = MODE_INVALID2;
	}
	// start bit
	else if (_mode != MODE_VALID && c == STX)
	{
		_mode = MODE_VALID;
		_ignore_packet = false;
		_rx_index = 0;
		_rx_packet_type = -1; // ??
	}
	// body of packet
	else if (_mode == MODE_VALID)
	{
		// an escape code -- swallow byte
		if (c == 0x10 && _ignore_next_byte == false)
		{
			_ignore_next_byte = true;
		}
		// end of packet
		else if (c == ETX && _ignore_next_byte == false)
		{
			_mode = MODE_INVALID0;
			_rx_index = 0;
			_have_valid_packet = true;
			_ignore_packet = false;
			return true;
		}
		// packet too long
		else if (_rx_index >= _rx_length + 2)
		{
			_mode	= MODE_INVALID0;
			_rx_index = 0;
			_ignore_next_byte = false;
			_ignore_packet = false;
			return false;
		}
		// address
		else if (_rx_index == 0)
		{
			if (c != (65 + _address)) {
				_ignore_packet = true;
				_rx_packet_type = -1; // ??
				_have_valid_packet = false; // ??
			}
			_rx_index++;
		}
		// packet type
		else if (_rx_index == 1 && _ignore_packet == false)
		{
			_rx_packet_type = c;
			_rx_index++;
			// we are only interested in SET packets, anything else just note 
			// that we got a packet of that type and then return.
			if (_rx_packet_type != SET)
			{
	 			_mode = MODE_INVALID0;
	 			_rx_index = 0;
	 			_have_valid_packet = true;
				_ignore_packet = false;
	 			return true;
			}
		}
		// this packet is not meant for us
		else if (_ignore_packet == true)
		{
		}
		// add to buffer
		else
		{
			_rx_buffer[_rx_index - 2] = c;
			_rx_index++;
			_ignore_next_byte = false;
		}
	}
	_have_valid_packet = false;
	return false;
}
