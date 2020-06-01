#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "uartdev.h"
#include "wifi.h"
#include "audio.h"
#include "sysfat.h"
#include "button.h"
#include "wav_record.h"
#include "bt_speaker.h"
#include "ble_serial.h"
#include "aplay.h"
#include "mCotton.h"
#include "at_cmd.h"


#define TAG "AT:"

#define DEFAULT_FILE_NAME   "please input a file_name!"
#define DEFAULT_WAV_NAME    "please input a wav_name!"
#define DEFAULT_BT_NAME     "please input a bt_name!"
#define DEFAULT_MQTT_SERVER "mqtt://mCotton.microduino.cn"

#define DEFAULT_VOLUME      30

static char *bt_name;
static char play_file[64] = "/sdcard/";
static char wav_file[64] = "/sdcard/";
static uint8_t volume = DEFAULT_VOLUME;
static uint8_t baudrate = 0xFF;
static int dev_mode = MODE_BASE;
static uint8_t ble_mode = 0;
static int rest_code = 0;
static int rest_flag = 0;
static uint32_t test_flag = 0;
static esp_err_t fat_flag = ESP_FAIL;
static char *mqtt_ser;
static char *mqtt_cid;
static char *mqtt_usr;
static char *mqtt_pwd;

char* const play_name = play_file+8; 
char* const wav_name = wav_file+8; 


static bool isFnMp3(char* filename) 
{
    uint16_t len = strlen(filename);
    if(strstr(strlwr(filename + (len - 4)), ".mp3")) {
        return true;
    }
    return false;
}


static bool isFnWav(char* filename) 
{
    uint16_t len = strlen(filename);
    if(strstr(strlwr(filename + (len - 4)), ".wav")) {
        return true;
    }
    return false;
}


static bool isFnHttp(char *uri)
{
	return (!strncmp(strlwr(uri), "http:", strlen("http:")) || 
            !strncmp(strlwr(uri), "https:", strlen("https:")));
}



static void buttonRest_cb(uint8_t action)
{
	if(action == BUTTON_PRESS){
        if(dev_mode != MODE_WIFI)
            return;
    
        if(!wifi_isConnected())
            return;             

         if(rest_flag || aplay_get_state()==APLAY_STATE_REQUEST)
            return;

        rest_flag = 1;   
        wav_record_stop();
        aplay_stop();     
        rest_record_give();       	
	}
}


static void buttonA_cb(uint8_t action)
{
	if(action == BUTTON_PRESS){
        uart_send(_CRLF);
        uart_send(CMD_BTN);
        uart_send(":A\r\n");
	}
}


static void buttonB_cb(uint8_t action)
{
	if(action == BUTTON_PRESS){
        uart_send(_CRLF);
        uart_send(CMD_BTN);
        uart_send(":B\r\n");
	}
}


static void jack_dect_cb(uint8_t action)
{
	if(action == BUTTON_PRESS){
		record_setChannel(MIC_JACK);
	}else if(action == BUTTON_RELEASE){
		record_setChannel(MIC_BOARD);
	} 
}


void gattc_btnA_cb(uint8_t action)
{
	if(action == BUTTON_PRESS){
        uart_dev_baudrate(4);       
        gattc_set_baudrate(1152);  
        vTaskDelay(50 / portTICK_PERIOD_MS);      
        gattc_reset(RST_COMMAND);
	}
}


void gattc_btnB_cb(uint8_t action)
{
	if(action == BUTTON_PRESS){
        gattc_connection_close();
	}
}


static void restDone_cb(int code, const char* msg)
{
	ESP_LOGI(TAG, "err_code: %d", code);
    rest_code = code;
    char *send_buf;
    if(asprintf(&send_buf, "\r\n%s:%d,%s\r\n", CMD_RELT, code, msg) < 0){
    	ESP_LOGE(TAG, "send_buf malloc failed!");
		return;
    }
	uart_send(send_buf);
	free(send_buf);   
}


static void testA_cb(uint8_t action)
{
	if(action == BUTTON_PRESS){
        test_flag |= 0x0100;
	}else if(action == BUTTON_RELEASE){
		test_flag |= 0x0200;
	} 
}

