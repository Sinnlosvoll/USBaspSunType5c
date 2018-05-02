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

#include "clock.h"
#include "usbdrv.h"
#include "main.h"
#include "keycodes.h"
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
uint32_t readFromKeyboard = 0;
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
	if (keyBoardHasReported < 2)
	{
		keyBoardHasReported++;
		return 0xFF;
	}
	switch ( keyCodeIn ) {
		case 0x02 : return KEY_VOLUMEDOWN;
		case 0x04 : return KEY_VOLUMEUP;
		case 0x05 : return KEY_F1;
		case 0x06 : return KEY_F2;
		case 0x07 : return KEY_F10;
		case 0x08 : return KEY_F3;
		case 0x09 : return KEY_F11;
		case 0x0a : return KEY_F4;
		case 0x0b : return KEY_F12;
		case 0x0c : return KEY_F5;
		case 0x0d : toggleModifier((1 << 6)); return 0xFF; // alt right (graph)
		case 0x0e : return KEY_F6;
		case 0x10 : return KEY_F7;
		case 0x11 : return KEY_F8;
		case 0x12 : return KEY_F9;
		case 0x13 : toggleModifier((1 << 2)); return 0xFF; // alt left
		case 0x14 : return KEY_UP;
		case 0x15 : return KEY_PAUSE;
		case 0x16 : return KEY_SYSRQ;
		case 0x17 : return KEY_SCROLLLOCK;
		case 0x18 : return KEY_LEFT; 
		case 0x1b : return KEY_DOWN; 
		case 0x1c : return KEY_RIGHT; 
		case 0x1d : return KEY_ESC;
		case 0x1e : return KEY_1;
		case 0x1f : return KEY_2;
		case 0x20 : return KEY_3;
		case 0x21 : return KEY_4;
		case 0x22 : return KEY_5;
		case 0x23 : return KEY_6;
		case 0x24 : return KEY_7;
		case 0x25 : return KEY_8;
		case 0x26 : return KEY_9;
		case 0x27 : return KEY_0;
		case 0x28 : return KEY_MINUS;
		case 0x29 : return KEY_EQUAL;
		case 0x2a : return KEY_GRAVE;
		case 0x2b : return KEY_BACKSPACE;
		case 0x2c : return KEY_INSERT;
		case 0x2d : return KEY_MUTE;
		case 0x2e : return KEY_KPSLASH;
		case 0x2f : return KEY_KPASTERISK;
		case 0x30 : return KEY_POWER;
		case 0x32 : return KEY_KPDOT; 
		case 0x34 : return KEY_HOME; 
		case 0x35 : return KEY_TAB;
		case 0x36 : return KEY_Q;
		case 0x37 : return KEY_W;
		case 0x38 : return KEY_E;
		case 0x39 : return KEY_R;
		case 0x3a : return KEY_T;
		case 0x3b : return KEY_Y;
		case 0x3c : return KEY_U;
		case 0x3d : return KEY_I;
		case 0x3e : return KEY_O;
		case 0x3f : return KEY_P;
		case 0x40 : return KEY_LEFTBRACE;
		case 0x41 : return KEY_RIGHTBRACE;
		case 0x43 : toggleModifier((1 << 4)); return 0xFF; //compose mapped to r_ctrl
		case 0x44 : return KEY_KP7;
		case 0x45 : return KEY_KP8;
		case 0x46 : return KEY_KP9;
		case 0x47 : return KEY_KPMINUS;
		case 0x4a : return KEY_END;
		case 0x4c : toggleModifier((1 << 0)); return 0xFF; // left crtl
		case 0x4d : return KEY_A;
		case 0x4e : return KEY_S;
		case 0x4f : return KEY_D;
		case 0x50 : return KEY_F;
		case 0x51 : return KEY_G;
		case 0x52 : return KEY_H;
		case 0x53 : return KEY_J;
		case 0x54 : return KEY_K;
		case 0x55 : return KEY_L;
		case 0x56 : return KEY_SEMICOLON;
		case 0x57 : return KEY_APOSTROPHE;
		case 0x58 : return KEY_BACKSLASH;
		case 0x59 : return KEY_ENTER;
		case 0x5a : return KEY_KPENTER;
		case 0x5b : return KEY_KP4;
		case 0x5c : return KEY_KP5;
		case 0x5d : return KEY_KP6;
		case 0x5e : return KEY_KP0;
		case 0x60 : return KEY_PAGEUP;
		case 0x62 : return KEY_NUMLOCK;
		case 0x63 : toggleModifier((1 << 5)); return 0xFF; // left shift
		case 0x64 : return KEY_Z; 
		case 0x65 : return KEY_X; 
		case 0x66 : return KEY_C; 
		case 0x67 : return KEY_V; 
		case 0x68 : return KEY_B; 
		case 0x69 : return KEY_N; 
		case 0x6a : return KEY_M; 
		case 0x6b : return KEY_COMMA;
		case 0x6c : return KEY_DOT;
		case 0x6d : return KEY_SLASH;
		case 0x6e : toggleModifier((1 << 1)); return 0xFF; // right shift
		case 0x70 : return KEY_KP1; 
		case 0x71 : return KEY_KP2; 
		case 0x72 : return KEY_KP3; 
		case 0x77 : return KEY_CAPSLOCK; 
		case 0x78 : toggleModifier((1 << 3)); return 0xFF;// meta left
		case 0x79 : return KEY_SPACE; 
		case 0x7a : toggleModifier((1 << 7)); return 0xFF; // meta right
		case 0x7b : return KEY_PAGEDOWN;
		case 0x7c : return KEY_BACKSLASH;
		case 0x7d : return KEY_KPPLUS; 

		case 0x76 : toggleSound(); return KEY_HELP; // help

		case 0x01 : return KEY_STOP; // stop
		case 0x03 : return KEY_AGAIN; // wiederholen (again)
		case 0x19 : return KEY_PROPS; // eigenschaften (props)
		case 0x1A : return KEY_UNDO; // Zurücksetzen (undo)
		case 0x31 : return KEY_FRONT; // Vordergrung (front)
		case 0x33 : return KEY_COPY; // Kopieren (copy)
		case 0x48 : return KEY_OPEN; // Öffnen (open)
		case 0x49 : return KEY_PASTE; // Einsetzen (paste)
		case 0x5F : return KEY_FIND; // Suchen (find)
		case 0x61 : return KEY_CUT; // Ausschneiden (cut)

		default : return 0xFF; // return error code
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
	
	// bellOn();

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
