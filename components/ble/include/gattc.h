#ifndef _GATTC_H_
#define _GATTC_H_

#include "gatt_defs.h"

esp_err_t gattc_init();
bool gattc_isConnected();
void gattc_connection_close();
void gattc_reset(uint8_t _data);
void gattc_set_baudrate(uint16_t baud);
void gattc_trans_send(uint8_t *_data, int _len);
void gattc_trans_notify(uint16_t enable);


#endif