void testB_cb(uint8_t action)
{
	if(action == BUTTON_PRESS){
        test_flag |= 0x0400;
	}else if(action == BUTTON_RELEASE){
		test_flag |= 0x0800;
	} 
}

void testJID_cb(uint8_t action)
{
	if(action == BUTTON_PRESS){
        test_flag |= 0x1000;
        record_setChannel(MIC_JACK);
	}else if(action == BUTTON_RELEASE){
		test_flag |= 0x2000;
        record_setChannel(MIC_BOARD);
	} 
}


static void modeSetup(int mode)
{    
    static int temp_mode = MODE_BASE;
    if(mode == temp_mode)
        return;

    if(mode == MODE_WIFI){
        Button_Cfg_st buttons[] = {
            {BUTTON_A_PIN, buttonRest_cb},
            {BUTTON_B_PIN, buttonB_cb},
            {JACK_DECT_PIN, jack_dect_cb}
        };

        bt_speaker_deinit();
        button_if_init(buttons, sizeof(buttons)/sizeof(buttons[0]));
        rest_record_start(restDone_cb);
        aplay_init();
        strcpy(play_name, DEFAULT_FILE_NAME);
        strcpy(wav_name, DEFAULT_WAV_NAME);         
    }else if(mode == MODE_BT){
        Button_Cfg_st buttons[] = {
            {BUTTON_A_PIN, buttonA_cb},
            {BUTTON_B_PIN, buttonB_cb},
        };
        wifi_sta_deinit();
        button_if_init(buttons, sizeof(buttons)/sizeof(buttons[0]));
        bt_name = malloc(64);
        strcpy(bt_name, DEFAULT_BT_NAME);
    }else if(mode == MODE_MQTT){
        Button_Cfg_st buttons[] = {
            {BUTTON_A_PIN, buttonA_cb},
            {BUTTON_B_PIN, buttonB_cb},
        };
        bt_speaker_deinit();
        button_if_init(buttons, sizeof(buttons)/sizeof(buttons[0]));
        mqtt_ser = malloc(64);
        mqtt_cid = malloc(50);
        mqtt_usr = malloc(50);
        mqtt_pwd = malloc(50);
        strcpy(mqtt_ser, DEFAULT_MQTT_SERVER);
    }else if(mode == MODE_BLE){
        wifi_sta_deinit();
        vTaskDelay(100 / portTICK_PERIOD_MS);
        ble_init();
    }else if(mode == MODE_TEST){
        Button_Cfg_st buttons[] = {
            {BUTTON_A_PIN, testA_cb},
            {BUTTON_B_PIN, testB_cb},
            {JACK_DECT_PIN, testJID_cb}
        };

        test_flag = (fat_flag==ESP_OK);
        bt_speaker_deinit();
        button_if_init(buttons, sizeof(buttons)/sizeof(buttons[0]));
        Audio_setSpeakerVolume(false, 12);
    }
    temp_mode = mode;
}


static bool parse_Baud(char* ptr)
{
    if(*ptr == '?'){
        char *send_buf;
        if(asprintf(&send_buf, "%s:%d\r\n", CMD_BAUD, uart_dev_getBaud()) < 0){	       
	        ESP_LOGE(TAG, "send_buf malloc failed!");
            return false;
        }

        uart_send(send_buf);
	    free(send_buf);	 
    }else if(*ptr == '='){
    	char *baud_tmp = strtok(ptr+1, "\r");

        uart_send(CMD_BAUD);  
        uart_send(":"); 

        if(baud_tmp == NULL){
            uart_send(ERR_PARAMS);    
            return false; 
        }

        if(*baud_tmp < '0' || *baud_tmp > '4'){
            uart_send(ERR_CMD);    
            return false; 
        }

        baudrate = baud_tmp[0] - '0';
        uart_sendlen(baud_tmp, 1);
        uart_send(_CRLF);
    }else{
        return false;
    }
    return true;
}


