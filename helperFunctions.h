#ifndef HELPERFUNC_H
#define HELPERFUNC_H

#define ledRedOn()	 		PORTC &= ~(1 << PC1)
#define ledRedOff()			PORTC |= (1 << PC1)
#define debugtx_hi()		PORTC |= (1 << PC2)
#define debugtx_low()		PORTC &= ~(1 << PC2)
#define tx_low()				PORTB &= ~(1 << PB3)
#define tx_hi()	  			PORTB |= (1 << PB3)
#define ledGreenOn()  	PORTC &= ~(1 << PC0)
#define ledGreenOff() 	PORTC |= (1 << PC0)
#define bellOn()				sendToKeyboard(0x02, 1)
#define bellOff()	  		sendToKeyboard(0x03, 1)
#define clickOn() 			soundIsOn = 1;sendToKeyboard(0x0a, 1)
#define clickOff()    	soundIsOn = 0;sendToKeyboard(0x0b, 1)
#define updateLeds()  	sendToKeyboard(0x0e, 1);sendToKeyboard(leds, 1)
#define resetKbrd()	  sendToKeyboard(0x01)
#define getLayout()	  sendToKeyboard(0x0F)

// due to the 12MHz crystal a clk is 83.33ns
#define DELAY_1_CLK asm volatile("nop")
// 5 are 416.7 ns
#define DELAY_5_CLK DELAY_1_CLK;DELAY_1_CLK;DELAY_1_CLK;DELAY_1_CLK;DELAY_1_CLK
// 10 are 833.33 ns
#define DELAY_10_CLK DELAY_5_CLK;DELAY_5_CLK

#define DELAY_HALF_KB_CLK() _delay_us(417)
#define DELAY_FULL_KB_CLK() _delay_us(833)


void toggleSound();
void wiggle(uchar times);
void displayValue32(uint32_t toDisplay);
void displayValue8(uint8_t toDisplay);

#include "helperFunctions.c" 

#endif

#ifndef B_CONST
/// \brief Provides a C Standard compliant way of coding binary constants.
///
/// When building with some command-line options,  GCC may issue a warning when
/// using  binary  constants of  the  form  \c 0bnnnnnnnn
/// \verbatim warning: binary constants are a GCC extension \endverbatim
/// This macro provides an alternate means of specifying binary constants.
///
/// Within  the macro,  \a n  is  interpreted as  an unsigned  long long  octal
/// constant, and the  result is an unsigned long long  constant.  If the other
/// octal digits '2' through '7' are used,  the macro will detect this and will
/// trigger a division by zero to draw it to your attention:
/// \verbatim warning: division by zero \endverbatim
/// ... or  in some  cases will  result in an  overflow:
/// \verbatim warning: integer constant is too large for its type \endverbatim
/// The use of non-octal characters will result in yet a different error:
/// \verbatim error: invalid suffix "x0Aull" on integer constant \endverbatim
///
/// The parameter \a n may be up to 22 bits.  Longer constants may result in an
/// overflow as above.
///
/// You may pass another  macro in place of \a n, but  its final expansion must
/// satisfy the above constraints.  Expressions are not supported.
///
/// \b Example:
///
/// \par
/// \code
///   foo = 0xA5;
///   bar = B_CONST(10100101);
/// \endcode
///
/// \par
/// Both variables will be assigned the same value.
///
/// \par External reference:
///   - http://www.mikrocontroller.net/topic/254816
///
/// \hideinitializer
///
  #define B_CONST(n) B_CONST_(n)
  #ifndef __DOXYGEN__
    #ifndef B_CONST_
      #define B_CONST_(n) ((0##n##ull & 0666666666666666666666ull) ? 0/0 :    \
                          ((0##n##ull >>  0) & 0x000001) |                    \
                          ((0##n##ull >>  2) & 0x000002) |                    \
                          ((0##n##ull >>  4) & 0x000004) |                    \
                          ((0##n##ull >>  6) & 0x000008) |                    \
                          ((0##n##ull >>  8) & 0x000010) |                    \
                          ((0##n##ull >> 10) & 0x000020) |                    \
                          ((0##n##ull >> 12) & 0x000040) |                    \
                          ((0##n##ull >> 14) & 0x000080) |                    \
                          ((0##n##ull >> 16) & 0x000100) |                    \
                          ((0##n##ull >> 18) & 0x000200) |                    \
                          ((0##n##ull >> 20) & 0x000400) |                    \
                          ((0##n##ull >> 22) & 0x000800) |                    \
                          ((0##n##ull >> 24) & 0x001000) |                    \
                          ((0##n##ull >> 26) & 0x002000) |                    \
                          ((0##n##ull >> 28) & 0x004000) |                    \
                          ((0##n##ull >> 30) & 0x008000) |                    \
                          ((0##n##ull >> 32) & 0x010000) |                    \
                          ((0##n##ull >> 34) & 0x020000) |                    \
                          ((0##n##ull >> 36) & 0x040000) |                    \
                          ((0##n##ull >> 38) & 0x080000) |                    \
                          ((0##n##ull >> 40) & 0x100000) |                    \
                          ((0##n##ull >> 42) & 0x200000) )
    #else
      #error MACRO COLLISION. Macro 'B_CONST_' is already defined.
    #endif
  #endif
#else
  #error MACRO COLLISION. Macro 'B_CONST' is already defined.
#endif