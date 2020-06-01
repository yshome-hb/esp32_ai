#ifndef _AT_CMD_H_
#define _AT_CMD_H_

#define VER_MAJOR       1
#define VER_MINOR       0
#define VER_SUB         5

#define BUTTON_A_PIN    36
#define BUTTON_B_PIN    37
#define JACK_DECT_PIN   38


#define FROMMEM(x)      ((const char *)(x))

#define RESP_OK         FROMMEM("OK\r\n")
#define RESP_ERROR      FROMMEM("ERROR\r\n")
#define RESP_READY      FROMMEM("ready\r\n")
#define _CRLF           FROMMEM("\r\n")

#define ERR_PARAMS      FROMMEM("INVALID PARAMS!\r\n")
#define ERR_MODE        FROMMEM("INVALID MODE!\r\n")
#define ERR_WIFI        FROMMEM("WIFI NOT ATTACHED!\r\n")
#define ERR_SERVER      FROMMEM("SERVER NOT ATTACHED!\r\n")
#define ERR_BT          FROMMEM("BT ALREADY SET!\r\n")
#define ERR_BUSY        FROMMEM("BUSY...\r\n")
#define ERR_CMD         FROMMEM("INVALID COMMAND!\r\n")
#define ERR_FILE        FROMMEM("INVALID FILE!\r\n")
#define ERR_NAME        FROMMEM("INVALID NAME!\r\n")
#define ERR_INTERNAL    FROMMEM("INVALID INTERNAL!\r\n")

#define CMD_AT          "AT"
#define CMD_RST         "+RST"
#define CMD_BAUD        "+BAUD"
#define CMD_CWJAP       "+CWJAP"
#define CMD_MODE        "+MODE"
#define CMD_REST        "+REST"
#define CMD_RELT        "+RELT"
#define CMD_PLAY        "+PLAY"
#define CMD_BTA         "+BTA"
#define CMD_REC         "+REC"
#define CMD_MQSER       "+MQSER"
#define CMD_MQCON       "+MQCON"
#define CMD_MQSUB       "+MQSUB"
#define CMD_MQPUB       "+MQPUB"
#define CMD_MQQER       "+MQQER"
#define CMD_BLE         "+BLE"
#define CMD_VOL         "+VOL"
#define CMD_VER         "+VERS"
#define CMD_RAM         "+FRAM"
#define CMD_BTN         "+BTN"
#define CMD_TEST        "+TEST"


typedef enum{
    MODE_BASE = 0,
    MODE_WIFI,
    MODE_BT,
    MODE_MQTT,
    MODE_BLE,
    MODE_TEST,
    MODE_MAX,
}Dev_Mode_E;

typedef enum {
    UI_IDLE = 0,   
    UI_BT_INIT,
    UI_BT_CONNECTING,
    UI_BT_CONNECTED,   
    UI_BT_PLAYING,
    UI_BT_PAUSE,
	UI_WIFI_CONNECTING,   
    UI_WIFI_CONNECTED,
    UI_APLAY_REQUEST,
    UI_APLAY_START,
    UI_APLY_SUSPEND,
    UI_RECORD_DETECT,
    UI_RECORD_START,
    UI_RECORD_POST,
    UI_REST_ERROR,
    UI_REST_OK,
    UI_MQTT_CONNECTED,
    UI_BLE_CONNECTING,
    UI_BLE_CONNECTED,
    UI_FACTORY,

} Ui_Step_E;

void at_cmd_init(int pin_tx, int pin_rx);

Ui_Step_E at_process();

#endif /* _AT_CMD_H_ */