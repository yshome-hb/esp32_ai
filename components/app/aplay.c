#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "http_client.h"
#include "audio.h"
#include "mp3_decode.h"
#include "aplay.h"


#define TAG "aplay:"

#define PLAY_PLAY_BIT   	BIT0
#define PLAY_REQUEST_BIT   	BIT1
#define PLAY_STOP_BIT   	BIT2
#define PLAY_SUSPEND_BIT   	BIT3


static TaskHandle_t xwebplayTask = NULL;
static EventGroupHandle_t aplay_event_group = NULL;//24bit


static int begin_callback (http_parser* a){
  	xEventGroupClearBits(aplay_event_group, PLAY_REQUEST_BIT);
    return 0;
}


static int body_callback(http_parser* a, const char *at, size_t length)
{
    if (xEventGroupGetBits(aplay_event_group) & PLAY_SUSPEND_BIT){
		return 0;
	}
	
	if(decode_send(at, length, 1000 / portTICK_PERIOD_MS)){
		APLAY_DEBUGE(TAG, "decode send on timeout!");
	}
    return 0;
}


static http_parser_settings settings =
{   .on_message_begin = begin_callback,
	.on_header_field = 0,
    .on_header_value = 0,
    .on_url = 0,
    .on_status = 0,
    .on_body = body_callback,
    .on_headers_complete = 0,
    .on_message_complete = 0,
    .on_chunk_header = 0,
    .on_chunk_complete = 0
};


static void web_aplay_task(void* pvParameters)
{
    const char* web_url = (char *)pvParameters;

	ulTaskNotifyTake(pdTRUE, 0);

	decode_start();	
	
    http_client_get(web_url, USER_AGENT, &settings);

    APLAY_DEBUGE(TAG, "web aplay stop!");

    xEventGroupSetBits(aplay_event_group, PLAY_PLAY_BIT);
    vTaskDelete(NULL);
}


static void mp3_aplay_task(void *pvParameters)
{
	FILE* f = (FILE *)pvParameters;

    ssize_t read_bytes;
	char* read_buf = malloc(1024);
	if(read_buf == NULL){
		APLAY_DEBUGE(TAG, "read_buf malloc failed!");
		goto exit;
	}

	//check file header
	char tag[10];
	read_bytes = fread(tag, 1, 10, f);
	if(read_bytes == 10) {
		if (memcmp(tag, "ID3", 3) == 0) {
			int tag_len = ((tag[6] & 0x7F)<< 21)|((tag[7] & 0x7F) << 14) | ((tag[8] & 0x7F) << 7) | (tag[9] & 0x7F);
			APLAY_DEBUGI(TAG, "tag_len: %d %x %x %x %x", tag_len, tag[6], tag[7], tag[8], tag[9]);
			fseek(f, tag_len - 10, SEEK_SET);
		}else {
			fseek(f, 0, SEEK_SET);
		}
	}
	vTaskDelay(100 / portTICK_PERIOD_MS);
	xEventGroupClearBits(aplay_event_group, PLAY_REQUEST_BIT);
	
	decode_start();	

	for(;;){

        if (xEventGroupGetBits(aplay_event_group) & PLAY_STOP_BIT)
        	break;

        if (xEventGroupGetBits(aplay_event_group) & PLAY_SUSPEND_BIT){
			vTaskDelay(100 / portTICK_PERIOD_MS);
			continue;
		}	

		read_bytes = fread(read_buf, 1, 1024, f);
		if(read_bytes <= 0){
			break;
		}

		if(decode_send((const char*)read_buf, read_bytes, 1000 / portTICK_PERIOD_MS)){
			APLAY_DEBUGE(TAG, "decode send on timeout!");
		}
		vTaskDelay(20 / portTICK_PERIOD_MS);
	}

exit:
	free(read_buf);
    fclose(f);
	xEventGroupSetBits(aplay_event_group, PLAY_PLAY_BIT);
    vTaskDelete(NULL);	
}


