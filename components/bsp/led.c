#include "I2Cdev.h"
#include "led.h"

uint8_t Led_version(){
	uint8_t buf;
	I2C_readByte(LED_ADDR, LED_VERSION, &buf);
	return buf;
}

void Led_reset(){
	I2C_writeByte(LED_ADDR, LED_RST, 1);
}

void Led_setLedMode(uint8_t mode){
	I2C_writeByte(LED_ADDR, LED_MODE, mode);
}


