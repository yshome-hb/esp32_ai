#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "audio.h"
#include "sysfat.h"
#include "mcotton_rest.h"
#include "wav_record.h"


#define TAG "RECORD:"

#define CODE_NO_VOICE	-1

#define ABS16(n)	((n^(n>>15))-(n>>15))

#define FILE_NAME 	"record.wav"
#define FILE_TYPE 	"audio/wav"

#define SIGNAL_THRESHOLD    	(500)
#define ENERGY_THRESHOLD_MIN	(300000)
#define ZERO_CROSS_MIN    		(50)
#define ZERO_CROSS_MAX    		(270)

#define REST_DETECT_TIMEOUT_MS    (10000)
#define REST_RECORD_TIMEOUT_MS    (10000)
#define FILE_RECORD_TIMEOUT_MS	  (120000)	
#define REST_RECORD_INVALID_CNT   (12)

#define REC_PLAY_BIT   	BIT0
#define REC_DETECT_BIT  BIT1
#define REC_STOP_BIT   	BIT2
#define REC_POST_BIT  	BIT3


static recordDone_callback_t recordDone_callback = NULL;
static EventGroupHandle_t record_event_group = NULL;//24bit
static TaskHandle_t xrecordTask = NULL;
static uint32_t record_time = 0;


void record_setChannel(Mic_Type_E mic)
{
	if(mic == MIC_BOARD){
		Audio_setRxChannel(I2S_CHANNEL_FMT_ONLY_LEFT);
		Audio_setLeftMic(false);
		Audio_setRightMic(true);
		Audio_setRightMicVolume(45);
	}else if(mic == MIC_JACK){
		Audio_setRxChannel(I2S_CHANNEL_FMT_ONLY_RIGHT);
		Audio_setRightMic(false);
		Audio_setLeftMic(true);
		Audio_setLeftMicVolume(45);
	}else{
		Audio_setRxChannel(I2S_CHANNEL_FMT_RIGHT_LEFT);
		Audio_setRightMic(true);
		Audio_setRightMicVolume(45);
		Audio_setLeftMic(true);
		Audio_setLeftMicVolume(45);		
	}
}


static void wav_record_task(void* pvParameters)
{
	FILE* f = (FILE *)pvParameters;
	
	I2S_Params_st i2s_params = {
    	.rate = SR_16KHZ,
		.wlen = I2S_16BIT,
		.channel = I2S_CHANNEL_FMT_ONLY_LEFT
    };
	
	Audio_setSampleRates(i2s_params.rate);

	WaveHeader waveHeader;
	waveheader_init(&waveHeader, &i2s_params);

	if(fwrite(&waveHeader, 1, sizeof(waveHeader), f) != sizeof(waveHeader)){
		RECORD_DEBUGE(TAG, "Failed to write waveHeader");
		goto exit;
	}

	char* wave_buf = malloc(2048);
	if(wave_buf == NULL){
		RECORD_DEBUGE(TAG, "wave_buf malloc failed!");
		goto exit;
	}

	RECORD_DEBUGI(TAG, "start record!");
	xEventGroupClearBits(record_event_group, REC_DETECT_BIT);

    ssize_t wave_bytes;
	uint32_t startTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

	while(record_time < FILE_RECORD_TIMEOUT_MS){

		if (xEventGroupGetBits(record_event_group)&REC_STOP_BIT)
        	break;

	    wave_bytes = Audio_i2sRead(wave_buf, 2048, portMAX_DELAY);

	    if(wave_bytes <= 0){
	    	break;
	    }

		if(fwrite(wave_buf, 1, wave_bytes, f) != wave_bytes){
			RECORD_DEBUGE(TAG, "Failed to write wave_buf");
			break;
		}

		record_time = xTaskGetTickCount() * portTICK_PERIOD_MS - startTime;
	}

	free(wave_buf);
	wave_bytes = ftell(f);
	fseek(f, 0, SEEK_SET);

	waveHeader.data.ChunkSize = wave_bytes - sizeof(waveHeader);
	waveHeader.riff.ChunkSize = wave_bytes - 8;

	if(fwrite(&waveHeader, 1, sizeof(waveHeader), f) != sizeof(waveHeader)){
		RECORD_DEBUGE(TAG, "Failed to write waveHeader");
	}

exit:
	record_time = 0;
    fclose(f);
	xEventGroupSetBits(record_event_group, REC_PLAY_BIT);
    vTaskDelete(NULL);
}


static bool signal_check(int16_t* data, uint32_t size)
{
	int i = 0;
	uint16_t zero_cnt = 0;
	int16_t  x_pre    = 0;
	uint32_t abs_sum = 0;

	while((i < size) && (ABS16(data[i]) < SIGNAL_THRESHOLD)){
		i++;
	}

	if(i >= size){
		return false;
	}

	abs_sum = ABS16(data[i]);
	x_pre = data[i];

	while(++i < size){

		if(ABS16(data[i]) < SIGNAL_THRESHOLD){
			x_pre = 0;
			continue;
		}

		if(x_pre && ((x_pre^data[i])>>15)){
			zero_cnt++;
		}

		abs_sum += ABS16(data[i]);
		x_pre = data[i];
	}
	return (abs_sum > ENERGY_THRESHOLD_MIN && zero_cnt > ZERO_CROSS_MIN && zero_cnt < ZERO_CROSS_MAX);
}


static void restPost_callback(int code, const char* msg)
{
	xEventGroupClearBits(record_event_group, REC_POST_BIT);
	xEventGroupSetBits(record_event_group, REC_PLAY_BIT);
	recordDone_callback(code, msg);
	vTaskDelay(1000 / portTICK_RATE_MS);
}


