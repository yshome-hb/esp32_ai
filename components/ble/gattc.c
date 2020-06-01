#include <string.h>
#include "esp_gap_ble_api.h"
#include "esp_gattc_api.h"
#include "esp_log.h"
#include "uartdev.h"
#include "gattc.h"

#define TAG "GATTC"

#define PROFILE_NUM      1
#define PROFILE_APP_IDX  0
#define INVALID_HANDLE   0


static esp_bt_uuid_t serial_filter_service_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = GATT_SERVICE_UUID_SERIAL,},
};

static esp_bt_uuid_t trans_filter_char_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = GATT_CHAR_UUID_TRANS,},
};

static esp_bt_uuid_t rst_filter_char_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = GATT_CHAR_UUID_RST,},
};

static esp_bt_uuid_t baud_filter_char_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = GATT_CHAR_UUID_BAUD,},
};

static esp_bt_uuid_t notify_descr_uuid = {
    .len = ESP_UUID_LEN_16,
    .uuid = {.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG,},
};

static esp_ble_scan_params_t ble_scan_params = {
    .scan_type              = BLE_SCAN_TYPE_ACTIVE,
    .own_addr_type          = BLE_ADDR_TYPE_PUBLIC,
    .scan_filter_policy     = BLE_SCAN_FILTER_ALLOW_ALL,
    .scan_interval          = 0x50,
    .scan_window            = 0x30,
    .scan_duplicate         = BLE_SCAN_DUPLICATE_DISABLE
};

struct gattc_profile_inst {
    esp_gattc_cb_t gattc_cb;
    uint16_t gattc_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_start_handle;
    uint16_t service_end_handle;
    uint16_t trans_handle;
    uint16_t trans_cccd_handle;
    uint16_t rst_handle;
    uint16_t baud_handle;
    esp_bd_addr_t remote_bda;
};

static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);
/* One gatt-based profile one app_id and one gattc_if, this array will store the gattc_if returned by ESP_GATTS_REG_EVT */
static struct gattc_profile_inst gattc_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gattc_cb = gattc_profile_event_handler,
        .gattc_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

static bool gattc_connected = false;
static bool get_server = false;

