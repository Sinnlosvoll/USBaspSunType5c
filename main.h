#ifndef __MAIN_H__
#define __MAIN_H__

/*  TOGGLES  */
/* #define DEBUGMODE *//* enables/disables display of wiggles on WIGGLE_PIN */


#define NUM_LOCK    1
#define CAPS_LOCK   2
#define SCROLL_LOCK 4
#define COMPOSE     0x08



#define RED_LED_PIN 		PC1
#define RED_LED_PORT 		PORTC
#define ledRedOn()    		RED_LED_PORT &= ~(1 << RED_LED_PIN)
#define ledRedOff()   		RED_LED_PORT |= (1 << RED_LED_PIN)

#define LED_GREEN_PIN  		PC0
#define LED_GREEN_PORT 		PORTC
#define ledGreenOn() 	 	LED_GREEN_PORT &= ~(1 << LED_GREEN_PIN)
#define ledGreenOff() 		LED_GREEN_PORT |= (1 << LED_GREEN_PIN)

#define WIGGLE_PIN  		PB5
#define WIGGLE_PORT 		PORTB
#define wiggleHigh()     	WIGGLE_PORT |= (1 << WIGGLE_PIN)
#define wiggleLow()      	WIGGLE_PORT &= ~(1 << WIGGLE_PIN)

#define TXD_PIN  			PB3
#define TXD_PORT 			PORTB
#define TXDhigh()    		TXD_PORT &= ~(1 << TXD_PIN)
#define TXDlow()     		TXD_PORT |= (1 << TXD_PIN)


/* due to the 12MHz crystal a clk is 83.33ns */
#define DELAY_1_CLK() asm volatile("nop")
#define DELAY_HALF_KB_CLK() _delay_us(417)
#define DELAY_FULL_KB_CLK() _delay_us(833)




 /* KEYBOARD COMMANDS */

#define bellOn()      sendToKeyboard(0x02)
#define bellOff()     sendToKeyboard(0x03)
#define updateLeds()  sendToKeyboard(0x0e);sendToKeyboard(leds)
#define resetKbrd()	  sendToKeyboard(0x01)
#define clickOn()	  sendToKeyboard(0x0A)
#define clickOff()	  sendToKeyboard(0x0B)
#define getLayout()	  sendToKeyboard(0x0F)

#endif
