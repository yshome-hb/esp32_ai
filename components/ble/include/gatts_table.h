#ifndef _GATTS_TABLE_H_
#define _GATTS_TABLE_H_

#include "gatt_defs.h"

esp_err_t gatts_init();
bool gatts_isConnected();
void gatts_trans_send(uint8_t *_data, int _len);


#endif