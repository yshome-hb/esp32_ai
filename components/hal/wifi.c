
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_event_loop.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "wifi.h"


#define TAG "wifi:"

#define STA_CONNECTED_BIT   BIT0
#define STA_GOTIP_BIT       BIT1


static EventGroupHandle_t station_event_group;//24bit
static wifi_config_t sta_config;


static esp_err_t net_event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:// station start
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED: //station disconnect from ap
        esp_wifi_connect();
        xEventGroupClearBits(station_event_group, STA_CONNECTED_BIT);
        xEventGroupClearBits(station_event_group, STA_GOTIP_BIT);
        break;
    case SYSTEM_EVENT_STA_CONNECTED: //station connect to ap
    	xEventGroupSetBits(station_event_group, STA_CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_GOT_IP:  //station get ip
    	ESP_LOGI(TAG, "got ip:%s\n",
		ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
    	xEventGroupSetBits(station_event_group, STA_GOTIP_BIT);
        break;
    case SYSTEM_EVENT_SCAN_DONE:
        // uint16_t apCount = 0;
        // esp_wifi_scan_get_ap_num(&apCount);
        // if (apCount == 0) {
        //     BLUFI_INFO("Nothing AP found");
        //     break;
        // }
        // wifi_ap_record_t *ap_list = (wifi_ap_record_t *)malloc(sizeof(wifi_ap_record_t) * apCount);
        // if (!ap_list) {
        //     BLUFI_ERROR("malloc error, ap_list is NULL");
        //     break;
        // }
        // ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&apCount, ap_list));
        // esp_blufi_ap_record_t * blufi_ap_list = (esp_blufi_ap_record_t *)malloc(apCount * sizeof(esp_blufi_ap_record_t));
        // if (!blufi_ap_list) {
        //     if (ap_list) {
        //         free(ap_list);
        //     }
        //     BLUFI_ERROR("malloc error, blufi_ap_list is NULL");
        //     break;
        // }
        // for (int i = 0; i < apCount; ++i)
        // {
        //     blufi_ap_list[i].rssi = ap_list[i].rssi;
        //     memcpy(blufi_ap_list[i].ssid, ap_list[i].ssid, sizeof(ap_list[i].ssid));
        // }
        // esp_blufi_send_wifi_list(apCount, blufi_ap_list);
        // esp_wifi_scan_stop();
        // free(ap_list);
        // free(blufi_ap_list);
        break;
    default:
        break;
    }
    return ESP_OK;
}


void wifi_sta_init()
{
    tcpip_adapter_init();
    station_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(net_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_start() );
}


void wifi_sta_deinit()
{
    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_deinit());
}


void wifi_set_config(const char* cfg_ssid, const char* cfg_pwd)
{
    strcpy((char *)sta_config.sta.ssid, cfg_ssid);
    strcpy((char *)sta_config.sta.password, cfg_pwd);

    esp_wifi_disconnect();
    esp_wifi_set_config(ESP_IF_WIFI_STA, &sta_config);
    esp_wifi_connect();
}


bool wifi_isConnected()
{
    EventBits_t uxBits = xEventGroupGetBits(station_event_group);

    return ((uxBits&STA_GOTIP_BIT) == STA_GOTIP_BIT);
}


tcpip_adapter_ip_info_t wifi_waitConnect()
{
    xEventGroupWaitBits(station_event_group, STA_GOTIP_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    tcpip_adapter_ip_info_t ip;
    memset(&ip, 0, sizeof(tcpip_adapter_ip_info_t));
    if (tcpip_adapter_get_ip_info(ESP_IF_WIFI_STA, &ip) == 0) {
        ESP_LOGI(TAG, "~~~~~~~~~~~");
        ESP_LOGI(TAG, "ETHIP:"IPSTR, IP2STR(&ip.ip));
        ESP_LOGI(TAG, "ETHPMASK:"IPSTR, IP2STR(&ip.netmask));
        ESP_LOGI(TAG, "ETHPGW:"IPSTR, IP2STR(&ip.gw));
        ESP_LOGI(TAG, "~~~~~~~~~~~");
    }
    return ip;
}
