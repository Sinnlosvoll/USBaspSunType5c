uint8_t soundIsOn = 0;



void wiggle(uint8_t times) {
	uint8_t i;
	for ( i = 0; i < times ; i++)
	{
		debugtx_hi();
		DELAY_1_CLK;
		debugtx_low();
		DELAY_1_CLK;
	}
}

void toggleSound() {
	if (soundIsOn == 1)
	{
		clickOff();
	} else {
		clickOn();
	}
}


/*
* uses an additional pin on the usbasp to serially write out a value
*/
void displayValue32(uint32_t toDisplay) {
	uint8_t counter;
	uint32_t loopVariable = toDisplay;

	for (counter = 0; counter < 32; counter++)
	{
		debugtx_low();
		debugtx_hi();
		debugtx_low();
		if (loopVariable & 0x01)
		{
			debugtx_hi();
			DELAY_10_CLK;

		} else {
			debugtx_low();
			DELAY_10_CLK;
		}
		loopVariable = loopVariable >> 1;
	}
	debugtx_low();
}

void displayValue8(uint8_t toDisplay) {
	uint8_t counter;
	uint8_t loopVariable = toDisplay;

	// wiggle(2);

	for (counter = 0; counter < 8; counter++)
	{
		debugtx_low();
		debugtx_hi();
		debugtx_low();
		if (loopVariable & 0x01)
		{
			debugtx_hi();
			DELAY_10_CLK;

		} else {
			debugtx_low();
			DELAY_10_CLK;
		}
		loopVariable = loopVariable >> 1;
	}
	debugtx_low();
	// wiggle(2);
}