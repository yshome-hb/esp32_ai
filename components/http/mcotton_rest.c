#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "cJSON.h"
#include "http_client.h"
#include "mcotton_rest.h"

#define TAG "REST:"

#define MICRODUINO_URL	"https://mproxy.microduino.cn/baidu/asr"

#define CODE_OK			200
#define CODE_NOT_FOUND	404

#define REST_CONNECT_BIT   	BIT0
#define REST_SEND_BIT  		BIT1


typedef struct{
	uint16_t index;
	char data[512];
}Parse_Body_st;

static TaskHandle_t xrestTask = NULL;
static EventGroupHandle_t rest_event_group = NULL;//24bit
static restDone_callback_t restDone_callback = NULL;
static Parse_Body_st parse_body = {0};

static int body_callback(http_parser* a, const char *at, size_t length)
{
	if(parse_body.index + length > 512){
		length = 512 - parse_body.index;
	}
	memcpy(parse_body.data + parse_body.index, at, length);
	parse_body.index += length;
    return 0;
}


static int body_done_callback (http_parser* a)
{	  
	parse_body.data[parse_body.index] = '\0';
    parse_body.index = 0;

    cJSON *root = cJSON_Parse(parse_body.data);
    int ecode = cJSON_GetObjectItem(root, "errorCode")->valueint;

	char* res_msg = NULL;
    if(ecode == CODE_OK){
    	cJSON *data= cJSON_GetObjectItem(root, "data");
		if(cJSON_HasObjectItem(data, "err_no")){
			ecode = cJSON_GetObjectItem(data, "err_no")->valueint;
			asprintf(&res_msg, "invalid result");
		}else{
			res_msg = cJSON_Print(data);
		}
    }else{
		asprintf(&res_msg, "http connect error");
	}
	
	xTaskNotifyGive(xrestTask);
	restDone_callback(ecode, res_msg);
	free(res_msg);
    cJSON_Delete(root);
    return 0;
}


static http_parser_settings settings =
{   .on_message_begin = 0,
    .on_header_field = 0,
    .on_header_value = 0,
    .on_url = 0,
    .on_status = 0,
    .on_body = body_callback,
    .on_headers_complete = 0,
    .on_message_complete = body_done_callback,
    .on_chunk_header = 0,
    .on_chunk_complete = 0
};


static void mcotton_rest_task(void* pvParameters)
{
	url_t *url = url_parse(MICRODUINO_URL);
	if(url == NULL){
		HTTPC_DEBUGE(TAG, "Could not parse URI %s", MICRODUINO_URL);
	    vTaskDelete(NULL);
	}

	char* form_data_buf[6] = {NULL};
	// form_data_buf[0] = NULL;
	// form_data_buf[0] = form_data_create("uid", "1122334455");
	// form_data_buf[1] = form_data_create("format", "wav");
	// form_data_buf[2] = form_data_create("rate", "8000");
	// form_data_buf[3] = form_data_create("lan", "zh");
	// form_data_buf[4] = form_data_create("per", "4");
	// form_data_buf[5] = NULL;

 	struct esp_tls *tls = NULL;

	for(;;){
		xEventGroupWaitBits(rest_event_group, REST_CONNECT_BIT, pdTRUE, pdTRUE, portMAX_DELAY);	

		tls = http_tls_conn(url);	

		xEventGroupWaitBits(rest_event_group, REST_SEND_BIT, pdTRUE, pdTRUE, portMAX_DELAY);	

    	if(tls == NULL){
			HTTPC_DEBUGE(TAG, "Connection failed...");
			restDone_callback(CODE_NOT_FOUND, "can not attach to server");
			continue;	
		}

		HTTPC_DEBUGI(TAG, "Connection established...");

		if(http_rest_post(url, tls, USER_AGENT, (const char**)form_data_buf, sys_data_get(), &settings) != ESP_OK){
			HTTPC_DEBUGE(TAG, "Http rest post failed!");
			restDone_callback(CODE_NOT_FOUND, "can not attach to server");
		}

		http_tls_free(tls); 	
	   	sys_data_erase();
	}

	url_free(url);
	http_tls_free(tls); 	
	form_data_free(form_data_buf);
    vTaskDelete(NULL);
}


void mcotton_rest_start(restDone_callback_t resDone_cb)
{
	restDone_callback = resDone_cb;
	rest_event_group = xEventGroupCreate();
	xTaskCreatePinnedToCore(mcotton_rest_task, "mcotton_rest_task", 4352, NULL, 4, &xrestTask, 0);
}


void mcotton_rest_conn()
{
	xEventGroupSetBits(rest_event_group, REST_CONNECT_BIT);
}


void mcotton_rest_send()
{
	xEventGroupSetBits(rest_event_group, REST_SEND_BIT);
}