static bool parse_CWJAP(char* ptr)
{
    if(*ptr == '?'){
        wifi_ap_record_t ap_info;
        esp_err_t ret = esp_wifi_sta_get_ap_info(&ap_info);

        uart_send(CMD_CWJAP);
        uart_send(":");

        int rssi = 0;
        if(wifi_isConnected()){
            rssi = ap_info.rssi;           
        }

        if(ret == ESP_OK){
            char *send_buf;
            if(asprintf(&send_buf, "\"%s\",\"%x:%x:%x:%x:%x:%x\",%d\r\n", (char*)ap_info.ssid, \
                        ap_info.bssid[0],ap_info.bssid[1],ap_info.bssid[2],ap_info.bssid[3],ap_info.bssid[4],ap_info.bssid[5], \
                        rssi) < 0){	       
	            ESP_LOGE(TAG, "send_buf malloc failed!");
                return false;
            }

            uart_send(send_buf);
	        free(send_buf);	 
        }else{
            uart_send(ERR_WIFI);
            return false;
        }
    
    }else if(*ptr == '='){
    	char *ssid_tmp = strtok(ptr+1, ",");
	    char *pwd_tmp = strtok(NULL, "\r");    

        uart_send(CMD_CWJAP);  
        uart_send(":"); 

        if(ssid_tmp==NULL || pwd_tmp==NULL){
            uart_send(ERR_PARAMS);    
            return false; 
        }

        if(dev_mode == MODE_BT){
            uart_send(ERR_MODE); 
            return false;
        }

        char ssid_cfg[20] = "";
        char pwd_cfg[20] = "";

        strrpl(ssid_cfg, ssid_tmp, "\"", "");
        strrpl(pwd_cfg, pwd_tmp, "\"", "");

        wifi_set_config(ssid_cfg, pwd_cfg);
        uart_send("1\r\n");
    }else{
        return false;
    }
    return true;
}


static bool parse_Mode(char* ptr)
{
    if(*ptr == '?'){
        char *send_buf;
        if(asprintf(&send_buf, "%s:%d\r\n", CMD_MODE, dev_mode) < 0){
    	    ESP_LOGE(TAG, "send_buf malloc failed!");
		    return false;
        }

	    uart_send(send_buf);
	    free(send_buf);                
    }else if(*ptr == '='){
    	char *mode_tmp = strtok(++ptr, "\r");  

        uart_send(CMD_MODE); 
        uart_send(":");         

        if(mode_tmp==NULL || strlen(mode_tmp)>1){  
            uart_send(ERR_PARAMS);
            return false; 
        }

        if(dev_mode){
            uart_send(ERR_MODE);
            return false; 
        }

        dev_mode = atoi(mode_tmp);
        if(dev_mode <= MODE_BASE || dev_mode >= MODE_MAX){
            dev_mode = 0;
            uart_send(ERR_MODE);
            return false;
        }

        char *send_buf;
        if(asprintf(&send_buf, "%d\r\n", dev_mode) < 0){
    	    ESP_LOGE(TAG, "send_buf malloc failed!");
		    return false;
        }

	    uart_send(send_buf);
	    free(send_buf);    
    }else{
        return false;
    }
    return true;
}


static bool parse_Rest()
{
    uart_send(CMD_REST);  
    uart_send(":");

    if(dev_mode != MODE_WIFI){
        uart_send(ERR_MODE); 
        return false;
    }
    
    if(!wifi_isConnected()){
        uart_send(ERR_WIFI); 
        return false;             
    }

    if(rest_flag || aplay_get_state()==APLAY_STATE_REQUEST){
        uart_send(ERR_BUSY); 
        return false;
    }

    uart_send("1\r\n");
    rest_flag = 1;   
    wav_record_stop();
    aplay_stop();     
    rest_record_give(); 
    return true;
}


