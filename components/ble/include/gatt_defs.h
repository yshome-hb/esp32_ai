#ifndef _GATT_DEFS_H_
#define _GATT_DEFS_H_


#define GATT_CHAR_VAL_LEN_MAX       20

#define MAI_DEVICE_NAME             "mCookie"

#define SCAN_RSSI					-50

#define MIN_ADV_INTERVAL            0x20 
#define MAX_ADV_INTERVAL            0x40    

#define MIN_CONN_INTERVAL           0x10      // min_int = 0x10*1.25ms = 20ms
#define MAX_CONN_INTERVAL           0x20      // max_int = 0x20*1.25ms = 40ms
#define CONN_SUP_TIMEOUT            400       // timeout = 400*10ms = 4000ms

#define GATT_SERVICE_UUID_SERIAL    0xFFF0
#define GATT_CHAR_UUID_TRANS        0xFFF6
#define GATT_CHAR_UUID_RST          0xFFF1
#define GATT_CHAR_UUID_BAUD         0xFFF2

#define RST_COMMAND				    0xA5


#endif