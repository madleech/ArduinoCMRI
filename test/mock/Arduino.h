/*
    Minimal Arduino.h mock for native unit tests.

    Provides just enough of the Arduino runtime for CMRI to build and run
    off-device. The core of the library is a Stream-based protocol decoder, so
    the mock supplies a concrete Stream backed by an input queue the test fills
    and an output buffer the test inspects.
*/

#ifndef _CMRI_test_Arduino_h
#define _CMRI_test_Arduino_h

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h> // malloc/free used by the CMRI constructor
#include <deque>
#include <vector>

// Serial config token; the mock never interprets it.
#define SERIAL_8N2 0x0E

// A concrete stand-in for the Arduino Stream interface. rx holds bytes waiting
// to be read() by the library; tx collects everything the library write()s.
class Stream
{
  public:
	std::deque<uint8_t> rx;
	std::vector<uint8_t> tx;

	int available()
	{
		return (int)rx.size();
	}

	int read()
	{
		if (rx.empty())
			return -1;
		uint8_t c = rx.front();
		rx.pop_front();
		return c;
	}

	size_t write(uint8_t c)
	{
		tx.push_back(c);
		return 1;
	}

	void flush()
	{
	}

	// Test helper: enqueue a byte as if it had arrived over the wire.
	void feed(uint8_t c)
	{
		rx.push_back(c);
	}
};

inline void delayMicroseconds(unsigned int)
{
}

extern unsigned long _mock_millis;
inline unsigned long millis()
{
	return _mock_millis;
}
inline void mock_advance_millis(unsigned long ms)
{
	_mock_millis += ms;
}

// The default argument of the CMRI constructor references Serial; the test
// translation unit defines it.
extern Stream Serial;

#endif