static bool parse_Play(char* ptr)
{
    if(*ptr == '?'){
        char *send_buf;
        if(asprintf(&send_buf, "%s:%d,\"%s\"\r\n", CMD_PLAY, aplay_get_state(), play_name) < 0){
    	    ESP_LOGE(TAG, "send_buf malloc failed!");
		    return false;
        }
	    uart_send(send_buf);
	    free(send_buf);     
             
    }else if(*ptr == '='){
    	char *play_cmd = strtok(ptr+1, ",");
	    char *play_tmp = strtok(NULL, "\r"); 

        uart_send(CMD_PLAY); 
        uart_send(":");        

        if(dev_mode != MODE_WIFI){
            uart_send(ERR_MODE); 
            return false;
        }

        if(rest_flag || aplay_get_state()==APLAY_STATE_REQUEST){
            uart_send(ERR_BUSY); 
            return false;
        }

        if(*play_cmd == '0'){
            if(play_tmp==NULL){
                uart_send(ERR_PARAMS);
                return false; 
            }

            wav_record_stop();
            strrpl(play_name, play_tmp, "\"", "");
            if(isFnHttp(play_name)){
                
                if(!wifi_isConnected()){ 
                    uart_send(ERR_WIFI); 
                    return false;             
                }
                aplay_stop();  
                web_aplay_start(play_name);
            
            }else if(isFnMp3(play_name)){
                
                FILE* f = fopen(play_file, "rb");
                if(f == NULL){
		            ESP_LOGE(TAG, "Failed to open file for reading");
                    uart_send(ERR_FILE);
                    return false; 
	            
                }
                aplay_stop();
                mp3_aplay_start(f);

            }else if(isFnWav(play_name)){
             
                FILE* f = fopen(play_file, "rb");
                if(f == NULL){
		            ESP_LOGE(TAG, "Failed to open file for reading");
                    uart_send(ERR_FILE);
                    return false; 
	            
                }
                aplay_stop(); 
                wav_aplay_start(f);

            }else{
                uart_send(ERR_NAME);
                return false; 
            }

        }else if(*play_cmd == '1'){
            aplay_suspend();
        }else if(*play_cmd == '2'){
            aplay_stop();
            strcpy(play_name, DEFAULT_FILE_NAME);
        }else{
            uart_send(ERR_CMD);
            return false; 
        }

        char *send_buf;
        if(asprintf(&send_buf, "%d\r\n", aplay_get_state()) < 0){
    	    ESP_LOGE(TAG, "send_buf malloc failed!");
		    return false;
        }

	    uart_send(send_buf);
	    free(send_buf);     
    }else{
        return false;
    }
    return true;
}


static bool parse_Bta(char* ptr)
{
    if(*ptr == '?'){
        char *send_buf;
        if(asprintf(&send_buf, "%s:%d,%d\r\n", CMD_BTA, bt_speaker_is_connected(), bt_speaker_is_started()) < 0){
    	    ESP_LOGE(TAG, "send_buf malloc failed!");
		    return false;
        }

	    uart_send(send_buf);
	    free(send_buf);               
    }else if(*ptr == '='){
    	char *bt_cmd = strtok(ptr+1, ",");
	    char *bt_tmp = strtok(NULL, "\r"); 

        uart_send(CMD_BTA); 
        uart_send(":");         

        if(dev_mode != MODE_BT){
            uart_send(ERR_MODE); 
            return false;
        }

        if(*bt_cmd == '0'){    
            if(bt_tmp==NULL){
                uart_send(ERR_PARAMS);
                return false; 
            }

            if(bt_speaker_is_init()){
                uart_send(ERR_BT);
                return false;             
            }

            strrpl(bt_name, bt_tmp, "\"", "");
            bt_speaker_init(bt_name);

        }else if(*bt_cmd == '1'){
            if(bt_speaker_is_started())
                bt_speaker_pause();
            else
                bt_speaker_play();

        }else if(*bt_cmd == '2'){
            bt_speaker_stop(); 
        
        }else if(*bt_cmd == '3'){
            bt_speaker_forward();    

        }else if(*bt_cmd == '4'){
            bt_speaker_backward();

        }else{
            uart_send(ERR_CMD);
            return false; 
        }

        char *send_buf;
        if(asprintf(&send_buf, "%d\r\n", bt_speaker_is_connected()) < 0){
    	    ESP_LOGE(TAG, "send_buf malloc failed!");
		    return false;
        }

	    uart_send(send_buf);
	    free(send_buf);   
    }else{
        return false;
    }
    return true;
}


