#ifndef I2C_H__
#define I2C_H__

#include <stddef.h>
#include <stdint.h>

#include <esp_err.h>
#include <driver/i2c.h>
#include <freertos/FreeRTOS.h>

esp_err_t i2c_master_init(i2c_port_t i2c_port, int sda_io_num, int scl_io_num);
esp_err_t i2c_master_reg_read(i2c_port_t i2c_port, uint8_t device_address, TickType_t timeout, uint8_t reg_address, uint8_t *data, size_t length);
esp_err_t i2c_master_reg_write(i2c_port_t i2c_port, uint8_t device_address, TickType_t timeout, uint8_t reg_address, uint8_t *data, size_t length);

#endif
