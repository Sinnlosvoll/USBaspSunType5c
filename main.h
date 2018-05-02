#ifndef __MAIN_H__
#define __MAIN_H__


#define NUM_LOCK	0x01
#define CAPS_LOCK	0x02
#define SCROLL_LOCK 0x04
#define COMPOSE		0x08


#define STATE_WAIT 0
#define STATE_SEND_KEY 1
#define STATE_RELEASE_KEY 2

usbMsgLen_t usbFunctionSetup(uint8_t data[8]);
void sendToKeyboard(uint8_t toSend, uint8_t isLastOne);
usbMsgLen_t usbFunctionWrite(uint8_t * data, uint8_t len);
void key_down(uint8_t down_key);
void key_up(uint8_t up_key);
void toggleModifier(uint8_t key);
void resetKeysDown();
uint8_t map(uint8_t keyCodeIn);
void parseKeyboardResponse();
void emergencyParse();
void startReading();
int main();

#endif
