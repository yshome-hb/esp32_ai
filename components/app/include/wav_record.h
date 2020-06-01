#ifndef _WAV_RECORD_H_
#define _WAV_RECORD_H_

#define RECORD_DEBUG

#ifdef RECORD_DEBUG
	#define RECORD_DEBUGI( tag, format, ... )		ESP_LOGI( tag, format, ##__VA_ARGS__)
	#define RECORD_DEBUGW( tag, format, ... )		ESP_LOGW( tag, format, ##__VA_ARGS__)
	#define RECORD_DEBUGE( tag, format, ... )		ESP_LOGE( tag, format, ##__VA_ARGS__)
#else
	#define RECORD_DEBUGI( tag, format, ... )
	#define RECORD_DEBUGW( tag, format, ... )
	#define RECORD_DEBUGE( tag, format, ... )
#endif


typedef enum {
	MIC_JACK = 0, 
    MIC_BOARD,   
}Mic_Type_E;


typedef enum {
    REC_STATE_STOP = 0,   
	REC_STATE_DETECT,   
    REC_STATE_START,      
    REC_STATE_POST,    
} Record_State_E;


typedef void (*recordDone_callback_t)    (int code, const char* msg);

void record_setChannel(Mic_Type_E mic);
void rest_record_start(recordDone_callback_t record_cb);
void rest_record_give();
void wav_record_init();
void wav_record_deinit();
void wav_record_start(FILE* f);
void wav_record_stop();
Record_State_E record_get_state();
uint32_t wav_record_time();


#endif





