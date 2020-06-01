#ifndef _WIFI_H_
#define _WIFI_H_


void wifi_sta_init();
void wifi_sta_deinit();
void wifi_set_config(const char* ssid, const char* pwd);
bool wifi_isConnected();
tcpip_adapter_ip_info_t wifi_waitConnect();


#endif
