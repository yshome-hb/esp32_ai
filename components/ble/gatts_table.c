#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "uartdev.h"
#include "gatts_table.h"


#define TAG "GATTS"

#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define ESP_APP_ID                  0x55
#define SVC_INST_ID                 0

#define ADV_CONFIG_FLAG             (1 << 0)
#define SCAN_RSP_CONFIG_FLAG        (1 << 1)

static uint8_t adv_config_done = 0;
static bool gatts_connected  = false; 
static bool trans_notification_enabled = false;

static uint8_t service_uuid[16] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    //first uuid, 16bit, [12],[13] is the value
    0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0xf0, 0xff, 0x00, 0x00,
};

/* The length of adv data must be less than 31 bytes */
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp        = false,
    .include_name        = true,
    .include_txpower     = false,
    .min_interval        = 0x00,
    .max_interval        = 0x00,
    .appearance          = 0x00,
    .manufacturer_len    = 0,    //TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //test_manufacturer,
    .service_data_len    = 0,
    .p_service_data      = NULL,
    .service_uuid_len    = sizeof(service_uuid),
    .p_service_uuid      = service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};


static esp_ble_adv_params_t adv_params = {
    .adv_int_min         = MIN_ADV_INTERVAL,
    .adv_int_max         = MAX_ADV_INTERVAL,
    .adv_type            = ADV_TYPE_IND,
    .own_addr_type       = BLE_ADDR_TYPE_PUBLIC,
    .channel_map         = ADV_CHNL_ALL,
    .adv_filter_policy   = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

static struct gatts_profile_inst gatts_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    }
};

/* Attributes State Machine */
enum
{
    IDX_SVC,

    IDX_CHAR_TRANS,
    IDX_CHAR_VAL_TRANS,
    IDX_CHAR_CFG_TRANS,
    IDX_CHAR_DESC_TRANS,

    IDX_CHAR_RST,
    IDX_CHAR_VAL_RST,
    IDX_CHAR_DESC_RST,

    IDX_CHAR_BAUD,
    IDX_CHAR_VAL_BAUD,
    IDX_CHAR_DESC_BAUD,

    HRS_IDX_NB,
};

static uint16_t mSerial_handle_table[HRS_IDX_NB];

/* Service */
static const uint16_t service_uuid_serial = GATT_SERVICE_UUID_SERIAL;
static const uint16_t char_uuid_trans     = GATT_CHAR_UUID_TRANS;
static const uint16_t char_uuid_rst       = GATT_CHAR_UUID_RST;
static const uint16_t char_uuid_baud      = GATT_CHAR_UUID_BAUD;

static const uint16_t primary_service_uuid      = ESP_GATT_UUID_PRI_SERVICE;
static const uint16_t char_declaration_uuid     = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t char_client_config_uuid   = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
static const uint16_t char_description_uuid     = ESP_GATT_UUID_CHAR_DESCRIPTION;
static const uint8_t char_prop_writenr          = ESP_GATT_CHAR_PROP_BIT_WRITE_NR;
static const uint8_t char_prop_writenr_notify   = ESP_GATT_CHAR_PROP_BIT_WRITE_NR | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t trans_char_ccc[2]          = {0x00, 0x00};

/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t gatt_db[HRS_IDX_NB] =
{
    // Service Declaration
    [IDX_SVC]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(service_uuid_serial), (uint8_t *)&service_uuid_serial}},

    /* Characteristic Declaration */
    [IDX_CHAR_TRANS]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_declaration_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_writenr_notify}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_TRANS] =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_uuid_trans, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      GATT_CHAR_VAL_LEN_MAX, 0, NULL}},

    /* Client Characteristic Configuration Descriptor */
    [IDX_CHAR_CFG_TRANS]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(trans_char_ccc), (uint8_t *)trans_char_ccc}},

    [IDX_CHAR_DESC_TRANS]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_description_uuid, ESP_GATT_PERM_READ,
      strlen("trans"), strlen("trans"), (uint8_t *)"trans"}},  

    /* Characteristic Declaration */
    [IDX_CHAR_RST]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_declaration_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_writenr}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_RST]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_uuid_rst, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      1, 0, NULL}},

    [IDX_CHAR_DESC_RST]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_description_uuid, ESP_GATT_PERM_READ,
      strlen("Reset"), strlen("Reset"), (uint8_t *)"Reset"}},  

    /* Characteristic Declaration */
    [IDX_CHAR_BAUD]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_declaration_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t), sizeof(uint8_t), (uint8_t *)&char_prop_writenr}},

    /* Characteristic Value */
    [IDX_CHAR_VAL_BAUD]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_uuid_baud, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      2, 0, NULL}},

    [IDX_CHAR_DESC_BAUD]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&char_description_uuid, ESP_GATT_PERM_READ,
      strlen("Baudrate"), strlen("Baudrate"), (uint8_t *)"Baudrate"}},  

};


