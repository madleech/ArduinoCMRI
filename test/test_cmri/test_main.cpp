/*
    Native unit tests for CMRI.

    These run off-device via PlatformIO's `native` platform. The Arduino Stream
    is mocked (see test/mock/Arduino.h) with a concrete class backed by an input
    queue and an output buffer, so we can feed the decoder raw C/MRI packets and
    inspect the frames the library transmits.

    Protocol reminder (all packets are wrapped FF FF STX <addr> <cmd> ... ETX):
      - the address byte is 'A' + node address
      - SET ('T') carries output data PC -> node
      - POLL ('P') asks the node to transmit its input state back
      - GET ('R') is the frame the node sends in reply to a POLL
    In CMRI terms, set_bit/set_byte stage the node's *input* state (sent on
    transmit); get_bit/get_byte read the *output* state received in a SET.
*/

#include <unity.h>

#include "Arduino.h"
#include "CMRI.h"

// Referenced by the CMRI constructor's default argument (unused by the tests,
// which always pass an explicit Stream, but needed to satisfy the symbol).
Stream Serial;

void setUp(void)
{
}

void tearDown(void)
{
}

// --- helpers ---------------------------------------------------------------

// Feed a full framed packet (preamble + STX + body + ETX) into the stream.
static void feed_packet(Stream &s, uint8_t addr, uint8_t cmd, const uint8_t *data, size_t len)
{
	s.feed(0xFF);
	s.feed(0xFF);
	s.feed(CMRI::STX);
	s.feed('A' + addr);
	s.feed(cmd);
	for (size_t i = 0; i < len; i++)
		s.feed(data[i]);
	s.feed(CMRI::ETX);
}

// --- bit/byte accessors ----------------------------------------------------

// A staged input bit shows up in the transmitted GET frame.
void test_set_bit_reflected_in_transmit(void)
{
	Stream s;
	CMRI cmri(0, 24, 48, s); // 3 input bytes, 6 output bytes

	TEST_ASSERT_TRUE(cmri.set_bit(0, true));
	TEST_ASSERT_TRUE(cmri.set_bit(9, true)); // byte 1, bit 1

	cmri.transmit();

	// Frame: FF FF STX 'A' GET <3 data bytes> ETX
	// byte 1 = 0x02 (STX), so it gets DLE-escaped to ESC 0x02
	TEST_ASSERT_EQUAL_UINT8(0x01, s.tx[5]);      // byte 0, bit 0
	TEST_ASSERT_EQUAL_UINT8(CMRI::ESC, s.tx[6]); // DLE escape for byte 1 (STX value)
	TEST_ASSERT_EQUAL_UINT8(0x02, s.tx[7]);      // byte 1, bit 1
	TEST_ASSERT_EQUAL_UINT8(0x00, s.tx[8]);      // byte 2
}

// Regression for the set_bit() bounds bug: the old (pos + 7) / 8 check wrongly
// rejected the high bits (17..23) of the last input byte on a default SMINI.
void test_set_bit_accepts_high_bits_of_last_byte(void)
{
	Stream s;
	CMRI cmri(0, 24, 48, s); // 3 input bytes -> valid bits 0..23

	for (int pos = 17; pos <= 23; pos++)
		TEST_ASSERT_TRUE(cmri.set_bit(pos, true));

	cmri.transmit();
	TEST_ASSERT_EQUAL_UINT8(0xFE, s.tx[7]); // bits 1..7 of byte 2 set
}

// Bits past the configured input width are rejected.
void test_set_bit_out_of_bounds(void)
{
	Stream s;
	CMRI cmri(0, 24, 48, s); // valid bits 0..23
	TEST_ASSERT_FALSE(cmri.set_bit(24, true));
	TEST_ASSERT_FALSE(cmri.set_bit(100, true));
}

// set_byte respects the input length; get_byte returns 0 past the output length.
void test_byte_bounds(void)
{
	Stream s;
	CMRI cmri(0, 24, 48, s); // 3 input bytes, 6 output bytes
	TEST_ASSERT_TRUE(cmri.set_byte(2, 0xAB));
	TEST_ASSERT_FALSE(cmri.set_byte(3, 0xAB));
	TEST_ASSERT_EQUAL_UINT8(0, cmri.get_byte(6)); // past output length
}

