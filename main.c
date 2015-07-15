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
#include "main.h"

 

/* ************************
// *** USB HID ROUTINES ***
// ************************

// From Frank Zhao's USB Business Card project
// http://www.frank-zhao.com/cache/usbbusinesscard_details.php */
PROGMEM const char usbHidReportDescriptor[USB_CFG_HID_REPORT_DESCRIPTOR_LENGTH] = {
    0x05, 0x01,                    /* USAGE_PAGE (Generic Desktop) */
    0x09, 0x06,                    /* USAGE (Keyboard) */
    0xa1, 0x01,                    /* COLLECTION (Application) */
    0x75, 0x01,                    /*   REPORT_SIZE (1) */
    0x95, 0x08,                    /*   REPORT_COUNT (8) */
    0x05, 0x07,                    /*   USAGE_PAGE (Keyboard)(Key Codes) */
    0x19, 0xe0,                    /*   USAGE_MINIMUM (Keyboard LeftControl)(224) */
    0x29, 0xe7,                    /*   USAGE_MAXIMUM (Keyboard Right GUI)(231) */
    0x15, 0x00,                    /*   LOGICAL_MINIMUM (0) */
    0x25, 0x01,                    /*   LOGICAL_MAXIMUM (1) */
    0x81, 0x02,                    /*   INPUT (Data,Var,Abs) ; Modifier byte */
    0x95, 0x01,                    /*   REPORT_COUNT (1) */
    0x75, 0x08,                    /*   REPORT_SIZE (8) */
    0x81, 0x03,                    /*   INPUT (Cnst,Var,Abs) ; Reserved byte */
    0x95, 0x05,                    /*   REPORT_COUNT (5) */
    0x75, 0x01,                    /*   REPORT_SIZE (1) */
    0x05, 0x08,                    /*   USAGE_PAGE (LEDs) */
    0x19, 0x01,                    /*   USAGE_MINIMUM (Num Lock) */
    0x29, 0x05,                    /*   USAGE_MAXIMUM (Kana) */
    0x91, 0x02,                    /*   OUTPUT (Data,Var,Abs) ; LED report */
    0x95, 0x01,                    /*   REPORT_COUNT (1) */
    0x75, 0x03,                    /*   REPORT_SIZE (3) */
    0x91, 0x03,                    /*   OUTPUT (Cnst,Var,Abs) ; LED report padding */
    0x95, 0x06,                    /*   REPORT_COUNT (6) */
    0x75, 0x08,                    /*   REPORT_SIZE (8) */
    0x15, 0x00,                    /*   LOGICAL_MINIMUM (0) */
    0x25, 0x65,                    /*   LOGICAL_MAXIMUM (101) */
    0x05, 0x07,                    /*   USAGE_PAGE (Keyboard)(Key Codes) */
    0x19, 0x00,                    /*   USAGE_MINIMUM (Reserved (no event indicated))(0) */
    0x29, 0x65,                    /*   USAGE_MAXIMUM (Keyboard Application)(101) */
    0x81, 0x00,                    /*   INPUT (Data,Ary,Abs) */
    0xc0                           /* END_COLLECTION */
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
signed char keys_pressed = 0;
uint8_t keysHaveChanged = 0;
uint16_t emergencyResponceCounter = 0;
uint8_t leds = 0;


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




void wiggle(uchar times) {
    if (DEBUGMODE)
    {
        uchar i;
        for ( i = 0; i < times ; i++)
        {
            wiggleHigh();
            DELAY_1_CLK;
            wiggleLow();
            DELAY_1_CLK;
        }
    }

}



void sendToKeyboard(uint8_t toSend) {
    uchar i;
    cli();
    TXDhigh();
    wiggle(1);
    DELAY_FULL_KB_CLK();
    // the keyboard wants its data as inverted logic and lsb first
    // don't ask me why they chose this format
    for (i = 0; i < 7; i++)
    {
        if (toSend  & (0x01 << i))
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


void displayValue(uint32_t toDisplay) {
    if (DEBUGMODE)
    {
        uint8_t counter;

        for (counter = 0; counter < 32; counter++)
        {
            if ((toDisplay >> (31 - counter)) & 0x01)
            {
                wiggleHigh();
                TXDhigh();
                DELAY_1_CLK;
                TXDlow();
                wiggleLow();
            } else {
                TXDhigh();
                DELAY_1_CLK;
                TXDlow();
            }
        }
    }
    
}


usbMsgLen_t usbFunctionWrite(uint8_t * data, uchar len) {
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

void toggleModifier(uchar key) {
    keyboard_report.modifier ^= key;
}


uint8_t map(uint8_t keyCodeIn) {
    switch ((keyCodeIn & ~(0x01))) {
        case 0x4c : return 0x04; // a
        case 0x8c : return 0x16; // s
        case 0x0c : return 0x07; // d
        case 0x92 : return 0x14; // q
        case 0x12 : return 0x1a; // w
        case 0xE2 : return 0x08; // e
        case 0x62 : return 0x15; // r
        case 0xA2 : return 0x17; // t
        case 0x22 : return 0x1C; // y (z-key)
        case 0xC2 : return 0x18; // u
        case 0x42 : return 0x0c; // i
        case 0x82 : return 0x12; // o
        case 0x02 : return 0x13; // p
        case 0xFC : return 0x2f; // [ (ü-key)
        case 0x7C : return 0x30; // ] (pound key)
        case 0x2a : return 0x2a; // backspace
        case 0x64 : return 0x28; // enter
        case 0x88 : toggleModifier((1 << 5)); return 0xFF; // right shift
        case 0x60 : return 0x2c; // spacebar
        case 0xe6 : return 0x50; // leftArrow
        case 0x26 : return 0x51; // downarrow
        case 0xc6 : return 0x4f; // rightArrow
        case 0xd6 : return 0x52; // uparrow
        case 0x84 : return 0x62; // keypad 0
        case 0xb2 : return 0x63; // keypad ,
        case 0xa4 : return 0x58; // keypad enter
        case 0xe0 : toggleModifier((1 << 3)); return 0xFF;// meta left
        case 0x36 : toggleModifier((1 << 2)); return 0xFF; // alt left
        case 0xcc : toggleModifier((1 << 0)); return 0xFF; // ctrl left
        case 0x38 : toggleModifier((1 << 1)); return 0xFF; // shift left
        case 0x10 : return 0x39; // capslock
        case 0x52 : return 0x2b; // tabulator
        case 0xb8 : return 0x53; // num lock
        case 0xf4 : return 0x09; // f
        case 0x74 : return 0x0A; // g
        case 0xb4 : return 0x0b; // h
        case 0x34 : return 0x0D; // j
        case 0xd4 : return 0x0e; // k
        case 0x54 : return 0x0f; // l
        case 0x94 : return 0x33; // ; ö
        case 0x14 : return 0x34; // ' ä
        case 0xd8 : return 0x1d; // z
        case 0x58 : return 0x1b; // x
        case 0x98 : return 0x06; // c
        case 0x18 : return 0x19; // v
        case 0xe8 : return 0x05; // b
        case 0x68 : return 0x11; // n
        case 0xa8 : return 0x10; // m
        case 0x28 : return 0x36; // ,
        case 0xc8 : return 0x37; // .
        case 0x48 : return 0x38; // /
        case 0x5e : return 0x3a; // F1
        case 0x9e : return 0x3b; // 
        case 0xee : return 0x3c; //
        case 0xae : return 0x3d; //
        case 0xce : return 0x3e; //
        case 0x8e : return 0x3f; //
        case 0xf5 : return 0x40; //
        case 0xe5 : return 0x41; //
        case 0xb5 : return 0x42; //
        case 0x1e : return 0x43; //
        case 0x6e : return 0x44; //
        case 0x2e : return 0x45; // F12
        case 0xaa : return 0x35; // `
        case 0x86 : return 0x1e; // 1
        case 0x06 : return 0x1f; // 
        case 0xfa : return 0x20; //
        case 0x7a : return 0x21; //
        case 0xba : return 0x22; //
        case 0x3a : return 0x23; //
        case 0xda : return 0x24; //
        case 0x5a : return 0x25; //
        case 0x9a : return 0x26; //
        case 0x1a : return 0x27; // 0
        case 0xea : return 0x2D; // -
        case 0x6a : return 0x2E; // =
        case 0x46 : return 0x29; // esc
        case 0x96 : return 0x46; // printscrn
        case 0x16 : return 0x47; // scroll lock
        case 0x56 : return 0x48; // pause
        case 0x4a : return 0x7f; // mute
        case 0xb7 : return 0x81; // vol down
        case 0xd7 : return 0x80; // vol up 
        case 0xf2 : return 0x66; // power
        case 0x8a : return 0x54; // keypad /
        case 0x0a : return 0x55; // keypad *
        case 0x1c : return 0x56; // keypad -
        case 0x4d : return 0x57; // keypad +
        case 0xF0 : return 0x59; // keypad 1
        case 0x70 : return 0x5a; //
        case 0xb0 : return 0x5b; //
        case 0x24 : return 0x5c; //
        case 0xc4 : return 0x5d; //
        case 0x44 : return 0x5e; //
        case 0xdc : return 0x5f; //
        case 0x5c : return 0x60; //
        case 0x9c : return 0x61; // keypad 9
        case 0xc9 : return 0x49; // insert
        case 0xd2 : return 0x4a; // home
        case 0xF8 : return 0x4b; // PgUp 
        case 0xbc : return 0x4c; // delete
        case 0xac : return 0x4d; // end
        case 0x20 : return 0x4e; // PgDn


        default : return 0xFF; // return error code
    }
}

void parseKeyboardResponse() {
    uint8_t usbHIDcode = 0xFF;
    if ((readFromKeyboard & 0xFF00) != 0)
    {
        usbHIDcode = map((uint8_t)(readFromKeyboard >> 8));

        if (usbHIDcode == 0xFF)
            wiggle(12);
        else
            key_up(usbHIDcode);

    } else {
        usbHIDcode = map((uint8_t)(readFromKeyboard >> 0));
        if (usbHIDcode == 0xFF)
            wiggle(12);
        else
            key_down(usbHIDcode);

    }
    
    wiggle(7);
    displayValue((uint32_t)readFromKeyboard);

    readFromKeyboard = 0;

}


void emergencyParse() {
    // for now lets just assume, that this only happens when multiple keys are pressed and one of them comes back up
    // for an unknown reason there is no break command for that key, only for the last one of that sequence that comes back up
    key_up(map((uint8_t)readFromKeyboard));
    readFromKeyboard = 0;
    wiggle(4);
    displayValue(readFromKeyboard);
}


void startReading() {
    uchar i;
    DELAY_HALF_KB_CLK();
    // wiggleHigh();rea

    for (i = 0; i < 8; i++)
    {
        DELAY_FULL_KB_CLK();
        
        readFromKeyboard = readFromKeyboard << 1;
        readFromKeyboard |= (PINB & (1 << PB4)) >> PB4;
        // indicate measurement
        wiggle(1);
    }


    if (readFromKeyboard & 0x01)
    {
        parseKeyboardResponse();
        wiggle(2);
    } else {
        wiggle(3);
        emergencyResponceCounter = 2000;
        
    }
    DELAY_HALF_KB_CLK();
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
    DDRB |= (1 << PB5); 
    DDRB |= (1 << PB3);

    /* all inputs except PC0, PC1 */
    DDRC = 0x03;
    PORTC = 0;

    /* init timer */
    clockInit();

    /* main event loop */
    usbInit();
    sei();
    ledRedOff();
    TXDlow();
    
    for (;;) {
        usbPoll();
        if (PINB & (1 << PB4))
        {
            // ledRedOn();
            // wiggleHigh();
            ledRedOn();
            emergencyResponceCounter = 0;
            startReading();

        } else {
            // wiggleLow();
            if (emergencyResponceCounter == 1)
            {
                emergencyParse();
                emergencyResponceCounter = 0;
            } else {
                if (emergencyResponceCounter > 0) 
                    emergencyResponceCounter--;
            }
            ledRedOff();
            //ledGreenOff();
        }
        if (usbInterruptIsReady() && keysHaveChanged)
        {
            usbSetInterrupt((void *)&keyboard_report, sizeof(keyboard_report));
            keysHaveChanged = 0;
        }
    }
    return 0;
}