static void mSerial_trans_handler(uint8_t *_data, uint16_t _size);
static void mSerial_rst_handler(uint8_t _data);
static void mSerial_baud_handler(uint16_t _data);

static void gatts_on_write_event(esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    if (mSerial_handle_table[IDX_CHAR_CFG_TRANS] == param->write.handle && param->write.len == 2){
        uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
        if (descr_value == 0x0001){
            ESP_LOGI(TAG, "notify enable"); 
            trans_notification_enabled = true;
        }else if (descr_value == 0x0000){
            ESP_LOGI(TAG, "notify disable ");
            trans_notification_enabled = false;
        }else{
            ESP_LOGE(TAG, "unknown descr value");
            esp_log_buffer_hex(TAG, param->write.value, param->write.len);
        }

    }else if (mSerial_handle_table[IDX_CHAR_VAL_RST] == param->write.handle && param->write.len == 1){ 
        mSerial_rst_handler(param->write.value[0]);
    }else if (mSerial_handle_table[IDX_CHAR_VAL_BAUD] == param->write.handle && param->write.len == 2){ 
        uint16_t write_value = param->write.value[1]<<8 | param->write.value[0];
        mSerial_baud_handler(write_value);
    }else if (mSerial_handle_table[IDX_CHAR_VAL_TRANS] == param->write.handle){  
        mSerial_trans_handler(param->write.value, param->write.len);
    }
}


static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {
        case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~ADV_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
            adv_config_done &= (~SCAN_RSP_CONFIG_FLAG);
            if (adv_config_done == 0){
                esp_ble_gap_start_advertising(&adv_params);
            }
            break;
        case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
            /* advertising start complete event to indicate advertising start successfully or failed */
            if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "advertising start failed");
            }else{
                ESP_LOGI(TAG, "advertising start successfully");
            }
            break;
        case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
            if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
                ESP_LOGE(TAG, "Advertising stop failed");
            }
            else {
                ESP_LOGI(TAG, "Stop adv successfully\n");
            }
            break;
        case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
            ESP_LOGI(TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
                  param->update_conn_params.status,
                  param->update_conn_params.min_int,
                  param->update_conn_params.max_int,
                  param->update_conn_params.conn_int,
                  param->update_conn_params.latency,
                  param->update_conn_params.timeout);
            break;
        default:
            break;
    }
}


