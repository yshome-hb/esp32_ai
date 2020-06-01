#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "mCotton.h"


#define TAG "mCotton:"


typedef struct{
    int len;
    char *data;

}Mqtt_Data_st;

static esp_mqtt_client_handle_t client;
static bool mqtt_connected = false;
static Mqtt_Data_st mqtt_data;


static esp_err_t mqtt_event_handler(esp_mqtt_event_handle_t event)
{
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
			mqtt_connected = true;
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            break;
        case MQTT_EVENT_DISCONNECTED:
			mqtt_connected = false;;
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
            //printf("DATA=%.*s\r\n", event->data_len, event->data);
            mqtt_data.len = event->data_len;
            mqtt_data.data = event->data;
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
    }
    return ESP_OK;
}


int mCotton_available()
{
    return mqtt_data.len;
}


char* mCotton_read()
{
    mqtt_data.len = 0;
    return mqtt_data.data;
}


esp_err_t mCotton_init(const char *uri, const char *cid, const char *usr, const char *pwd)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = uri,
        .client_id = cid,
        .username = usr,
        .password = pwd,
        .event_handle = mqtt_event_handler,
        // .user_context = (void *)your_context
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    if(client == NULL)
        return ESP_FAIL;
    
    return esp_mqtt_client_start(client);
}


bool mCotton_isConnected()
{
    return mqtt_connected;
}


void mCotton_subscribe(const char *topic)
{
    int msg_id = esp_mqtt_client_subscribe(client, topic, 0);
    ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);	
}


void mCotton_unsubscribe(const char *topic)
{
    int msg_id = esp_mqtt_client_unsubscribe(client, topic);
    ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);	
}


void mCotton_publish(const char *topic, const char *msg)
{
    int msg_id = esp_mqtt_client_publish(client, topic, msg, 0, 0, 0);
    ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);	
}