#ifndef _I2CDEV_H_
#define _I2CDEV_H_

#include "driver/i2c.h"

void I2C_init(uint8_t pinSda, uint8_t pinScl, uint32_t i2cFreq);
void I2C_deinit();
esp_err_t I2C_readBit(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint8_t *data);
esp_err_t I2C_readBitW(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint16_t *data);
esp_err_t I2C_readBits(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t *data);
esp_err_t I2C_readBitsW(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint16_t *data);
esp_err_t I2C_readByte(uint8_t devAddr, uint8_t regAddr, uint8_t *data);
esp_err_t I2C_readWord(uint8_t devAddr, uint8_t regAddr, uint16_t *data);
esp_err_t I2C_readBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t *data);
esp_err_t I2C_writeCmd(uint8_t devAddr, uint8_t command);
esp_err_t I2C_readWords(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint16_t *data);
esp_err_t I2C_writeBit(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint8_t data);
esp_err_t I2C_writeBitW(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint16_t data);
esp_err_t I2C_writeBits(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t data);
esp_err_t I2C_writeBitsW(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint16_t data);
esp_err_t I2C_writeByte(uint8_t devAddr, uint8_t regAddr, uint8_t data);
esp_err_t I2C_writeWord(uint8_t devAddr, uint8_t regAddr, uint16_t data);
esp_err_t I2C_writeBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t* data);
esp_err_t I2C_writeWords(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint16_t *data);

#endif /* _I2CDEV_H_ */