static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT:{
            char bt_name[15];
            uint8_t bt_mac[6] = {0};
            esp_read_mac(bt_mac, ESP_MAC_BT);
            sprintf(bt_name, MAI_DEVICE_NAME"%02X%02X", bt_mac[4], bt_mac[5]);
            esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(bt_name);
            if (set_dev_name_ret){
                ESP_LOGE(TAG, "set device name failed, error code = %x", set_dev_name_ret);
            }
            //config adv data
            esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
            if (ret){
                ESP_LOGE(TAG, "config adv data failed, error code = %x", ret);
            }
            adv_config_done |= ADV_CONFIG_FLAG;
            esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, HRS_IDX_NB, SVC_INST_ID);
            if (create_attr_ret){
                ESP_LOGE(TAG, "create attr table failed, error code = %x", create_attr_ret);
            }
        }
       	    break;
        case ESP_GATTS_READ_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_READ_EVT");
       	    break;
        case ESP_GATTS_WRITE_EVT:
            ESP_LOGI(TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d, gatts_if %d", param->write.conn_id, param->write.trans_id, param->write.handle, gatts_if);
            ESP_LOGI(TAG, "GATT_WRITE_EVT, value len %d, value %32x", param->write.len, *(uint32_t *)param->write.value);
            gatts_on_write_event(gatts_if, param);
      	    break;
        case ESP_GATTS_EXEC_WRITE_EVT: 
            ESP_LOGI(TAG, "ESP_GATTS_EXEC_WRITE_EVT");
            break;
        case ESP_GATTS_MTU_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
            break;
        case ESP_GATTS_CONF_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_CONF_EVT, status = %d", param->conf.status);
            break;
        case ESP_GATTS_START_EVT:
            ESP_LOGI(TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
            break;
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
            esp_log_buffer_hex(TAG, param->connect.remote_bda, 6);
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            /* For the IOS system, please reference the apple official documents about the ble connection parameters restrictions. */
            conn_params.latency = 0;
            conn_params.min_int = MIN_CONN_INTERVAL;
            conn_params.max_int = MAX_CONN_INTERVAL;
            conn_params.timeout = CONN_SUP_TIMEOUT;    
            //start sent the update connection parameters to the peer device.
            esp_ble_gap_update_conn_params(&conn_params);
            gatts_connected = true;
            uart_dev_intr_enable(gatts_trans_send);
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(TAG, "ESP_GATTS_DISCONNECT_EVT, reason = %d", param->disconnect.reason);
            gatts_connected = false;
            uart_dev_intr_disable();
            esp_ble_gap_start_advertising(&adv_params);
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
            if (param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            }
            else if (param->add_attr_tab.num_handle != HRS_IDX_NB){
                ESP_LOGE(TAG, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to HRS_IDX_NB(%d)", param->add_attr_tab.num_handle, HRS_IDX_NB);
            }
            else {
                ESP_LOGI(TAG, "create attribute table successfully, the number handle = %d\n",param->add_attr_tab.num_handle);
                memcpy(mSerial_handle_table, param->add_attr_tab.handles, sizeof(mSerial_handle_table));
                esp_ble_gatts_start_service(mSerial_handle_table[IDX_SVC]);
            }            
            break;
        }
        case ESP_GATTS_STOP_EVT:
        case ESP_GATTS_OPEN_EVT:
        case ESP_GATTS_CANCEL_OPEN_EVT:
        case ESP_GATTS_CLOSE_EVT:
        case ESP_GATTS_LISTEN_EVT:
        case ESP_GATTS_CONGEST_EVT:
        case ESP_GATTS_UNREG_EVT:
        case ESP_GATTS_DELETE_EVT:
        default:
            break;
    }
}


static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gatts_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
        } else {
            ESP_LOGE(TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == gatts_profile_tab[idx].gatts_if) {
                if (gatts_profile_tab[idx].gatts_cb) {
                    gatts_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}


static void mSerial_trans_handler(uint8_t *_data, uint16_t _size)
{
	ESP_LOGI(TAG, "trans data: %s", (char*)_data);
    uart_sendlen((char*)_data, _size);
}


static void mSerial_rst_handler(uint8_t _data)
{
    if(_data == RST_COMMAND){
		ESP_LOGI(TAG, "RESET");	
	}
}


static void mSerial_baud_handler(uint16_t _data)
{	
	uint8_t baudrate = 0;	
	switch(_data){	
		case 96:
			baudrate = 0;
			break;	
		case 192:
			baudrate = 1;
			break;
		case 384:
			baudrate = 2;
			break;	
		case 576:
			baudrate = 3;
			break;	
		case 1152:
			baudrate = 4;
			break;	
		default:
			return;
	}
	ESP_LOGI(TAG, "setbaudrate: %d", baudrate);
    uart_dev_baudrate(baudrate);
}


esp_err_t gatts_init()
{
    esp_err_t ret;

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        ESP_LOGE(TAG, "gatts register error, error code = %x", ret);
        return ret;
    }

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret){
        ESP_LOGE(TAG, "gap register error, error code = %x", ret);
        return ret;
    }

    ret = esp_ble_gatts_app_register(ESP_APP_ID);
    if (ret){
        ESP_LOGE(TAG, "gatts app register error, error code = %x", ret);
    }
    return ret;
}


bool inline gatts_isConnected()
{
    return gatts_connected;
}


void gatts_trans_send(uint8_t *_data, int _len)
{
	if(!gatts_connected || !trans_notification_enabled)
        return;

    esp_ble_gatts_send_indicate(gatts_profile_tab[PROFILE_APP_IDX].gatts_if,
								gatts_profile_tab[PROFILE_APP_IDX].conn_id,
								mSerial_handle_table[IDX_CHAR_VAL_TRANS],
								_len, _data, false);
}
