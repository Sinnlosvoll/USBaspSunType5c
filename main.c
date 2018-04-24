/**
 * Project: Suntype5c USBasp 
 * Author: Christopher Hofmann, christopherushofmann@googlemail.com
 * Base on V-USB example code by Christian Starkjohann
 * Copyright: (c) 2008 by OBJECTIVE DEVELOPMENT Software GmbH
 * License: GNU GPL v3 (see License.txt)
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <avr/eeprom.h>
#include <util/delay.h>

#include "usbdrv.h"
#include "clock.h"
#include "helperFunctions.h"


// ************************
// *** USB HID ROUTINES ***
// ************************

// From Frank Zhao's USB Business Card project
// http://www.frank-zhao.com/cache/usbbusinesscard_details.php
PROGMEM const uint8_t usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
	0x05, 0x01,						  // USAGE_PAGE (Generic Desktop)
	0x09, 0x06,						  // USAGE (Keyboard)
	0xa1, 0x01,						  // COLLECTION (Application)
	0x75, 0x01,						  //	REPORT_SIZE (1)
	0x95, 0x08,						  //	REPORT_COUNT (8)
	0x05, 0x07,						  //	USAGE_PAGE (Keyboard)(Key Codes)
	0x19, 0xe0,						  //	USAGE_MINIMUM (Keyboard LeftControl)(224)
	0x29, 0xe7,						  //	USAGE_MAXIMUM (Keyboard Right GUI)(231)
	0x15, 0x00,						  //	LOGICAL_MINIMUM (0)
	0x25, 0x01,						  //	LOGICAL_MAXIMUM (1)
	0x81, 0x02,						  //	INPUT (Data,Var,Abs) ; Modifier byte
	0x95, 0x01,						  //	REPORT_COUNT (1)
	0x75, 0x08,						  //	REPORT_SIZE (8)
	0x81, 0x03,						  //	INPUT (Cnst,Var,Abs) ; Reserved byte
	0x95, 0x05,						  //	REPORT_COUNT (5)
	0x75, 0x01,						  //	REPORT_SIZE (1)
	0x05, 0x08,						  //	USAGE_PAGE (LEDs)
	0x19, 0x01,						  //	USAGE_MINIMUM (Num Lock)
	0x29, 0x05,						  //	USAGE_MAXIMUM (Kana)
	0x91, 0x02,						  //	OUTPUT (Data,Var,Abs) ; LED report
	0x95, 0x01,						  //	REPORT_COUNT (1)
	0x75, 0x03,						  //	REPORT_SIZE (3)
	0x91, 0x03,						  //	OUTPUT (Cnst,Var,Abs) ; LED report padding
	0x95, 0x06,						  //	REPORT_COUNT (6)
	0x75, 0x08,						  //	REPORT_SIZE (8)
	0x15, 0x00,						  //	LOGICAL_MINIMUM (0)
	0x25, 0xFF,						  //	LOGICAL_MAXIMUM (101)
	0x05, 0x07,						  //	USAGE_PAGE (Keyboard)(Key Codes)
	0x19, 0x00,						  //	USAGE_MINIMUM (Reserved (no event indicated))(0)
	0x29, 0x91,						  //	USAGE_MAXIMUM (Keyboard Application)(101)
	0x81, 0x00,						  //	INPUT (Data,Ary,Abs)
	0xc0									// END_COLLECTION
};

typedef struct {
	uint8_t modifier;
	uint8_t reserved;
	uint8_t keycode[6];
} keyboard_report_t;

static keyboard_report_t keyboard_report; // sent to PC
static volatile uint8_t LED_state = 0xff; // received from PC
static uint8_t idleRate; // repeat rate for keyboardseport; // sent to PC
// static keyboard_report_t keyboard_report; // sent to PC
// volatile static uint8_t LED_state = 0xff; // received from PC
// static uint8_t idleRate; // repeat rate for keyboards
extern uint32_t readFromKeyboard = 0;
uint8_t newResponse = 0;
uint8_t keys_pressed = 0;
uint8_t keysHaveChanged = 0;
uint16_t emergencyResponseCounter = 0;
uint8_t leds = 0;
uint8_t keyBoardHasReported = 0;


usbMsgLen_t usbFunctionSetup(uint8_t data[8]) {
	usbRequest_t *rq = (void *)data;

	if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS) {
		switch(rq->bRequest) {
		case USBRQ_HID_GET_REPORT: // send "no keys pressed" if asked here
			// wValue: ReportType (highbyte), ReportID (lowbyte)
			usbMsgPtr = (void *)&keyboard_report; // we only have this one
			keyboard_report.modifier = 0;
			keyboard_report.keycode[0] = 0;
			return sizeof(keyboard_report);
		case USBRQ_HID_SET_REPORT: // if wLength == 1, should be LED state
			return (rq->wLength.word == 1) ? USB_NO_MSG : 0;
		case USBRQ_HID_GET_IDLE: // send idle rate to PC as required by spec
			usbMsgPtr = &idleRate;
			return 1;
		case USBRQ_HID_SET_IDLE: // save idle rate as required by spec
			idleRate = rq->wValue.bytes[1];
			return 0;
		}
	}
	
	return 0; // by default don't return any data
}

#define NUM_LOCK	 1
#define CAPS_LOCK	2
#define SCROLL_LOCK 4
#define COMPOSE	  0x08








void sendToKeyboard(uint8_t toSend, uint8_t isLastOne) {
	uint8_t i;
	cli();
	tx_hi();
	// wiggle(1);
	DELAY_FULL_KB_CLK();
	// the keyboard wants its data as inverted logic and lsb first
	// don't ask me why they chose this format
	for (i = 0; i < 7; i++)
	{
		if (toSend  & (0x01 << i))
		{
			tx_low();
			// wiggle(1);
		} else {
			tx_hi();
			// wiggle(1);
		}
		DELAY_FULL_KB_CLK();
	}
	if (isLastOne)
	{
		tx_hi();
		// wiggle(3);
		DELAY_FULL_KB_CLK();
		tx_low();
		DELAY_FULL_KB_CLK();
		DELAY_FULL_KB_CLK();
	} else {
		tx_low();
		// wiggle(4);
		DELAY_FULL_KB_CLK();
		DELAY_FULL_KB_CLK();

	}
	tx_low();
	DELAY_FULL_KB_CLK();

	
	sei();
}


usbMsgLen_t usbFunctionWrite(uint8_t * data, uint8_t len) {
	if (data[0] == LED_state)
		return 1;
	else
		LED_state = data[0];
	
	// LED state changed
	if(LED_state & CAPS_LOCK){
		leds |= (1 << 3);
		//sendToKeyboard((uint8_t)0x02, 1);
	}
	else {
		leds &= ~(1 << 3);
		//sendToKeyboard((uint8_t)0x03, 1);
	}
	if(LED_state & NUM_LOCK){
		leds |= (1 << 0);
	}
	else {
		leds &= ~(1 << 0);
	}
	if(LED_state & SCROLL_LOCK){
		ledGreenOn(); // LED on
		leds |= (1 << 2);
	}
	else {
		ledGreenOff(); // LED off
		leds &= ~(1 << 2);
	}
	if(LED_state & COMPOSE){
		leds |= (1 << 1);
	}
	else {
		leds &= ~(1 << 1);
	}
	updateLeds();
	
	return 1; // Data read, not expecting more
}


#define STATE_WAIT 0
#define STATE_SEND_KEY 1
#define STATE_RELEASE_KEY 2

void key_down(uint8_t down_key) {
	/* we only add a key to our pressed list, when we have less than 6 already pressed.
	** additional keys will be ignored  */
	ledGreenOn();
	_delay_us(300);
	ledGreenOff();
	if (keys_pressed < 6)
	{
		keyboard_report.keycode[keys_pressed] = down_key;
		keys_pressed++;
	}
	keysHaveChanged = 1;
}

