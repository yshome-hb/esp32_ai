#ifndef _APLAY_H_
#define _APLAY_H_


#define APLAY_DEBUG

#ifdef APLAY_DEBUG
	#define APLAY_DEBUGI( tag, format, ... )		ESP_LOGI( tag, format, ##__VA_ARGS__)
	#define APLAY_DEBUGW( tag, format, ... )		ESP_LOGW( tag, format, ##__VA_ARGS__)
	#define APLAY_DEBUGE( tag, format, ... )		ESP_LOGE( tag, format, ##__VA_ARGS__)
#else
	#define APLAY_DEBUGI( tag, format, ... )
	#define APLAY_DEBUGW( tag, format, ... )
	#define APLAY_DEBUGE( tag, format, ... )
#endif


typedef enum 
{
    APLAY_STATE_STOP = 0,  
    APLAY_STATE_REQUEST,  
    APLAY_STATE_START,           
    APLAY_STATE_SUSPEND,

} Aplay_State_E;


void aplay_init();
void aplay_deinit();
void aplay_suspend();
void aplay_stop();
Aplay_State_E aplay_get_state();
void mp3_aplay_start(FILE* f);
void wav_aplay_start(FILE* f);
void web_aplay_start(char* web_url);

#endif
