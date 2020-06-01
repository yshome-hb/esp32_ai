/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013/2014 Ibrahim Abdelkader <i.abdalkader@gmail.com>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * SCCB (I2C like) driver.
 *
 */
#include "sccb.h"

esp_err_t SCCB_Probe(uint8_t addr)
{
    return I2C_writeCmd(addr, 0);
}

uint8_t SCCB_Read(uint8_t addr, uint8_t reg)
{
    uint8_t data=0;
    I2C_readByte(addr, reg, &data);
    return data;
}

esp_err_t SCCB_Write(uint8_t addr, uint8_t reg, uint8_t data)
{
    return I2C_writeByte(addr, reg, data);
}