void key_up(uint8_t up_key) {
	uint8_t n,i;
	for (i = 0; i < 6; i++)
	{
		if (keyboard_report.keycode[i] == up_key)
		{
			for (n = i; n <= 4; n++ )
			{
				keyboard_report.keycode[n] = keyboard_report.keycode[n + 1];
			}
			keyboard_report.keycode[5] = 0;
			keys_pressed--;
			break;
		}
	}
	keysHaveChanged = 1; 
}

void toggleModifier(uint8_t key) {
	keyboard_report.modifier ^= key;
	keysHaveChanged = 1;
}

// clear internal record of keys pressed
void resetKeysDown() {
	uint8_t i;
	for (	i = 0; i < 6; i++)
	{
		keyboard_report.keycode[i] = 0;
	}
}


//  the mapping function which turns the keycode from the keyboard into the usb keycode

uint8_t map(uint8_t keyCodeIn) {
	if (keyBoardHasReported == 0)
	{
		keyBoardHasReported = 1;
		return 0xFF;
	}
	switch ( keyCodeIn ) {
		case 0x05 : return 0x3a; // F1
		case 0x06 : return 0x3b; // F2
		case 0x07 : return 0x43; // F10
		case 0x08 : return 0x3c; // F3
		case 0x09 : return 0x44; // F11
		case 0x0a : return 0x3d; // F4
		case 0x0b : return 0x45; // F12
		case 0x0c : return 0x3e; // F5
		case 0x0d : toggleModifier((1 << 6)); return 0xFF; // alt right (graph)
		case 0x0e : return 0x3f; // F6
		case 0x10 : return 0x40; // F7
		case 0x11 : return 0x41; // F8
		case 0x12 : return 0x42; // F9
		case 0x13 : toggleModifier((1 << 2)); return 0xFF; // alt left
		case 0x15 : return 0x48; // pause
		case 0x16 : return 0x46; // printscrn
		case 0x17 : return 0x47; // scroll lock
		case 0x1d : return 0x29; // esc
		case 0x1e : return 0x1e; // 1
		case 0x1f : return 0x1f; // 2 
		case 0x20 : return 0x20; // 3
		case 0x21 : return 0x21; // 4
		case 0x22 : return 0x22; // 5
		case 0x23 : return 0x23; // 6
		case 0x24 : return 0x24; // 7
		case 0x25 : return 0x25; // 8
		case 0x26 : return 0x26; // 9
		case 0x27 : return 0x27; // 0
		case 0x28 : return 0x2D; // -
		case 0x29 : return 0x2E; // =
		case 0x2b : return 0x2a; // backspace
		case 0x2e : return 0x54; // keypad /
		case 0x2f : return 0x55; // keypad *
		case 0x32 : return 0x4c; // delete
		case 0x35 : return 0x2b; // tabulator
		case 0x36 : return 0x14; // q
		case 0x37 : return 0x1a; // w
		case 0x38 : return 0x08; // e
		case 0x39 : return 0x15; // r
		case 0x3a : return 0x17; // t
		case 0x3b : return 0x1C; // y (z-key)
		case 0x3c : return 0x0c; // u
		case 0x3d : return 0x18; // i
		case 0x3e : return 0x12; // o
		case 0x3f : return 0x13; // p
		case 0x40 : return 0x2f; // [ (ü-key)
		case 0x41 : return 0x32; // ] (pound key)
		case 0x43 : toggleModifier((1 << 4)); return 0xFF; //compose mapped to r_ctrl
		//case 0x44 : return 0x5e; // keypad 6
		case 0x45 : return 0x52; // uparrow
		case 0x46 : return 0x4b; // PgUp 
		case 0x47 : return 0x56; // keypad -
		case 0x4a : return 0x7f; // mute
		case 0x4c : toggleModifier((1 << 0)); return 0xFF; // delete
		case 0x4d : return 0x04; // a
		case 0x4e : return 0x16; // s
		case 0x4f : return 0x07; // d
		case 0x50 : return 0x09; // f
		case 0x51 : return 0x0A; // g
		case 0x52 : return 0x0b; // h
		case 0x53 : return 0x0D; // j
		case 0x54 : return 0x0e; // k
		case 0x55 : return 0x0f; // l
		case 0x56 : return 0x33; // ; ö
		case 0x57 : return 0x34; // ' ä
		case 0x58 : return 0x30; // \|
		case 0x44 : return 0x4a; // home
		case 0x59 : return 0x58; // keypad enter
		case 0x5a : return 0x28; // enter
		case 0x5b : return 0x50; // leftArrow
		case 0x5d : return 0x4f; // rightArrow
		case 0x5e : return 0x49; // insert
		case 0x62 : return 0x53; // num lock
		case 0x63 : toggleModifier((1 << 5)); return 0xFF; // right shift
		case 0x64 : return 0x1d; // z
		case 0x65 : return 0x1b; // x
		case 0x66 : return 0x06; // c
		case 0x67 : return 0x19; // v
		case 0x68 : return 0x05; // b
		case 0x69 : return 0x11; // n
		case 0x6a : return 0x10; // m
		case 0x6b : return 0x36; // ,
		case 0x6c : return 0x37; // .
		case 0x6d : return 0x38; // /
		case 0x6e : toggleModifier((1 << 1)); return 0xFF; // shift left
		case 0x70 : return 0x4d; // end
		case 0x71 : return 0x51; // downarrow
		case 0x72 : return 0x4e; // PgDn
		case 0x77 : return 0x39; // capslock
		case 0x78 : toggleModifier((1 << 3)); return 0xFF;// meta left
		case 0x79 : return 0x2c; // spacebar
		case 0x7a : toggleModifier((1 << 7)); return 0xFF; // meta right
		case 0x7d : return 0x57; // keypad +
		case 0x90 : resetKeysDown(); return 0xFF; // reset keys on help
		case 0xaa : return 0x32; // `
		case 0xbE : return 0x81; // vol down
		case 0xc0 : return 0x64; // <>|(german)
		case 0xcc : toggleModifier((1 << 0)); return 0xFF; // ctrl left
		case 0xdE : return 0x80; // vol up 
		case 0xf2 : return 0x66; // power


		// keypads are not in the docs
		// testing is needed
		// case 0xb2 : return 0x63; // keypad ,
		// case 0x84 : return 0x62; // keypad 0
		// case 0xF0 : return 0x59; // keypad 1
		// case 0x70 : return 0x5a; // keypad 2 
		// case 0xb0 : return 0x5b; // keypad 3
		// case 0x24 : return 0x5c; // keypad 4
		// case 0xc4 : return 0x5d; // keypad 5
		// case 0xdc : return 0x5f; // keypad 7
		// case 0x5c : return 0x60; // keypad 8
		// case 0x9c : return 0x61; // keypad 9
		default : return 0xFF; // return error code

		// what is 0x2D (=) mapped to when 0x29 is already =+
		// shifts might be switched
	}
}

