#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "button.h"


#define TAG "BTN:"


typedef struct{
	uint8_t value;
	uint8_t filter;

}Button_Val_st;

static Button_Cfg_st *button_cfg = NULL;
static Button_Val_st *button_val = NULL;
static uint8_t button_count = 0;
static TaskHandle_t xbuttonTask = NULL;


static uint8_t button_filter(Button_Cfg_st *cfg, Button_Val_st *val)
{
	uint8_t cache = val->value;
	val->filter <<= 1;
	val->filter |= (gpio_get_level(cfg->num)>0);

	if((val->filter&0xFF) == 0xFF){
		val->value = 1;
	}else if((val->filter&0xFF) == 0x00){
		val->value = 0;
	}

	val->value |= (cache<<4);
	return val->value;
}


static void button_task(void *pvParameters)
{
	for(int i=0; i<button_count; i++){
		button_val[i].value = 0x11;
		button_val[i].filter = 0xFF;
		gpio_pad_select_gpio(button_cfg[i].num);
    	gpio_set_direction(button_cfg[i].num, GPIO_MODE_INPUT);
		gpio_set_pull_mode(button_cfg[i].num, GPIO_PULLUP_ONLY);
	}

    for(;;){

		for(int i=0; i<button_count; i++){
			button_filter(button_cfg+i, button_val+i);
			if(button_val[i].value > BUTTON_OFF && button_val[i].value < BUTTON_ON)
				 button_cfg[i].action_cb(button_val[i].value);
		}

		if(ulTaskNotifyTake(pdTRUE, 30 / portTICK_RATE_MS)){
            break;
        }
    }
	free(button_cfg);
	button_cfg = NULL;	
	free(button_val);
	button_val = NULL;
    vTaskDelete(NULL);
}


void button_if_init(Button_Cfg_st *p_buttons, uint8_t count)
{
    if(xbuttonTask)
		return;

	button_count = count;
	button_cfg = (Button_Cfg_st*)malloc(sizeof(Button_Cfg_st) * button_count);
	if(button_cfg == NULL){
		ESP_LOGE(TAG, "button_cfg malloc failed!");
		return;
	}

	button_val = (Button_Val_st*)malloc(sizeof(Button_Val_st) * button_count);
	if(button_val == NULL){
		free(button_cfg);
		button_cfg = NULL;
		ESP_LOGE(TAG, "button_val malloc failed!");
		return;
	}

	memcpy(button_cfg, p_buttons, sizeof(Button_Cfg_st) * button_count);
	xTaskCreate(button_task, "button_task", 1792, NULL, 3, &xbuttonTask);
}


void button_if_destroy()
{
	if(xbuttonTask == NULL)
		return;
	
    xTaskNotifyGive(xbuttonTask);
	xbuttonTask = NULL;
}