static void wav_aplay_task(void *pvParameters)
{
	FILE* f = (FILE *)pvParameters;

	WaveHeader waveHeader;
	if(fread(&waveHeader, 1, sizeof(WaveHeader), f) != sizeof(WaveHeader)){
		APLAY_DEBUGE(TAG, "wav file read failed!");
		goto exit;
	}

	I2S_Params_st i2s_params;
	if(!waveheader_read(&waveHeader, &i2s_params)){
		APLAY_DEBUGE(TAG, "waveheader read failed!");
		goto exit;
	}

	Audio_setSampleRates(i2s_params.rate);
	Audio_setTxChannel(i2s_params.channel);

    ssize_t read_bytes;
	char* read_buf = malloc(1024);
	if(read_buf == NULL){
		APLAY_DEBUGE(TAG, "read_buf malloc failed!");
		goto exit;
	}

	xEventGroupClearBits(aplay_event_group, PLAY_REQUEST_BIT);

	for(;;){
		
        if (xEventGroupGetBits(aplay_event_group)&PLAY_STOP_BIT)
        	break;

        if (xEventGroupGetBits(aplay_event_group) & PLAY_SUSPEND_BIT){
			Audio_i2s_clear();
			vTaskDelay(100 / portTICK_PERIOD_MS);
			continue;
		}	

		read_bytes = fread(read_buf, 1, 1024, f);

		if(read_bytes <= 0){
			break;
		}

		Audio_i2sWrite(read_buf, read_bytes, 1000 / portTICK_RATE_MS);
		vTaskDelay(20 / portTICK_PERIOD_MS);
	}

	Audio_i2s_clear();
	free(read_buf);
exit:
    fclose(f);
	xEventGroupSetBits(aplay_event_group, PLAY_PLAY_BIT);
    vTaskDelete(NULL);	
}


void aplay_init()
{
	if(aplay_event_group)
		return;
		
    aplay_event_group = xEventGroupCreate();
	xEventGroupSetBits(aplay_event_group, PLAY_PLAY_BIT);
	decode_init();
}


void aplay_deinit()
{
	if(aplay_event_group == NULL)
        return;

	aplay_stop();
    xEventGroupWaitBits(aplay_event_group, PLAY_PLAY_BIT, pdTRUE, pdTRUE, portMAX_DELAY);

	vEventGroupDelete(aplay_event_group);
	aplay_event_group = NULL;
}


void aplay_suspend()
{
	if(aplay_event_group == NULL)
        return;

	if (xEventGroupGetBits(aplay_event_group) & (PLAY_PLAY_BIT|PLAY_REQUEST_BIT)){
		return;
	}

	if (xEventGroupGetBits(aplay_event_group) & PLAY_SUSPEND_BIT){
		decode_suspend(false);	
    	xEventGroupClearBits(aplay_event_group, PLAY_SUSPEND_BIT);
	}else{
        xEventGroupSetBits(aplay_event_group, PLAY_SUSPEND_BIT);
		vTaskDelay(50 / portTICK_PERIOD_MS);
		decode_suspend(true);			
	}
}


void aplay_stop()
{
	if(aplay_event_group == NULL)
        return;

    xEventGroupSetBits(aplay_event_group, PLAY_STOP_BIT);
	if(xwebplayTask)
    	xTaskNotifyGive(xwebplayTask);	
    xEventGroupWaitBits(aplay_event_group, PLAY_PLAY_BIT, pdFALSE, pdTRUE, portMAX_DELAY);	
	decode_destroy();
}


Aplay_State_E aplay_get_state()
{
	if(aplay_event_group == NULL || (xEventGroupGetBits(aplay_event_group) & PLAY_PLAY_BIT))
        return APLAY_STATE_STOP;

	if (xEventGroupGetBits(aplay_event_group) & PLAY_REQUEST_BIT){
		return APLAY_STATE_REQUEST;
	}

	if (xEventGroupGetBits(aplay_event_group) & PLAY_SUSPEND_BIT){
		return APLAY_STATE_SUSPEND;
	}	

	return APLAY_STATE_START;	
}


void mp3_aplay_start(FILE* f)
{
    xEventGroupWaitBits(aplay_event_group, PLAY_PLAY_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
	xEventGroupClearBits(aplay_event_group, PLAY_SUSPEND_BIT);
	xEventGroupClearBits(aplay_event_group, PLAY_STOP_BIT);	
	xEventGroupSetBits(aplay_event_group, PLAY_REQUEST_BIT);	

    xTaskCreatePinnedToCore(mp3_aplay_task, "mp3_aplay_task", 2048, (void*)f, 4, NULL, 0);
}


void wav_aplay_start(FILE* f)
{
    xEventGroupWaitBits(aplay_event_group, PLAY_PLAY_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
	xEventGroupClearBits(aplay_event_group, PLAY_SUSPEND_BIT);
	xEventGroupClearBits(aplay_event_group, PLAY_STOP_BIT);	
	xEventGroupSetBits(aplay_event_group, PLAY_REQUEST_BIT);

    xTaskCreatePinnedToCore(wav_aplay_task, "wav_aplay_task", 2048, (void*)f, 4, NULL, 0);
}


void web_aplay_start(char* web_url)
{
    xEventGroupWaitBits(aplay_event_group, PLAY_PLAY_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
	xEventGroupClearBits(aplay_event_group, PLAY_SUSPEND_BIT);
	xEventGroupClearBits(aplay_event_group, PLAY_STOP_BIT);	
	xEventGroupSetBits(aplay_event_group, PLAY_REQUEST_BIT);

    xTaskCreatePinnedToCore(web_aplay_task, "web_aplay_task", 4224, (void*)web_url, 4, &xwebplayTask, 0);
}
