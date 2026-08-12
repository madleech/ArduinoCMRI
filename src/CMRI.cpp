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
    : _address(address), _rx_length((output_bits + 7) / 8), _tx_length((input_bits + 7) / 8)

      // set state
      ,
      _rx_buffer((char *)malloc(_rx_length)), _tx_buffer((char *)malloc(_tx_length))

      ,
      _serial(serial_class)

      // parsing state
      ,
      _mode(PREAMBLE_1), _rx_index(0), _rx_data_len(0), _init_handler(nullptr), _init_buffer(nullptr), _init_length(0)

{
	// clear to zero
	for (int i = 0; i < _rx_length; i++)
		_rx_buffer[i] = 0;
	for (int i = 0; i < _tx_length; i++)
		_tx_buffer[i] = 0;
}

void CMRI::set_address(unsigned int address)
{
	_address = address;
}

void CMRI::set_init_handler(void (*handler)(const uint8_t *, int))
{
	_init_handler = handler;

	// Lazily allocate a dedicated buffer for INIT payloads the first time a
	// handler is registered, so INIT bodies are decoded away from the output
	// image in _rx_buffer. Sized to the output length (with a small floor to
	// still capture the NDP + delay header on input-only nodes), which covers
	// standard C/MRI INIT bodies; anything longer is safely truncated.
	if (handler && _init_buffer == nullptr)
	{
		_init_length = _rx_length > 4 ? _rx_length : 4;
		_init_buffer = (char *)malloc(_init_length);
		if (_init_buffer == nullptr)
			_init_length = 0; // allocation failed: discard INIT bodies rather than risk a null write
	}
}

// reads in serial data, decodes packets
// automatically responds to POLL requests
// returns packet type so if we got a SET request you know to update your outputs
bool CMRI::process()
{
	while (_serial.available() > 0)
	{
		if (process_char(_serial.read()))
		{
			return true;
		}
	}
	return false;
}

bool CMRI::process_char(char c)
{
	// if it's a SET, update the output buffer
	// if it's an INIT, the payload was consumed and the sketch can handle it
	// if it's a GET, ignore — must be another node replying
	// if it's a POLL, reply straight away with our input data
	unsigned char ret = _decode(c);
	switch (ret)
	{
	case POLL:
		transmit();
		return true;

	case SET:
		// a SET updated the output image; tell the caller to refresh outputs
		return true;

	case INIT:
		// an INIT never touches the output image, so it must not signal
		// "outputs updated"; deliver the payload via the handler instead
		if (_init_handler)
			_init_handler((const uint8_t *)_init_buffer, _rx_data_len);
		return false;

	default:
		return false;
	}
}

// public methods

bool CMRI::get_bit(int pos)
{
	// 1: divide index by 8 to get byte offset
	char c = get_byte(pos / 8);
	// 2: return bit at that location
	return (bool)((c >> (pos % 8)) & 0x01);
}

char CMRI::get_byte(int pos)
{
	if (pos >= _rx_length)
		return 0; // out of bounds
	else
		return _rx_buffer[pos];
}

bool CMRI::set_bit(int pos, bool bit)
{
	if (pos / 8 >= _tx_length)
		return false; // out of bounds
	else
	{
		int index = pos / 8;
		_tx_buffer[index] = bit
		                        ? _tx_buffer[index] | 1 << pos % 8    // if bit=1, then OR it
		                        : _tx_buffer[index] & ~(1 << pos % 8) // if bit=0, then NAND it
		    ;
		return true;
	}
}

bool CMRI::set_byte(int pos, char b)
{
	if (pos >= _tx_length)
		return false; // out of bounds
	else
	{
		_tx_buffer[pos] = b;
		return true;
	}
}

void CMRI::transmit()
{
	delayMicroseconds(50); // a minscule delay to let things recover
	_serial.write(255);
	_serial.write(255);
	_serial.write(STX);
	_serial.write(65 + _address);
	_serial.write(GET);
	for (int i = 0; i < _tx_length; i++)
	{
		// DLE-escape STX, ETX and DLE characters
		if (_tx_buffer[i] == STX || _tx_buffer[i] == ETX || _tx_buffer[i] == ESC)
			_serial.write(ESC);
		_serial.write(_tx_buffer[i]);
	}
	_serial.write(ETX);
	_serial.flush();
}

// Private methods

// Append one decoded body byte to the buffer belonging to the packet currently
// being parsed: SET data lands in the output image (_rx_buffer), while INIT data
// is kept separate in _init_buffer so it can never corrupt the layout outputs.
// Bytes past the target buffer's length (or with no buffer allocated) are dropped.
void CMRI::_store_data_byte(uint8_t c)
{
	char *buffer = (_rx_packet_type == INIT) ? _init_buffer : _rx_buffer;
	int length = (_rx_packet_type == INIT) ? _init_length : _rx_length;

	if (buffer != nullptr && _rx_index < length)
		buffer[_rx_index++] = c;
}

uint8_t CMRI::_decode(uint8_t c)
{
	switch (_mode)
	{
	case PREAMBLE_1:
		_rx_index = 0;
		if (c == 0xFF)
			_mode = PREAMBLE_2;
		break;

	case PREAMBLE_2:
		if (c == 0xFF)
			_mode = PREAMBLE_3;
		else
			_mode = PREAMBLE_1;
		break;

	case PREAMBLE_3:
		if (c == STX)
			_mode = DECODE_ADDR;
		else
			_mode = PREAMBLE_1;
		break;

	case DECODE_ADDR:
		if (c == 'A' + _address)
			_mode = DECODE_CMD;
		else if (c >= 'A')
			_mode = IGNORE_CMD;
		else
			_mode = PREAMBLE_1;
		break;

	case DECODE_CMD:
		_rx_packet_type = c;
		if (c == SET)
			_mode = DECODE_DATA;
		else if (c == INIT)
			_mode = DECODE_DATA;
		else if (c == POLL)
			goto POSTAMBLE_POLL;
		else
			_mode = POSTAMBLE_OTHER;
		break;

	case IGNORE_CMD:
		_mode = IGNORE_DATA;
		break;

	case DECODE_DATA:
		if (c == ESC)
			_mode = DECODE_ESC_DATA;
		else if (c == ETX)
			goto POSTAMBLE_SET;
		else
			_store_data_byte(c);
		break;

	case DECODE_ESC_DATA:
		_store_data_byte(c);
		_mode = DECODE_DATA;
		break;

	case IGNORE_DATA:
		if (c == ESC)
			_mode = IGNORE_ESC_DATA;
		else if (c == ETX)
			goto POSTAMBLE_IGNORE;
		break;

	case IGNORE_ESC_DATA:
		_mode = IGNORE_DATA;
		break;

	case POSTAMBLE_OTHER:
		_mode = PREAMBLE_1;
		break;
	}
	return NOOP;

POSTAMBLE_SET:
	_rx_data_len = _rx_index;
	_mode = PREAMBLE_1;
	_rx_index = 0;
	return _rx_packet_type;

POSTAMBLE_POLL:
	_mode = PREAMBLE_1;
	return POLL;

POSTAMBLE_IGNORE:
	_mode = PREAMBLE_1;
	return NOOP;
}