void parseKeyboardResponse() {
	uint8_t usbHIDcode = 0xFF;

	displayValue8((uint8_t)readFromKeyboard);

	if ((readFromKeyboard & 0x80) != 0) {
		// break code 
		usbHIDcode = map((uint8_t)(readFromKeyboard & ~(0x80) ));

		if (usbHIDcode == 0xFF)
		{
			wiggle(12);
		}
		else
		{
			key_up(usbHIDcode);
		}

	} else {
		// make code
		usbHIDcode = map((uint8_t)(readFromKeyboard & 0xFF));
		if (usbHIDcode == 0xFF)
		{
			wiggle(12);
		}
		else
		{
			key_down(usbHIDcode);
		}

	}
	
	// wiggle(2);

	// displayValue32(readFromKeyboard);
	// displayValue8(usbHIDcode);

	// wiggle(2);
	readFromKeyboard = 0;

}


void emergencyParse() {
	// for now lets just assume, that this only happens when multiple keys are pressed and one of them comes back up
	// for an unknown reason there is no break command for that key, only for the last one of that sequence that comes back up
	key_up(map((uint8_t)readFromKeyboard));
	wiggle(15);
	readFromKeyboard = 0;
	// notification for "debugger"
	// wiggle(40);
	// displayValue32(readFromKeyboard);
}




