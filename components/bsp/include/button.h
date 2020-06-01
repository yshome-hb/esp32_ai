#ifndef _BUTTON_H_
#define _BUTTON_H_


#define BUTTON_OFF      0x00
#define BUTTON_RELEASE  0x01
#define BUTTON_PRESS    0x10
#define BUTTON_ON       0x11


typedef void (*action_callback_t)    (uint8_t action);

typedef struct{
	uint8_t num;
	action_callback_t action_cb;

}Button_Cfg_st;

void button_if_init(Button_Cfg_st *p_buttons, uint8_t count);
void button_if_deinit();


#endif /* _BUTTON_H_ */