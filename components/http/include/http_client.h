#ifndef __HTTPCLIENT_H__
#define __HTTPCLIENT_H__

#include "esp_tls.h"
#include "http_parser.h"
#include "url_parser.h"
#include "sysfat.h"


#define HTTP_CLIENT_DEBUG

#ifdef HTTP_CLIENT_DEBUG
	#define HTTPC_DEBUGI( tag, format, ... )		ESP_LOGI( tag, format, ##__VA_ARGS__)
	#define HTTPC_DEBUGW( tag, format, ... )		ESP_LOGW( tag, format, ##__VA_ARGS__)
	#define HTTPC_DEBUGE( tag, format, ... )		ESP_LOGE( tag, format, ##__VA_ARGS__)
#else
	#define HTTPC_DEBUGI( tag, format, ... )
	#define HTTPC_DEBUGW( tag, format, ... )
	#define HTTPC_DEBUGE( tag, format, ... )
#endif


#define USER_AGENT		"User-Agent: MicroAi/1.0.2 Microduino"


char* form_data_create(const char* name, char* content);
void form_data_free(char** form_data);
esp_tls_t *http_tls_conn(url_t* url);
void http_tls_free(esp_tls_t* tls);
esp_err_t http_client_post(const char* uri, const char* headers, const char* post_data, http_parser_settings* http_callback);
esp_err_t http_client_get(const char* uri, const char* headers, http_parser_settings* http_callback);
esp_err_t http_client_delete(const char* uri, const char* headers, const char* post_data, http_parser_settings* http_callback);
esp_err_t http_client_put(const char* uri, const char* headers, const char* post_data, http_parser_settings* http_callback);
esp_err_t http_rest_post(url_t* url, struct esp_tls *tls, const char* headers, const char** form_data, Fat_Data_st* fat_data, http_parser_settings* http_callback);


#endif // __HTTPCLIENT_H__