static bool parse_Record(char* ptr)
{
    if(*ptr == '?'){
        char *send_buf;
        if(asprintf(&send_buf, "%s:%d\r\n", CMD_REC, wav_record_time()) < 0){
    	    ESP_LOGE(TAG, "send_buf malloc failed!");
		    return false;
        }

	    uart_send(send_buf);
	    free(send_buf);                
    }else if(*ptr == '='){
        char *wav_cmd = strtok(ptr+1, ",");
        char *wav_tmp = strtok(NULL, "\r"); 

        uart_send(CMD_REC);  
        uart_send(":");   

        if(dev_mode != MODE_WIFI){
            uart_send(ERR_MODE); 
            return false;
        }

        if(rest_flag || aplay_get_state()==APLAY_STATE_REQUEST){
            uart_send(ERR_BUSY); 
            return false;
        }

        if(*wav_cmd == '0'){
            if(wav_tmp==NULL){
                uart_send(ERR_PARAMS);
                return false; 
            }

            strrpl(wav_name, wav_tmp, "\"", "");
            if(!isFnWav(wav_name)){ 
                uart_send(ERR_NAME);
                return false; 
            }

            aplay_stop();
	        FILE* f = fopen(wav_file, "wb");
	        if(f == NULL){
		        ESP_LOGE(TAG, "Failed to open file for writing");
                uart_send(ERR_FILE);
                return false; 
	        }
            wav_record_stop();
            wav_record_start(f);

        }else if(*wav_cmd == '2'){
            wav_record_stop();    
        }else{
            uart_send(ERR_CMD);
            return false; 
        }

        char *send_buf;
        if(asprintf(&send_buf, "%d\r\n", record_get_state()) < 0){
    	    ESP_LOGE(TAG, "send_buf malloc failed!");
		    return false;
        }

	    uart_send(send_buf);
	    free(send_buf);   
    }else{
        return false;
    }
    return true;
}


static bool parse_MqttSer(char* ptr)
{
    if(*ptr == '?'){
        char *send_buf;
        if(asprintf(&send_buf, "%s:\"%s\"\r\n", CMD_MQSER, mqtt_ser) < 0){
    	    ESP_LOGE(TAG, "send_buf malloc failed!");
		    return false;
        }

	    uart_send(send_buf);
	    free(send_buf);                
    }else if(*ptr == '='){
        char *ser_tmp = strtok(ptr+1, "\r");

        uart_send(CMD_MQSER);  
        uart_send(":");   

        if(dev_mode != MODE_MQTT){
            uart_send(ERR_MODE); 
            return false;
        }

        strrpl(mqtt_ser, ser_tmp, "\"", "");
    }else{
        return false;
    }
    return true;
}


static bool parse_MqttCon(char* ptr)
{
    if(*ptr == '?'){
        char *send_buf;
        if(asprintf(&send_buf, "%s:%d\r\n", CMD_MQCON, mCotton_isConnected()) < 0){
    	    ESP_LOGE(TAG, "send_buf malloc failed!");
		    return false;
        }

	    uart_send(send_buf);
	    free(send_buf);                
    }else if(*ptr == '='){
        char *cid_tmp = strtok(ptr+1, ",");
        char *usr_tmp = strtok(NULL, ","); 
        char *pwd_tmp = strtok(NULL, "\r"); 

        uart_send(CMD_MQCON);  
        uart_send(":");   

        if(dev_mode != MODE_MQTT){
            uart_send(ERR_MODE); 
            return false;
        }

        if(!wifi_isConnected()){
            uart_send(ERR_WIFI); 
            return false;             
        }

        strrpl(mqtt_cid, cid_tmp, "\"", "");
        strrpl(mqtt_usr, usr_tmp, "\"", "");
        strrpl(mqtt_pwd, pwd_tmp, "\"", "");
        if(mCotton_init(mqtt_ser, mqtt_cid, mqtt_usr, mqtt_pwd) == ESP_OK)
            uart_send("1\r\n");
        else 
            uart_send("0\r\n"); 
    }else{
        return false;
    }
    return true;
}


static bool parse_MqttSub(char* ptr)
{            
    if(*ptr == '='){
        char *sub_cmd = strtok(ptr+1, ",");
        char *sub_tmp = strtok(NULL, "\r");

        uart_send(CMD_MQSUB);  
        uart_send(":");   

        if(dev_mode != MODE_MQTT){
            uart_send(ERR_MODE); 
            return false;
        }

        if(!mCotton_isConnected()){
            uart_send(ERR_SERVER); 
            return false;             
        }

        if(sub_tmp==NULL){
            uart_send(ERR_PARAMS);
            return false; 
        }

        char mqtt_sub[50] = "";
        strrpl(mqtt_sub, sub_tmp, "\"", "");
        if(*sub_cmd == '0'){
            mCotton_unsubscribe(mqtt_sub);
        }else if(*sub_cmd == '1'){
            mCotton_subscribe(mqtt_sub);
        }else{
            uart_send(ERR_CMD);
            return false; 
        }

	    uart_send("1\r\n");
    }else{
        return false;
    }
    return true;
}