// --- protocol behaviour ----------------------------------------------------

// A POLL for our address makes process() transmit a well-formed GET frame.
void test_poll_produces_get_frame(void)
{
	Stream s;
	CMRI cmri(0, 24, 48, s);

	cmri.set_byte(0, 0x5A); // stage some input state

	feed_packet(s, 0, CMRI::POLL, nullptr, 0);
	TEST_ASSERT_TRUE(cmri.process());

	const std::vector<uint8_t> &tx = s.tx;
	TEST_ASSERT_EQUAL_UINT(9u, tx.size()); // 2 preamble + STX + addr + GET + 3 data + ETX
	TEST_ASSERT_EQUAL_UINT8(0xFF, tx[0]);
	TEST_ASSERT_EQUAL_UINT8(0xFF, tx[1]);
	TEST_ASSERT_EQUAL_UINT8(CMRI::STX, tx[2]);
	TEST_ASSERT_EQUAL_UINT8('A' + 0, tx[3]);
	TEST_ASSERT_EQUAL_UINT8(CMRI::GET, tx[4]);
	TEST_ASSERT_EQUAL_UINT8(0x5A, tx[5]);
	TEST_ASSERT_EQUAL_UINT8(CMRI::ETX, tx[8]);
}

// A SET updates the output buffer read back through get_bit/get_byte.
void test_set_packet_updates_outputs(void)
{
	Stream s;
	CMRI cmri(0, 24, 48, s);

	uint8_t data[6] = {0x01, 0x00, 0x80, 0, 0, 0};
	feed_packet(s, 0, CMRI::SET, data, 6);

	TEST_ASSERT_TRUE(cmri.process());
	TEST_ASSERT_TRUE(cmri.get_bit(0)); // byte 0, bit 0
	TEST_ASSERT_FALSE(cmri.get_bit(1));
	TEST_ASSERT_TRUE(cmri.get_bit(23)); // byte 2, bit 7
	TEST_ASSERT_EQUAL_UINT8(0x80, cmri.get_byte(2));
}

// Regression for the v1.7.0 INIT bug: an INIT ('I') frame must not disturb the
// output image set by a previous SET. Before the fix, INIT bodies were decoded
// into _rx_buffer, transiently driving the INIT header bytes onto the outputs.
static uint8_t g_init_len;
static uint8_t g_init_data[8];
static void capture_init(const uint8_t *data, int len)
{
	g_init_len = (uint8_t)len;
	for (int i = 0; i < len && i < (int)sizeof(g_init_data); i++)
		g_init_data[i] = data[i];
}

void test_init_does_not_corrupt_outputs(void)
{
	Stream s;
	CMRI cmri(0, 24, 48, s);
	cmri.set_init_handler(capture_init);

	// Establish a known output image.
	uint8_t set_data[6] = {0xAA, 0x55, 0x0F, 0xF0, 0x12, 0x34};
	feed_packet(s, 0, CMRI::SET, set_data, 6);
	TEST_ASSERT_TRUE(cmri.process());
	TEST_ASSERT_EQUAL_UINT8(0xAA, cmri.get_byte(0));

	// A typical JMRI INIT: NDP='M', DLH, DLL, NS. bit0 of 'M' (0x4D) is 1 — the
	// value that used to leak onto output pin 0.
	g_init_len = 0xEE;
	uint8_t init_data[4] = {'M', 0x00, 0x0A, 0x00};
	feed_packet(s, 0, CMRI::INIT, init_data, 4);

	// INIT must not report "outputs updated"...
	TEST_ASSERT_FALSE(cmri.process());
	// ...the handler must have fired with the raw payload...
	TEST_ASSERT_EQUAL_UINT8(4, g_init_len);
	TEST_ASSERT_EQUAL_UINT8('M', g_init_data[0]);
	TEST_ASSERT_EQUAL_UINT8(0x0A, g_init_data[2]);
	// ...and the output image must be exactly what the SET left behind.
	TEST_ASSERT_EQUAL_UINT8(0xAA, cmri.get_byte(0));
	TEST_ASSERT_EQUAL_UINT8(0x55, cmri.get_byte(1));
	TEST_ASSERT_EQUAL_UINT8(0x34, cmri.get_byte(5));
	TEST_ASSERT_FALSE(cmri.get_bit(0)); // bit0 of byte0 (0xAA) — not the leaked 'M' bit
}

