#include <stdint.h>
#include "main.h"

#ifdef DEBUGMODE
void _wiggle(uchar times) {
	uint8_t i;
	for ( i = 0; i < times ; i++)
	{
		wiggleHigh();
		DELAY_1_CLK();
		wiggleLow();
		DELAY_1_CLK();
	}
}

void _displayValue(uint32_t toDisplay) {
	uint8_t counter;

	for (counter = 0; counter < 32; counter++)
	{
		if ((toDisplay >> (31 - counter)) & 0x01)
		{
			wiggleHigh();
			TXDhigh();
			DELAY_1_CLK();
			TXDlow();
			wiggleLow();
		} else {
			TXDhigh();
			DELAY_1_CLK();
			TXDlow();
		}
	}
}

	#define wiggle(...)       _wiggle(__VA_ARGS__)
	#define displayValue(...) _displayValue(__VA_ARGS__)
#else
	#define wiggle(...)
	#define displayValue(...)
#endif

void sendToKeyboard(uint8_t toSend) {
	uint8_t i;
	cli();
	TXDhigh();
	wiggle(1);
	DELAY_FULL_KB_CLK();
	// the keyboard wants its data as inverted logic and lsb first
	// don't ask me why they chose this format
	for(i = 1; i <= (1 << 7); i <<= 1)
	{
		if (toSend  & i)
		{
			TXDlow();
			wiggle(1);
		} else {
			TXDhigh();
			wiggle(1);
		}
		DELAY_FULL_KB_CLK();
	}

	TXDhigh();
	wiggle(3);
	DELAY_FULL_KB_CLK();
	TXDlow();
	DELAY_FULL_KB_CLK();
	DELAY_FULL_KB_CLK();

	TXDlow();
	DELAY_FULL_KB_CLK();


	sei();
}

#ifdef DEBUGMODE
#endif