static bool parse_MqttPub(char* ptr)
{
    if(*ptr == '='){
        char *pub_tmp = strtok(ptr+1, ",");
        char *msg_tmp = strtok(NULL, "\r");

        uart_send(CMD_MQPUB);  
        uart_send(":");   

        if(dev_mode != MODE_MQTT){
            uart_send(ERR_MODE); 
            return false;
        }

        if(!mCotton_isConnected()){
            uart_send(ERR_SERVER); 
            return false;             
        }

        char mqtt_pub[50] = "";
        strrpl(mqtt_pub, pub_tmp, "\"", "");
        mCotton_publish(mqtt_pub, msg_tmp);

	    uart_send("1\r\n");  
    }else{
        return false;
    }
    return true;
}


static bool parse_MqttQuery(char* ptr)
{
    if(*ptr == '?'){
        int len = mCotton_available();
	    char *send_buf;
        if(asprintf(&send_buf, "%s:%d,", CMD_MQQER, len) < 0){
    	    ESP_LOGE(TAG, "send_buf malloc failed!");
		    return false;
        }

	    uart_send(send_buf);
	    free(send_buf);
    
        if(len > 0)
            uart_sendlen(mCotton_read(), len);
        else
            uart_send("none");

        uart_send(_CRLF);
    }else{
        return false;
    }
    return true;
}


static bool parse_Ble(char* ptr)
{
    if(*ptr == '?'){
        char *send_buf;
        if(asprintf(&send_buf, "%s:%d\r\n", CMD_BLE, ble_mode) < 0){
    	    ESP_LOGE(TAG, "send_buf malloc failed!");
		    return false;
        }

	    uart_send(send_buf);
	    free(send_buf);                
    }else if(*ptr == '='){
        char *ble_cmd = strtok(ptr+1, "\r");

        uart_send(CMD_BLE);  
        uart_send(":");   

        if(dev_mode != MODE_BLE){
            uart_send(ERR_MODE); 
            return false;
        }

        if(ble_cmd==NULL){
            uart_send(ERR_PARAMS);
            return false; 
        }

        if(ble_mode){
            uart_send(ERR_PARAMS);
            return false; 
        }

        ble_mode = atoi(ble_cmd);
        if(ble_mode <= 0 || ble_mode > 2){
            ble_mode = 0;
            uart_send(ERR_CMD);
            return false;
        }

        if(ble_mode == 1){
            gatts_init();
        }else if(ble_mode == 2){
            gattc_init();
            Button_Cfg_st buttons[] = {
                {BUTTON_A_PIN, gattc_btnA_cb},
                {BUTTON_B_PIN, gattc_btnB_cb},
            };
            button_if_init(buttons, sizeof(buttons)/sizeof(buttons[0]));           
        }

        uart_sendlen(ble_cmd, 1);
	    uart_send(_CRLF);
    }else{
        return false;
    }
    return true;
}


static bool parse_Vol(char* ptr)
{
    if(*ptr == '?'){
	    char *send_buf;
        if(asprintf(&send_buf, "%s:%d\r\n", CMD_VOL, volume) < 0){
    	    ESP_LOGE(TAG, "send_buf malloc failed!");
		    return false;
        }

	    uart_send(send_buf);
	    free(send_buf);            
    }else if(*ptr == '='){
    	char *vol_tmp = strtok(++ptr, "\r");  

        uart_send(CMD_VOL); 
        uart_send(":");         

        if(vol_tmp==NULL || strlen(vol_tmp)>2){  
            uart_send(ERR_PARAMS);
            return false; 
        }

        if(*ptr == '+'){
            volume++;
        }else if(*ptr == '-'){
            if(volume)
                volume--;
        }else{
            volume = atoi(vol_tmp);
        }

        if(volume > 63)
            volume = 63;

        Audio_setEarphoneVolume((volume == 0), volume);
        Audio_setSpeakerVolume((volume == 0), volume);

        char *send_buf;
        if(asprintf(&send_buf, "%d\r\n", volume) < 0){
    	    ESP_LOGE(TAG, "send_buf malloc failed!");
		    return false;
        }

	    uart_send(send_buf);
	    free(send_buf);           
    }else{
        return false;
    }
    return true;
}