// Without a registered handler, INIT is ignored entirely (pre-v1.7.0 behaviour):
// no buffer is allocated, outputs are untouched, and process() reports false.
void test_init_ignored_without_handler(void)
{
	Stream s;
	CMRI cmri(0, 24, 48, s);

	uint8_t set_data[6] = {0xAA, 0x55, 0x0F, 0xF0, 0x12, 0x34};
	feed_packet(s, 0, CMRI::SET, set_data, 6);
	TEST_ASSERT_TRUE(cmri.process());

	uint8_t init_data[4] = {'M', 0x00, 0x0A, 0x00};
	feed_packet(s, 0, CMRI::INIT, init_data, 4);
	TEST_ASSERT_FALSE(cmri.process());
	TEST_ASSERT_EQUAL_UINT8(0xAA, cmri.get_byte(0));
}

// A packet addressed to another node is ignored: process() is false and no
// outputs change.
void test_address_filtering(void)
{
	Stream s;
	CMRI cmri(0, 24, 48, s); // we are node 0

	uint8_t data[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	feed_packet(s, 1, CMRI::SET, data, 6); // addressed to node 1

	TEST_ASSERT_FALSE(cmri.process());
	TEST_ASSERT_FALSE(cmri.get_bit(0));
	TEST_ASSERT_EQUAL_UINT8(0, cmri.get_byte(0));
}

// Data bytes that collide with STX, ETX or ESC are escaped in the transmitted frame.
void test_transmit_escapes_control_bytes(void)
{
	Stream s;
	CMRI cmri(0, 24, 48, s);

	cmri.set_byte(0, CMRI::ETX); // looks like end-of-frame
	cmri.set_byte(1, CMRI::ESC); // looks like an escape

	cmri.transmit();

	// header (5) then escaped payload then ETX
	TEST_ASSERT_EQUAL_UINT8(CMRI::ESC, s.tx[5]);
	TEST_ASSERT_EQUAL_UINT8(CMRI::ETX, s.tx[6]);
	TEST_ASSERT_EQUAL_UINT8(CMRI::ESC, s.tx[7]);
	TEST_ASSERT_EQUAL_UINT8(CMRI::ESC, s.tx[8]);
	TEST_ASSERT_EQUAL_UINT8(0x00, s.tx[9]);
	TEST_ASSERT_EQUAL_UINT8(CMRI::ETX, s.tx[10]);
}

// Garbage before a valid packet is resynced away by the preamble state machine.
void test_preamble_resync_after_garbage(void)
{
	Stream s;
	CMRI cmri(0, 24, 48, s);

	// Junk, including a lone 0xFF that must not be mistaken for the preamble.
	s.feed(0x00);
	s.feed(0xFF);
	s.feed(0x13);
	s.feed(0x41);

	feed_packet(s, 0, CMRI::POLL, nullptr, 0);
	TEST_ASSERT_TRUE(cmri.process());
	TEST_ASSERT_EQUAL_UINT(9u, s.tx.size());
	TEST_ASSERT_EQUAL_UINT8(CMRI::GET, s.tx[4]);
}

int main(int, char **)
{
	UNITY_BEGIN();
	RUN_TEST(test_set_bit_reflected_in_transmit);
	RUN_TEST(test_set_bit_accepts_high_bits_of_last_byte);
	RUN_TEST(test_set_bit_out_of_bounds);
	RUN_TEST(test_byte_bounds);
	RUN_TEST(test_poll_produces_get_frame);
	RUN_TEST(test_set_packet_updates_outputs);
	RUN_TEST(test_init_does_not_corrupt_outputs);
	RUN_TEST(test_init_ignored_without_handler);
	RUN_TEST(test_address_filtering);
	RUN_TEST(test_transmit_escapes_control_bytes);
	RUN_TEST(test_preamble_resync_after_garbage);
	return UNITY_END();
}