// this function reads the keycodes sent by the keyboard by waiting half a clock 
// cycle and then reading 8 bits by waiting a clock after each one
// keyboard clock cycles that is, so 833µs due to the 1200 baud the keyboard uses

void startReading() {
	uint8_t i;
	wiggle(10);
	DELAY_HALF_KB_CLK();


// probably not needed due to single byte transfer restrictions on keyboard side

	// if (newResponse)
	// {
	//	  // #weCantEven
	// } else {
	//	  // srsly
	//	  // readFromKeyboard = readFromKeyboard << 1;
	// }
	readFromKeyboard = 0;
	// data is read by getting the lsb first inverted. so read, invert, add to top and push right
	for (i = 0; i < 8; i++)
	{
		DELAY_FULL_KB_CLK();
		// read one bit, push right, invert, push left
		readFromKeyboard = readFromKeyboard >> 1;
		if (!(PINB & (1 << PB4)))
		{
			readFromKeyboard |= 0x80;
			wiggle(2);
		} else {
			wiggle(1);
		}
		// indicate measurement
		// wiggle(1);
	}

	parseKeyboardResponse();
	wiggle(2);

	DELAY_HALF_KB_CLK();
	// pb5low();
	// the additional wait time is for not detecting another byte right at the end of this one
}


int main() {
	uint8_t i, j;

	/* no pullups on USB and ISP pins */
	PORTD = 0;
	PORTB = 0;
	/* all outputs except PD2 = INT0 */
	DDRD = ~(1 << 2);

	/* output SE0 for USB reset */
	DDRB = ~0;
	j = 0;
	/* USB Reset by device only required on Watchdog Reset */
	while (--j) {
		i = 0;
		/* delay >10ms for USB reset */
		while (--i)
			;
	}
	/* all USB and ISP pins inputs */
	/* PB3 and PB5 are outputs */
	DDRB = 0;
	DDRB |= (1 << PB5); 
	DDRB |= (1 << PB3);

	/* all inputs except PC0, PC1, PC2*/
	DDRC = 0x07;
	PORTC = 0;

	/* init timer */
	clockInit();

	/* main event loop */
	usbInit();
	// allow interrupts
	sei();

	// signal stuff for debug
	ledRedOff();
	tx_low();
	
	bellOn();

	for (;;) {
		//  check for new usb events
		usbPoll();

		// if start bit is recieved
		if (PINB & (1 << PB4))
		{
			// ledRedOn();
			// pb5high();
			ledRedOn();
			// emergencyResponseCounter = 0;
			startReading();

		} else {
			// pb5low();

			// if (emergencyResponseCounter == 1)
			// {
			//	  emergencyParse();
			//	  emergencyResponseCounter = 0;
			// } else {
			//	  if (emergencyResponseCounter > 0) 
			//			emergencyResponseCounter--;
			// }

			ledRedOff();
			//ledGreenOff();
		}
		// guess: send new pressed keys to os if they changed
		if (usbInterruptIsReady() && keysHaveChanged)
		{
			usbSetInterrupt((void *)&keyboard_report, sizeof(keyboard_report));
			keysHaveChanged = 0;
			// wiggle(6);
		}
	}
	return 0;
}
