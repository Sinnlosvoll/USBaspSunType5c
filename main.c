/**
 * Project: AVR ATtiny USB Tutorial at http://codeandlife.com/
 * Author: Joonas Pihlajamaa, joonas.pihlajamaa@iki.fi
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
 

// ************************
// *** USB HID ROUTINES ***
// ************************

// From Frank Zhao's USB Business Card project
// http://www.frank-zhao.com/cache/usbbusinesscard_details.php
PROGMEM const char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)
    0x09, 0x06,                    // USAGE (Keyboard)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x95, 0x08,                    //   REPORT_COUNT (8)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)(Key Codes)
    0x19, 0xe0,                    //   USAGE_MINIMUM (Keyboard LeftControl)(224)
    0x29, 0xe7,                    //   USAGE_MAXIMUM (Keyboard Right GUI)(231)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //   LOGICAL_MAXIMUM (1)
    0x81, 0x02,                    //   INPUT (Data,Var,Abs) ; Modifier byte
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x81, 0x03,                    //   INPUT (Cnst,Var,Abs) ; Reserved byte
    0x95, 0x05,                    //   REPORT_COUNT (5)
    0x75, 0x01,                    //   REPORT_SIZE (1)
    0x05, 0x08,                    //   USAGE_PAGE (LEDs)
    0x19, 0x01,                    //   USAGE_MINIMUM (Num Lock)
    0x29, 0x05,                    //   USAGE_MAXIMUM (Kana)
    0x91, 0x02,                    //   OUTPUT (Data,Var,Abs) ; LED report
    0x95, 0x01,                    //   REPORT_COUNT (1)
    0x75, 0x03,                    //   REPORT_SIZE (3)
    0x91, 0x03,                    //   OUTPUT (Cnst,Var,Abs) ; LED report padding
    0x95, 0x06,                    //   REPORT_COUNT (6)
    0x75, 0x08,                    //   REPORT_SIZE (8)
    0x15, 0x00,                    //   LOGICAL_MINIMUM (0)
    0x25, 0x65,                    //   LOGICAL_MAXIMUM (101)
    0x05, 0x07,                    //   USAGE_PAGE (Keyboard)(Key Codes)
    0x19, 0x00,                    //   USAGE_MINIMUM (Reserved (no event indicated))(0)
    0x29, 0x65,                    //   USAGE_MAXIMUM (Keyboard Application)(101)
    0x81, 0x00,                    //   INPUT (Data,Ary,Abs)
    0xc0                           // END_COLLECTION
};

typedef struct {
	uint8_t modifier;
	uint8_t reserved;
	uint8_t keycode[6];
} keyboard_report_t;

static keyboard_report_t keyboard_report; // sent to PC
volatile static uchar LED_state = 0xff; // received from PC
static uchar idleRate; // repeat rate for keyboards
uint32_t readFromKeyboard = 0;
uchar newResponse = 0;
signed char keys_pressed = 0;


usbMsgLen_t usbFunctionSetup(uchar data[8]) {
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

#define NUM_LOCK 1
#define CAPS_LOCK 2
#define SCROLL_LOCK 4

#define ledRedOn()    PORTC &= ~(1 << PC1)
#define ledRedOff()   PORTC |= (1 << PC1)
#define ledGreenOn()  PORTC &= ~(1 << PC0)
#define ledGreenOff() PORTC |= (1 << PC0)


// due to the 12MHz crystal a clk is 83.33ns
#define DELAY_1_CLK asm volatile("nop")
// 5 are 416.7 ns
#define DELAY_5_CLK DELAY_1_CLK;DELAY_1_CLK;DELAY_1_CLK;DELAY_1_CLK;DELAY_1_CLK
// 10 are 833.33 ns
#define DELAY_10_CLK DELAY_5_CLK;DELAY_5_CLK

usbMsgLen_t usbFunctionWrite(uint8_t * data, uchar len) {
	if (data[0] == LED_state)
        return 1;
    else
        LED_state = data[0];
	
    // LED state changed
	if(LED_state & CAPS_LOCK)
		ledGreenOn(); // LED on
	else
		ledGreenOff(); // LED off
	
	return 1; // Data read, not expecting more
}


#define STATE_WAIT 0
#define STATE_SEND_KEY 1
#define STATE_RELEASE_KEY 2

void key_down(uint8_t down_key) {
    /* we only add a key to our pressed list, when we have less than 6 already pressed.
    ** additional keys will be ignored  */
    if (keys_pressed < 6)
    {
        keyboard_report.keycode[keys_pressed] = down_key;
        keys_pressed++;
    }
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
}

void parseKeyboardResponse() {
    switch((uint32_t)readFromKeyboard) {
        case 0x1C: key_down(0x04); break; // represents a 
        case ((0x1C << 8) | 0x01): key_up(0x04); break; 
        default: break;
    }
}


void startReading() {
    uchar i;
    DELAY_5_CLK;

    if (newResponse)
    {
        readFromKeyboard = 0x02;
    } else {
        readFromKeyboard == readFromKeyboard << 1;
    }
    for (i = 0; i < 8; i++)
    {
        DELAY_10_CLK; //wait for next bit to arrive on kbRXD
        // maybe the time needed to take a measurement will be destructive, but that needs to be tested
        readFromKeyboard |= (PINB & (1 << PB4)) >> PB4;
        readFromKeyboard == readFromKeyboard << 1;
    }
    if (!(readFromKeyboard & 0x01))
    {
        newResponse = 0;
    } else {
        parseKeyboardResponse();
    }
    DELAY_5_CLK;
    DELAY_1_CLK;
    // the additional wait time is for not detecting another byte right at the end of this one
    // 420 would be enought but just to be sure 500
}


int main() {
    uchar i, j;

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
    DDRB = 0;

    /* all inputs except PC0, PC1 */
    DDRC = 0x03;
    PORTC = 0;

    /* init timer */
    clockInit();

    /* main event loop */
    usbInit();
    sei();
    for (;;) {
        usbPoll();
        if (PINB & (1 << PB4))
        {
            //ledRedOn();
            startReading();
        } else {

            ledRedOff();
            //ledGreenOff();
        }
    }
    return 0;
}
