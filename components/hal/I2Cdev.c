#include "I2Cdev.h"

#define WRITE_BIT  I2C_MASTER_WRITE /*!< I2C master write */
#define READ_BIT   I2C_MASTER_READ  /*!< I2C master read */
#define ACK_CHECK_EN    0x1         /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS   0x0         /*!< I2C master will not check ack from slave */
#define ACK_VAL         0x0         /*!< I2C ack value */
#define NACK_VAL        0x1         /*!< I2C nack value */

#define I2C_NUM			I2C_NUM_0

#define I2C_TX_BUF_DISABLE	0
#define I2C_RX_BUF_DISABLE	0

/**
 * @brief i2c master initialization
 */
void I2C_init(uint8_t pinSda, uint8_t pinScl, uint32_t i2cFreq)
{
    int i2c_master_port = I2C_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = pinSda;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = pinScl;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = i2cFreq;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode,
    				I2C_TX_BUF_DISABLE,
					I2C_TX_BUF_DISABLE, 0);
}

/**
 * @brief i2c master deinit
 */
void I2C_deinit()
{
    i2c_driver_delete(I2C_NUM);
}

/** Read a single bit from an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param bitNum Bit position to read (0-7)
 * @param data Container for single bit value
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in I2Cdev::readTimeout)
 * @return Status of read operation (ESP_OK = success)
 */
esp_err_t I2C_readBit(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint8_t *data) {
    uint8_t b;
    esp_err_t ret = I2C_readByte(devAddr, regAddr, &b);
    *data = b & (1 << bitNum);
    return ret;
}

/** Read a single bit from a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param bitNum Bit position to read (0-15)
 * @param data Container for single bit value
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in I2Cdev::readTimeout)
 * @return Status of read operation (ESP_OK = success)
 */
esp_err_t I2C_readBitW(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint16_t *data) {
    uint16_t b;
    uint8_t ret = I2C_readWord(devAddr, regAddr, &b);
    *data = b & (1 << bitNum);
    return ret;
}

/** Read multiple bits from an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param bitStart First bit position to read (0-7)
 * @param length Number of bits to read (not more than 8)
 * @param data Container for right-aligned value (i.e. '101' read from any bitStart position will equal 0x05)
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in I2Cdev::readTimeout)
 * @return Status of read operation (ESP_OK = success)
 */
esp_err_t I2C_readBits(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t *data) {
    // 01101001 read byte
    // 76543210 bit numbers
    //    xxx   args: bitStart=4, length=3
    //    010   masked
    //   -> 010 shifted
    uint8_t b;
    esp_err_t ret = I2C_readByte(devAddr, regAddr, &b);
    if (ret != ESP_FAIL) {
        uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
        b &= mask;
        b >>= (bitStart - length + 1);
        *data = b;
    }
    return ret;
}


/** Read multiple bits from a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param bitStart First bit position to read (0-15)
 * @param length Number of bits to read (not more than 16)
 * @param data Container for right-aligned value (i.e. '101' read from any bitStart position will equal 0x05)
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in I2Cdev::readTimeout)
 * @return Status of read operation (ESP_OK = success)
 */
esp_err_t I2C_readBitsW(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint16_t *data) {
    // 1101011001101001 read byte
    // fedcba9876543210 bit numbers
    //    xxx           args: bitStart=12, length=3
    //    010           masked
    //           -> 010 shifted
    uint16_t w;
    esp_err_t ret = I2C_readWord(devAddr, regAddr, &w);
    if (ret != ESP_FAIL) {
        uint16_t mask = ((1 << length) - 1) << (bitStart - length + 1);
        w &= mask;
        w >>= (bitStart - length + 1);
        *data = w;
    }
    return ret;
}

/** Read single byte from an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param data Container for byte value read from device
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in I2Cdev::readTimeout)
 * @return Status of read operation (ESP_OK = success)
 */
esp_err_t I2C_readByte(uint8_t devAddr, uint8_t regAddr, uint8_t *data) {
    return I2C_readBytes(devAddr, regAddr, 1, data);
}

/** Read single word from a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to read from
 * @param data Container for word value read from device
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in I2Cdev::readTimeout)
 * @return Status of read operation (ESP_OK = success)
 */
esp_err_t I2C_readWord(uint8_t devAddr, uint8_t regAddr, uint16_t *data) {
    return I2C_readWords(devAddr, regAddr, 1, data);
}

/** Read multiple bytes from an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register regAddr to read from
 * @param length Number of bytes to read
 * @param data Buffer to store read data in
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in I2Cdev::readTimeout)
 * @return Number of bytes read (ESP_OK = success)
 */
esp_err_t I2C_readBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t *data) {

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (devAddr << 1) | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, regAddr, ACK_CHECK_EN);
//	i2c_master_stop(cmd);
//	esp_err_t ret = i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_RATE_MS);
 //   i2c_cmd_link_delete(cmd);
 //   if(ret==ESP_FAIL){
 //   	return ret;
 //   }

