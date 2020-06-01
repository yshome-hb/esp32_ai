#ifndef __LED_H__
#define __LED_H__

#include <stdint.h>

#define LED_ADDR 		0x7D

#define LED_VERSION     	0
#define NC	    	        1
#define LED_RST	    	    2
#define LED_MODE	    	3

uint8_t Led_version();
void Led_reset();
void Led_setLedMode(uint8_t mode);

#endif
