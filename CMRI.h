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

#ifndef CMRI_h
#define CMRI_h

#define _CMRI_VERSION 1 // software version of this library

class CMRI
{
	public:
		CMRI(unsigned int address = 0, unsigned int input_bits = 24, unsigned int output_bits = 48);
		void set_address(unsigned int address);
		void set_length(unsigned int input_bits, unsigned int output_bits);
		
		char process();
		void transmit();
		
		bool get_bit(int n);
		char get_byte(int n);
		
		bool set_bit(int n, bool b);
		bool set_byte(int n, char b);
		
		enum {
			MAX	= 258,	// max packet length in bytes (64 i/o cards @ 32 bits each + packet type and address bytes)
			INIT = 'I',	// PC is telling us stuff we don't really care about
			SET	= 'T',	// as in TX from the PC => Arduino, PC is SETing our status
			GET	= 'R',	// as in TX from Arduino => PC, PC is GETing our status
			POLL = 'P',	// PC wants to know our status
			STX	= 0x02, // start byte
			ETX	= 0x03, // end byte
		};

private:
		enum {MODE_INVALID0, MODE_INVALID1, MODE_INVALID2, MODE_VALID};
		
		int	  _address;
		int	  _rx_length;
		int	  _tx_length;
		char	_rx_packet_type;
		char* _tx_buffer;
		char* _rx_buffer;
		
		// parsing state variables
		int	  _mode;
		bool	_ignore_next_byte;
		bool	_ignore_packet;
		int	  _rx_index;
		bool	_have_valid_packet;
		
		bool  _decode(char c); // process one character received from serial port
};

#endif