static void gattc_profile_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    esp_ble_gattc_cb_param_t *p_data = (esp_ble_gattc_cb_param_t *)param;

    switch (event) {
    case ESP_GATTC_REG_EVT:
        ESP_LOGI(TAG, "REG_EVT");
        esp_err_t scan_ret = esp_ble_gap_set_scan_params(&ble_scan_params);
        if (scan_ret){
            ESP_LOGE(TAG, "set scan params error, error code = %x", scan_ret);
        }
        break;
    case ESP_GATTC_CONNECT_EVT:
        gattc_connected = true;
        uart_dev_intr_enable(gattc_trans_send);
        ESP_LOGI(TAG, "ESP_GATTC_CONNECT_EVT conn_id %d, if %d", p_data->connect.conn_id, gattc_if);
        gattc_profile_tab[PROFILE_APP_IDX].conn_id = p_data->connect.conn_id;
        memcpy(gattc_profile_tab[PROFILE_APP_IDX].remote_bda, p_data->connect.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(TAG, "REMOTE BDA:");
        esp_log_buffer_hex(TAG, gattc_profile_tab[PROFILE_APP_IDX].remote_bda, sizeof(esp_bd_addr_t));
        esp_err_t mtu_ret = esp_ble_gattc_send_mtu_req (gattc_if, p_data->connect.conn_id);
        if (mtu_ret){
            ESP_LOGE(TAG, "config MTU error, error code = %x", mtu_ret);
        }
        break;
    case ESP_GATTC_OPEN_EVT:
        if (param->open.status != ESP_GATT_OK){
            ESP_LOGE(TAG, "open failed, status %d", p_data->open.status);
            break;
        }
        ESP_LOGI(TAG, "open success");
        break;
    case ESP_GATTC_CFG_MTU_EVT:
        if (param->cfg_mtu.status != ESP_GATT_OK){
            ESP_LOGE(TAG,"config mtu failed, error status = %x", param->cfg_mtu.status);
        }
        ESP_LOGI(TAG, "ESP_GATTC_CFG_MTU_EVT, Status %d, MTU %d, conn_id %d", param->cfg_mtu.status, param->cfg_mtu.mtu, param->cfg_mtu.conn_id);
        esp_ble_gattc_search_service(gattc_if, param->cfg_mtu.conn_id, &serial_filter_service_uuid);
        break;
    case ESP_GATTC_SEARCH_RES_EVT: {
        ESP_LOGI(TAG, "SEARCH RES: conn_id = %x is primary service %d", p_data->search_res.conn_id, p_data->search_res.is_primary);
        ESP_LOGI(TAG, "start handle %d end handle %d current handle value %d", p_data->search_res.start_handle, p_data->search_res.start_handle, p_data->search_res.srvc_id.inst_id);
        if (p_data->search_res.srvc_id.uuid.len == ESP_UUID_LEN_16 && p_data->search_res.srvc_id.uuid.uuid.uuid16 == serial_filter_service_uuid.uuid.uuid16) {
            ESP_LOGI(TAG, "service found");
            get_server = true;
            gattc_profile_tab[PROFILE_APP_IDX].service_start_handle = p_data->search_res.start_handle;
            gattc_profile_tab[PROFILE_APP_IDX].service_end_handle = p_data->search_res.end_handle;
            ESP_LOGI(TAG, "UUID16: %x", p_data->search_res.srvc_id.uuid.uuid.uuid16);
        }
        break;
    }
    case ESP_GATTC_SEARCH_CMPL_EVT:
        if (p_data->search_cmpl.status != ESP_GATT_OK){
            ESP_LOGE(TAG, "search service failed, error status = %x", p_data->search_cmpl.status);
            break;
        }
        ESP_LOGI(TAG, "ESP_GATTC_SEARCH_CMPL_EVT");
        if (get_server){
            uint16_t count = 0;
            esp_gatt_status_t status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                     p_data->search_cmpl.conn_id,
                                                                     ESP_GATT_DB_CHARACTERISTIC,
                                                                     gattc_profile_tab[PROFILE_APP_IDX].service_start_handle,
                                                                     gattc_profile_tab[PROFILE_APP_IDX].service_end_handle,
                                                                     INVALID_HANDLE,
                                                                     &count);
            if (status != ESP_GATT_OK){
                ESP_LOGE(TAG, "esp_ble_gattc_get_attr_count error");
            }

            if (count > 0){
                esp_gattc_char_elem_t char_elem_result;
                status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                        p_data->search_cmpl.conn_id,
                                                        gattc_profile_tab[PROFILE_APP_IDX].service_start_handle,
                                                        gattc_profile_tab[PROFILE_APP_IDX].service_end_handle,
                                                        trans_filter_char_uuid,
                                                        &char_elem_result,
                                                        &count);
                if (status == ESP_GATT_OK){
                    gattc_profile_tab[PROFILE_APP_IDX].trans_handle = char_elem_result.char_handle;
                    if(char_elem_result.properties & ESP_GATT_CHAR_PROP_BIT_NOTIFY){
                        esp_ble_gattc_register_for_notify (gattc_if, gattc_profile_tab[PROFILE_APP_IDX].remote_bda, char_elem_result.char_handle);
                    }
                }

                status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                        p_data->search_cmpl.conn_id,
                                                        gattc_profile_tab[PROFILE_APP_IDX].service_start_handle,
                                                        gattc_profile_tab[PROFILE_APP_IDX].service_end_handle,
                                                        rst_filter_char_uuid,
                                                        &char_elem_result,
                                                        &count);

                if (status == ESP_GATT_OK){
                    gattc_profile_tab[PROFILE_APP_IDX].rst_handle = char_elem_result.char_handle;
                }

                status = esp_ble_gattc_get_char_by_uuid( gattc_if,
                                                        p_data->search_cmpl.conn_id,
                                                        gattc_profile_tab[PROFILE_APP_IDX].service_start_handle,
                                                        gattc_profile_tab[PROFILE_APP_IDX].service_end_handle,
                                                        baud_filter_char_uuid,
                                                        &char_elem_result,
                                                        &count);

                if (status == ESP_GATT_OK){
                    gattc_profile_tab[PROFILE_APP_IDX].baud_handle = char_elem_result.char_handle;
                }

            }else{
                ESP_LOGE(TAG, "no char found");
            }
        }
        break;
    case ESP_GATTC_REG_FOR_NOTIFY_EVT: {
        ESP_LOGI(TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT");
        if (p_data->reg_for_notify.status != ESP_GATT_OK){
            ESP_LOGE(TAG, "REG FOR NOTIFY failed: error status = %d", p_data->reg_for_notify.status);
        }else{
            uint16_t count = 0;
            esp_gatt_status_t ret_status = esp_ble_gattc_get_attr_count( gattc_if,
                                                                         gattc_profile_tab[PROFILE_APP_IDX].conn_id,
                                                                         ESP_GATT_DB_DESCRIPTOR,
                                                                         gattc_profile_tab[PROFILE_APP_IDX].service_start_handle,
                                                                         gattc_profile_tab[PROFILE_APP_IDX].service_end_handle,
                                                                         gattc_profile_tab[PROFILE_APP_IDX].trans_handle,
                                                                         &count);
            if (ret_status != ESP_GATT_OK){
                ESP_LOGE(TAG, "esp_ble_gattc_get_attr_count error");
            }
            if (count > 0){
                esp_gattc_descr_elem_t descr_elem_result;    
                ret_status = esp_ble_gattc_get_descr_by_char_handle( gattc_if,
                                                                    gattc_profile_tab[PROFILE_APP_IDX].conn_id,
                                                                    p_data->reg_for_notify.handle,
                                                                    notify_descr_uuid,
                                                                    &descr_elem_result,
                                                                    &count);
                if (ret_status == ESP_GATT_OK){
                    gattc_profile_tab[PROFILE_APP_IDX].trans_cccd_handle = descr_elem_result.handle;
                }  
                gattc_trans_notify(true);
            }else{
                ESP_LOGE(TAG, "decsr not found");
            }
        }
        break;
    }
    case ESP_GATTC_NOTIFY_EVT:
        if (p_data->notify.is_notify){
            ESP_LOGI(TAG, "ESP_GATTC_NOTIFY_EVT, receive notify value:");
        }else{
            ESP_LOGI(TAG, "ESP_GATTC_NOTIFY_EVT, receive indicate value:");
        }
        uart_sendlen((char*)p_data->notify.value, p_data->notify.value_len);
        break;
    case ESP_GATTC_WRITE_DESCR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(TAG, "write descr failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(TAG, "write descr success ");
        break;
    case ESP_GATTC_SRVC_CHG_EVT: {
        esp_bd_addr_t bda;
        memcpy(bda, p_data->srvc_chg.remote_bda, sizeof(esp_bd_addr_t));
        ESP_LOGI(TAG, "ESP_GATTC_SRVC_CHG_EVT, bd_addr:");
        esp_log_buffer_hex(TAG, bda, sizeof(esp_bd_addr_t));
        break;
    }
    case ESP_GATTC_WRITE_CHAR_EVT:
        if (p_data->write.status != ESP_GATT_OK){
            ESP_LOGE(TAG, "write char failed, error status = %x", p_data->write.status);
            break;
        }
        ESP_LOGI(TAG, "write char success ");
        break;
    case ESP_GATTC_DISCONNECT_EVT:
        gattc_connected = false;
        uart_dev_intr_disable();
        ESP_LOGI(TAG, "ESP_GATTC_DISCONNECT_EVT, reason = %d", p_data->disconnect.reason);
        esp_ble_gattc_close (gattc_if, gattc_profile_tab[PROFILE_APP_IDX].conn_id);
        break;
    case ESP_GATTC_CLOSE_EVT:
        get_server = false;
        ESP_LOGI(TAG, "ESP_GATTC_CLOSE_EVT, reason = %d", p_data->close.reason);
        esp_ble_gap_start_scanning(0);
        break;
    default:
        break;
    }
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    uint8_t *adv_name = NULL;
    uint8_t adv_name_len = 0;
    uint8_t *adv_uuid = NULL;
    uint8_t adv_uuid_len = 0;
    switch (event) {
    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        //the unit of the duration is second
        esp_ble_gap_start_scanning(0);
        break;
    }
    case ESP_GAP_BLE_SCAN_START_COMPLETE_EVT:
        //scan start complete event to indicate scan start successfully or failed
        if (param->scan_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
            ESP_LOGE(TAG, "scan start failed, error status = %x", param->scan_start_cmpl.status);
            break;
        }
        ESP_LOGI(TAG, "scan start success");
        break;
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        esp_ble_gap_cb_param_t *scan_result = (esp_ble_gap_cb_param_t *)param;
        switch (scan_result->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT:
            esp_log_buffer_hex(TAG, scan_result->scan_rst.bda, 6);
            ESP_LOGI(TAG, "searched Adv Data Len %d, Scan Response Len %d", scan_result->scan_rst.adv_data_len, scan_result->scan_rst.scan_rsp_len);
            ESP_LOGI(TAG, "ssid %d", scan_result->scan_rst.rssi);
            if(scan_result->scan_rst.rssi < SCAN_RSSI){
                break;
            }
            
            adv_name = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);
            if(adv_name_len < (strlen(MAI_DEVICE_NAME)+4) || strncmp((char *)adv_name, MAI_DEVICE_NAME, strlen(MAI_DEVICE_NAME))){
                break;
            }

            adv_uuid = esp_ble_resolve_adv_data(scan_result->scan_rst.ble_adv,
                                                ESP_BLE_AD_TYPE_16SRV_CMPL, &adv_uuid_len);

            if(adv_uuid_len != 2 || adv_uuid[0] != 0xF0 || adv_uuid[1] != 0xFF){
                break;
            }                   

            ESP_LOGI(TAG, "connect to the mCookie device.");
            esp_ble_gap_stop_scanning();
            esp_ble_gattc_open(gattc_profile_tab[PROFILE_APP_IDX].gattc_if, scan_result->scan_rst.bda, scan_result->scan_rst.ble_addr_type, true);
            break;
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            break;
        default:
            break;
        }
        break;
    }

    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT:
        if (param->scan_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(TAG, "scan stop failed, error status = %x", param->scan_stop_cmpl.status);
            break;
        }
        ESP_LOGI(TAG, "stop scan successfully");
        break;

    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS){
            ESP_LOGE(TAG, "adv stop failed, error status = %x", param->adv_stop_cmpl.status);
            break;
        }
        ESP_LOGI(TAG, "stop adv successfully");
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