static void rest_record_task(void* pvParameters)
{
	I2S_Params_st* i2s_params = (I2S_Params_st*)pvParameters;

	WaveHeader waveHeader;
	waveheader_init(&waveHeader, i2s_params);
	if(sys_data_init((uint8_t *)&waveHeader, sizeof(waveHeader), FILE_NAME, FILE_TYPE) != ESP_OK){
		vTaskDelete(NULL);
	}

	size_t flash_wr_size = 0;
	char* wave_buf = NULL;

	for(;;){

	    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		xEventGroupWaitBits(record_event_group, REC_PLAY_BIT, pdTRUE, pdTRUE, portMAX_DELAY);	
		xEventGroupSetBits(record_event_group, REC_DETECT_BIT);	
		xEventGroupClearBits(record_event_group, REC_POST_BIT);
		Audio_setSampleRates(i2s_params->rate);		

	    flash_wr_size = 0;
		wave_buf = malloc(2048);
		if(wave_buf == NULL){
			RECORD_DEBUGE(TAG, "wave_buf malloc failed!");
			restPost_callback(CODE_NO_VOICE, "no voice");
			continue;
		}

		uint32_t nowTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
		uint32_t endTime = nowTime + REST_DETECT_TIMEOUT_MS;
		uint8_t record_cnt = 0;

		while(nowTime < endTime){

			if(Audio_i2sRead(wave_buf, 2048, portMAX_DELAY) <= 0){
		    	break;
		    }

			if(signal_check((int16_t *)wave_buf, 1024)){
				record_cnt = REST_RECORD_INVALID_CNT;
				break;
			}

			nowTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
		}

		if(!record_cnt){
			RECORD_DEBUGE(TAG, "no voice found!");
			free(wave_buf);
			restPost_callback(CODE_NO_VOICE, "no voice");
			continue;
		}

		RECORD_DEBUGE(TAG, "start record!");
		mcotton_rest_conn();
		xEventGroupClearBits(record_event_group, REC_DETECT_BIT);

		endTime = nowTime + REST_RECORD_TIMEOUT_MS;

		while(nowTime < endTime && record_cnt){
			
			sys_data_write(flash_wr_size, wave_buf, 2048);
		    flash_wr_size += 2048;

			if(Audio_i2sRead(wave_buf, 2048, portMAX_DELAY) <= 0){
		    	break;
		    }

			if(signal_check((int16_t *)wave_buf, 1024)){
				record_cnt = REST_RECORD_INVALID_CNT;
			}
			else{
				record_cnt--;
			}

		    nowTime = xTaskGetTickCount() * portTICK_PERIOD_MS;
		}

		RECORD_DEBUGE(TAG, "finish record!");
		free(wave_buf);

		waveHeader.data.ChunkSize = flash_wr_size - sizeof(waveHeader);
		waveHeader.riff.ChunkSize = flash_wr_size - 8;
		sys_data_set_size(flash_wr_size);

		mcotton_rest_send();
		xEventGroupSetBits(record_event_group, REC_POST_BIT);		
	}

	sys_data_free();
    vTaskDelete(NULL);
}


void rest_record_start(recordDone_callback_t record_cb)
{
	static I2S_Params_st i2s_params = {
    	.rate = SR_16KHZ,
		.wlen = I2S_16BIT,
		.channel = I2S_CHANNEL_FMT_ONLY_LEFT
    };

	recordDone_callback = record_cb;
    wav_record_init();
	xTaskCreatePinnedToCore(rest_record_task, "rest_record_task", 1792, (void*)&i2s_params, 5, &xrecordTask, 1);
	mcotton_rest_start(restPost_callback);
}


void rest_record_give()
{
	if(xrecordTask)
		xTaskNotifyGive(xrecordTask);
}


void wav_record_init()
{
	if(record_event_group)
        return;

    record_event_group = xEventGroupCreate();
	xEventGroupSetBits(record_event_group, REC_PLAY_BIT);	
}


void wav_record_deinit()
{
	if(record_event_group == NULL)
        return;

	wav_record_stop();
	xEventGroupWaitBits(record_event_group, REC_PLAY_BIT, pdTRUE, pdTRUE, portMAX_DELAY);	

	vEventGroupDelete(record_event_group);
	record_event_group = NULL;	
}


void wav_record_stop()
{
	if(record_event_group == NULL)
        return;

    xEventGroupSetBits(record_event_group, REC_STOP_BIT);
	xEventGroupWaitBits(record_event_group, REC_PLAY_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
}


Record_State_E record_get_state()
{
	if(record_event_group == NULL)
        return REC_STATE_STOP;

	if (xEventGroupGetBits(record_event_group) & REC_PLAY_BIT){
		return REC_STATE_STOP;
	}

	if (xEventGroupGetBits(record_event_group) & REC_DETECT_BIT){
		return REC_STATE_DETECT;
	}	

	if (xEventGroupGetBits(record_event_group) & REC_POST_BIT){
		return REC_STATE_POST;
	}

	return REC_STATE_START;	
}


void wav_record_start(FILE* f)
{
	xEventGroupWaitBits(record_event_group, REC_PLAY_BIT, pdTRUE, pdTRUE, portMAX_DELAY);
	xEventGroupClearBits(record_event_group, REC_STOP_BIT);
	xEventGroupSetBits(record_event_group, REC_DETECT_BIT);	

    xTaskCreatePinnedToCore(wav_record_task, "wav_record_task", 2048, (void*)f, 5, NULL, 0);
}


uint32_t wav_record_time()
{
    return record_time;
}