static bool parse_Version(char* ptr)
{
    if(*ptr == '?'){
	    char *send_buf;
        if(asprintf(&send_buf, "%s:%d.%d.%d\r\n", CMD_VER, VER_MAJOR, VER_MINOR, VER_SUB) < 0){
    	    ESP_LOGE(TAG, "send_buf malloc failed!");
		    return false;
        }

	    uart_send(send_buf);
	    free(send_buf);                  
    }else{
        return false;
    }
    return true;
}


static bool parse_ram(char* ptr)
{
    if(*ptr == '?'){
	    char *send_buf;
        if(asprintf(&send_buf, "%s:%d\r\n", CMD_RAM, esp_get_free_heap_size()) < 0){
    	    ESP_LOGE(TAG, "send_buf malloc failed!");
		    return false;
        }

        uart_send(send_buf);
        free(send_buf);           
    }else{
        return false;
    }    
    return true;
}


static bool parse_test(char* ptr)
{
    if(*ptr == '?'){
	    char *send_buf;
        if(asprintf(&send_buf, "%s:%d\r\n", CMD_TEST, test_flag) < 0){
    	    ESP_LOGE(TAG, "send_buf malloc failed!");
		    return false;
        }

        uart_send(send_buf);
        free(send_buf);           
    }else{
        return false;
    }    
    return true;
}


static void cmd_parse(char* cmd, int len)
{ 
    ESP_LOGI(TAG, "pattern pos: %d", len);
    ESP_LOGI(TAG, "read data: %s", cmd);
    if(strncmp(cmd, CMD_AT, strlen(CMD_AT))){
        return;
    }   
    cmd += strlen(CMD_AT);
    len -= strlen(CMD_AT);

    uart_send(_CRLF);

    bool ret = false;

    if(len == 0){
        ret = true;
    }else{
        if(strncmp(cmd, CMD_RST, strlen(CMD_RST)) == 0){
            uart_send(RESP_OK);
            esp_restart();
        }else if(strncmp(cmd, CMD_BAUD, strlen(CMD_BAUD)) == 0){
            ret = parse_Baud(cmd+strlen(CMD_BAUD));
    
        }else if(strncmp(cmd, CMD_CWJAP, strlen(CMD_CWJAP)) == 0){
            ret = parse_CWJAP(cmd+strlen(CMD_CWJAP));

        }else if(strncmp(cmd, CMD_MODE, strlen(CMD_MODE)) == 0){
            ret = parse_Mode(cmd+strlen(CMD_MODE));

        }else if(strncmp(cmd, CMD_REST, strlen(CMD_REST)) == 0){
            ret = parse_Rest();

        }else if(strncmp(cmd, CMD_PLAY, strlen(CMD_PLAY)) == 0){
            ret = parse_Play(cmd+strlen(CMD_PLAY));

        }else if(strncmp(cmd, CMD_BTA, strlen(CMD_BTA)) == 0){
            ret = parse_Bta(cmd+strlen(CMD_BTA));

        }else if(strncmp(cmd, CMD_REC, strlen(CMD_REC)) == 0){
            ret = parse_Record(cmd+strlen(CMD_REC));

        }else if(strncmp(cmd, CMD_MQSER, strlen(CMD_MQSER)) == 0){
            ret = parse_MqttSer(cmd+strlen(CMD_MQSER));

        }else if(strncmp(cmd, CMD_MQCON, strlen(CMD_MQCON)) == 0){
            ret = parse_MqttCon(cmd+strlen(CMD_MQCON));

        }else if(strncmp(cmd, CMD_MQSUB, strlen(CMD_MQSUB)) == 0){
            ret = parse_MqttSub(cmd+strlen(CMD_MQSUB));
    
        }else if(strncmp(cmd, CMD_MQPUB, strlen(CMD_MQPUB)) == 0){
            ret = parse_MqttPub(cmd+strlen(CMD_MQPUB));

        }else if(strncmp(cmd, CMD_MQQER, strlen(CMD_MQQER)) == 0){
            ret = parse_MqttQuery(cmd+strlen(CMD_MQQER));

        }else if(strncmp(cmd, CMD_BLE, strlen(CMD_BLE)) == 0){
            ret = parse_Ble(cmd+strlen(CMD_BLE));

        }else if(strncmp(cmd, CMD_VOL, strlen(CMD_VOL)) == 0){
            ret = parse_Vol(cmd+strlen(CMD_VOL));

        }else if(strncmp(cmd, CMD_VER, strlen(CMD_VER)) == 0){
            ret = parse_Version(cmd+strlen(CMD_VER));

        }else if(strncmp(cmd, CMD_RAM, strlen(CMD_RAM)) == 0){
            ret = parse_ram(cmd+strlen(CMD_RAM));

        }else if(strncmp(cmd, CMD_TEST, strlen(CMD_TEST)) == 0){
            ret = parse_test(cmd+strlen(CMD_TEST));
        }

            
    }

    if(ret){
        uart_send(RESP_OK);
        modeSetup(dev_mode);
        if(baudrate < 0xFF){
            uart_dev_baudrate(baudrate);
            baudrate = 0xFF;
        }
    }else
        uart_send(RESP_ERROR);      
}