static void gattc_event_handler(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param)
{
    /* If event is register event, store the gattc_if for each profile */
    if (event == ESP_GATTC_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            gattc_profile_tab[param->reg.app_id].gattc_if = gattc_if;
        } else {
            ESP_LOGI(TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }

    /* If the gattc_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            if (gattc_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                    gattc_if == gattc_profile_tab[idx].gattc_if) {
                if (gattc_profile_tab[idx].gattc_cb) {
                    gattc_profile_tab[idx].gattc_cb(event, gattc_if, param);
                }
            }
        }
    } while (0);
}

esp_err_t gattc_init()
{
    esp_err_t ret;

    //register the  callback function to the gap module
    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret){
        ESP_LOGE(TAG, "%s gap register failed, error code = %x\n", __func__, ret);
        return ret;
    }

    //register the callback function to the gattc module
    ret = esp_ble_gattc_register_callback(gattc_event_handler);
    if(ret){
        ESP_LOGE(TAG, "%s gattc register failed, error code = %x\n", __func__, ret);
        return ret;
    }

    ret = esp_ble_gattc_app_register(PROFILE_APP_IDX);
    if (ret){
        ESP_LOGE(TAG, "%s gattc app register failed, error code = %x\n", __func__, ret);
    }
    return ret;
}


bool inline gattc_isConnected()
{
    return gattc_connected;
}


void gattc_connection_close()
{
	if(!gattc_connected)
        return;   

    esp_ble_gattc_close (gattc_profile_tab[PROFILE_APP_IDX].gattc_if, 
                         gattc_profile_tab[PROFILE_APP_IDX].conn_id);     
}


void gattc_reset(uint8_t _data)
{
	if(!gattc_connected)
        return;   
    
    esp_ble_gattc_write_char(gattc_profile_tab[PROFILE_APP_IDX].gattc_if,
                            gattc_profile_tab[PROFILE_APP_IDX].conn_id,
                            gattc_profile_tab[PROFILE_APP_IDX].rst_handle,
                            1,
                            &_data,
                            ESP_GATT_WRITE_TYPE_NO_RSP,
                            ESP_GATT_AUTH_REQ_NONE);        
}


void gattc_set_baudrate(uint16_t baud)
{
	if(!gattc_connected)
        return;      

    esp_ble_gattc_write_char(gattc_profile_tab[PROFILE_APP_IDX].gattc_if,
                            gattc_profile_tab[PROFILE_APP_IDX].conn_id,
                            gattc_profile_tab[PROFILE_APP_IDX].baud_handle,
                            2,
                            (uint8_t *)&baud,
                            ESP_GATT_WRITE_TYPE_NO_RSP,
                            ESP_GATT_AUTH_REQ_NONE);        
}


void gattc_trans_send(uint8_t *_data, int _len)
{
	if(!gattc_connected)
        return;      
    
    esp_ble_gattc_write_char(gattc_profile_tab[PROFILE_APP_IDX].gattc_if,
                            gattc_profile_tab[PROFILE_APP_IDX].conn_id,
                            gattc_profile_tab[PROFILE_APP_IDX].trans_handle,
                            (uint16_t)_len,
                            _data,
                            ESP_GATT_WRITE_TYPE_NO_RSP,
                            ESP_GATT_AUTH_REQ_NONE);        
}


void gattc_trans_notify(uint16_t enable)
{
	if(!gattc_connected)
        return;    

    esp_ble_gattc_write_char_descr(gattc_profile_tab[PROFILE_APP_IDX].gattc_if,
                                gattc_profile_tab[PROFILE_APP_IDX].conn_id,
                                gattc_profile_tab[PROFILE_APP_IDX].trans_cccd_handle,
                                2,
                                (uint8_t *)&enable,
                                ESP_GATT_WRITE_TYPE_RSP,
                                ESP_GATT_AUTH_REQ_NONE);   
}