//    cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (devAddr << 1) | READ_BIT, ACK_CHECK_EN);
    if (length > 1) {
        i2c_master_read(cmd, data, length - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, data + length - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}


/** Read multiple words from a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register regAddr to read from
 * @param length Number of words to read
 * @param data Buffer to store read data in
 * @param timeout Optional read timeout in milliseconds (0 to disable, leave off to use default class value in I2Cdev::readTimeout)
 * @return Number of words read (ESP_OK = success)
 */
esp_err_t I2C_readWords(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint16_t *data) {

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (devAddr << 1) | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, regAddr, ACK_CHECK_EN);
//	i2c_master_stop(cmd);
//	esp_err_t ret = i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_RATE_MS);
//    i2c_cmd_link_delete(cmd);
//    if(ret==ESP_FAIL){
//    	return ret;
//    }

//    cmd = i2c_cmd_link_create();
 	i2c_master_start(cmd);
 	i2c_master_write_byte(cmd, (devAddr << 1) | READ_BIT, ACK_CHECK_EN);
    if (length > 1) {
         i2c_master_read(cmd, (uint8_t *)data, length*2 - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, (uint8_t *)data + length*2 - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

/** write a single bit in an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to write to
 * @param bitNum Bit position to write (0-7)
 * @param value New bit value to write
 * @return Status of operation (ESP_OK = success)
 */
esp_err_t I2C_writeBit(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint8_t data) {
    uint8_t b;
    I2C_readByte(devAddr, regAddr, &b);
    b = (data != 0) ? (b | (1 << bitNum)) : (b & ~(1 << bitNum));
    return I2C_writeByte(devAddr, regAddr, b);
}

/** write a single bit in a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to write to
 * @param bitNum Bit position to write (0-15)
 * @param value New bit value to write
 * @return Status of operation (ESP_OK = success)
 */
esp_err_t I2C_writeBitW(uint8_t devAddr, uint8_t regAddr, uint8_t bitNum, uint16_t data) {
    uint16_t w;
    I2C_readWord(devAddr, regAddr, &w);
    w = (data != 0) ? (w | (1 << bitNum)) : (w & ~(1 << bitNum));
    return I2C_writeWord(devAddr, regAddr, w);
}

/** Write multiple bits in an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to write to
 * @param bitStart First bit position to write (0-7)
 * @param length Number of bits to write (not more than 8)
 * @param data Right-aligned value to write
 * @return Status of operation (ESP_OK = success)
 */
esp_err_t I2C_writeBits(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint8_t data) {
    //      010 value to write
    // 76543210 bit numbers
    //    xxx   args: bitStart=4, length=3
    // 00011100 mask byte
    // 10101111 original value (sample)
    // 10100011 original & ~mask
    // 10101011 masked | value
    uint8_t b;
    if (I2C_readByte(devAddr, regAddr, &b) != ESP_FAIL) {
        uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
        data <<= (bitStart - length + 1); // shift data into correct position
        data &= mask; // zero all non-important bits in data
        b &= ~(mask); // zero all important bits in existing byte
        b |= data; // combine data with existing byte
        return I2C_writeByte(devAddr, regAddr, b);
    } else {
        return ESP_FAIL;
    }
}


/** Write multiple bits in a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register regAddr to write to
 * @param bitStart First bit position to write (0-15)
 * @param length Number of bits to write (not more than 16)
 * @param data Right-aligned value to write
 * @return Status of operation (ESP_OK = success)
 */
esp_err_t I2C_writeBitsW(uint8_t devAddr, uint8_t regAddr, uint8_t bitStart, uint8_t length, uint16_t data) {
    //              010 value to write
    // fedcba9876543210 bit numbers
    //    xxx           args: bitStart=12, length=3
    // 0001110000000000 mask byte
    // 1010111110010110 original value (sample)
    // 1010001110010110 original & ~mask
    // 1010101110010110 masked | value
    uint16_t w;
    if (I2C_readWord(devAddr, regAddr, &w) != ESP_FAIL) {
        uint8_t mask = ((1 << length) - 1) << (bitStart - length + 1);
        data <<= (bitStart - length + 1); // shift data into correct position
        data &= mask; // zero all non-important bits in data
        w &= ~(mask); // zero all important bits in existing word
        w |= data; // combine data with existing word
        return I2C_writeWord(devAddr, regAddr, w);
    } else {
        return ESP_FAIL;
    }
}

/** Write single byte to an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register address to write to
 * @param data New byte value to write
 * @return Status of operation (ESP_OK = success)
 */
esp_err_t I2C_writeByte(uint8_t devAddr, uint8_t regAddr, uint8_t data) {
    return I2C_writeBytes(devAddr, regAddr, 1, &data);
}

/** Write single word to a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr Register address to write to
 * @param data New word value to write
 * @return Status of operation (ESP_OK = success)
 */
esp_err_t I2C_writeWord(uint8_t devAddr, uint8_t regAddr, uint16_t data) {
    return I2C_writeWords(devAddr, regAddr, 1, &data);
}

/** Write multiple bytes to an 8-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register address to write to
 * @param length Number of bytes to write
 * @param data Buffer to copy new data from
 * @return Status of operation (ESP_OK = success)
 */
esp_err_t I2C_writeBytes(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint8_t* data) {

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (devAddr << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, regAddr, ACK_CHECK_EN);
    i2c_master_write(cmd, data, length, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}


esp_err_t I2C_writeCmd(uint8_t devAddr, uint8_t command) {

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (devAddr << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, command, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}


/** Write multiple words to a 16-bit device register.
 * @param devAddr I2C slave device address
 * @param regAddr First register address to write to
 * @param length Number of words to write
 * @param data Buffer to copy new data from
 * @return Status of operation (ESP_OK = success)
 */
esp_err_t I2C_writeWords(uint8_t devAddr, uint8_t regAddr, uint8_t length, uint16_t* data) {

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (devAddr << 1) | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, regAddr, ACK_CHECK_EN);
    i2c_master_write(cmd, (uint8_t *)data, length*2, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