void at_cmd_init(int pin_tx, int pin_rx)
{ 
    uart_dev_init(pin_tx, pin_rx, cmd_parse);
    fat_flag = sys_fat_init();
    wifi_sta_init();

	I2S_Params_st i2s_params = {
    	.rate = SR_16KHZ,
		.wlen = I2S_16BIT,
		.channel = I2S_CHANNEL_FMT_RIGHT_LEFT
    };
	Audio_i2s_init(&i2s_params);
    Audio_setEarphoneSpeaker(volume);
	record_setChannel(MIC_BOARD);

    uart_send(RESP_READY);
}


Ui_Step_E at_process()
{
    if(dev_mode == MODE_TEST){
        test_flag |=  (wifi_isConnected()<<1);
        char audio_buf[128];
        Audio_i2sRead(audio_buf, 128, portMAX_DELAY);
        for(int i=0; i<128; i+=2){
            Audio_i2sWrite(audio_buf+i, 2, 1000 / portTICK_RATE_MS);  
            Audio_i2sWrite(audio_buf+i, 2, 1000 / portTICK_RATE_MS);  
        }
        return UI_FACTORY;
    }else if(dev_mode == MODE_WIFI){
        
        if(!wifi_isConnected()){
            return UI_WIFI_CONNECTING;
        }else{
            Aplay_State_E aplay_state = aplay_get_state();
            Record_State_E record_state = record_get_state();

            if(rest_code){
                vTaskDelay(2000 / portTICK_PERIOD_MS);
                rest_flag = 0;
                if(rest_code == 200){
                    rest_code = 0;                        
                    return UI_REST_OK;
                }else{
                    rest_code = 0;
                    return UI_REST_ERROR;
                }  
            }else if(record_state == REC_STATE_POST){
                return UI_RECORD_POST;
            }else if(record_state == REC_STATE_START){
                return UI_RECORD_START;
            }else if(record_state == REC_STATE_DETECT){
                return UI_RECORD_DETECT;
            }else if(aplay_state == APLAY_STATE_SUSPEND){
                return UI_APLY_SUSPEND;
            }else if(aplay_state == APLAY_STATE_START){
                return UI_APLAY_START;
            }else if(aplay_state == APLAY_STATE_REQUEST){
                return UI_APLAY_REQUEST;
            }else{
                return UI_WIFI_CONNECTED;
            }
        }
    }else if(dev_mode == MODE_BT){

        if(!bt_speaker_is_init()){
            return UI_BT_INIT;
        }else if(!bt_speaker_is_connected()){
            return UI_BT_CONNECTING;
        }else{
            return UI_BT_CONNECTED;
        }
     }else if(dev_mode == MODE_MQTT){

        if(!wifi_isConnected()){
            return UI_WIFI_CONNECTING;  
        }else if(mCotton_isConnected()){
            return UI_MQTT_CONNECTED;            
        }else{
            return UI_WIFI_CONNECTED;
        }
     }else if(dev_mode == MODE_BLE){

        if(ble_isConnected()){
            return UI_BLE_CONNECTED;           
        }else{
            return UI_BLE_CONNECTING;
        }
    }
    return UI_IDLE;
}
