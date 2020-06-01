#ifndef _MP3_DECODE_H_
#define _MP3_DECODE_H_

#define MP3_DECODE_DEBUG

#ifdef MP3_DECODE_DEBUG
	#define DECODE_DEBUGI( tag, format, ... )		ESP_LOGI( tag, format, ##__VA_ARGS__)
	#define DECODE_DEBUGW( tag, format, ... )		ESP_LOGW( tag, format, ##__VA_ARGS__)
	#define DECODE_DEBUGE( tag, format, ... )		ESP_LOGE( tag, format, ##__VA_ARGS__)
#else
	#define DECODE_DEBUGI( tag, format, ... )
	#define DECODE_DEBUGW( tag, format, ... )
	#define DECODE_DEBUGE( tag, format, ... )
#endif

esp_err_t decode_start();
void decode_init();
void decode_deinit();
void decode_suspend(bool state);
void decode_destroy();
esp_err_t decode_send(const char *send_buf, size_t send_size, TickType_t ticks_to_wait);


#endif
