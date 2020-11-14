#include "i2c.h"

esp_err_t i2c_master_init(i2c_port_t i2c_port, int sda_io_num, int scl_io_num)
{
	i2c_config_t config;
	config.mode = I2C_MODE_MASTER;
	config.sda_io_num = sda_io_num;
	config.sda_pullup_en = GPIO_PULLUP_ENABLE;
	config.scl_io_num = scl_io_num;
	config.scl_pullup_en = GPIO_PULLUP_ENABLE;
	config.clk_stretch_tick = 300; // ???

	ESP_ERROR_CHECK(i2c_driver_install(i2c_port, config.mode));
	ESP_ERROR_CHECK(i2c_param_config(i2c_port, &config));

	return ESP_OK;
}

esp_err_t i2c_master_reg_read(i2c_port_t i2c_port, uint8_t device_address, TickType_t timeout, uint8_t reg_address, uint8_t *data, size_t length)
{
	esp_err_t ret;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, device_address << 1 | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, reg_address, true);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(i2c_port, cmd, timeout);
	i2c_cmd_link_delete(cmd);

	if (ret != ESP_OK) {
		return ret;
	}

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, device_address << 1 | I2C_MASTER_READ, true);
	i2c_master_read(cmd, data, length, I2C_MASTER_LAST_NACK);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(i2c_port, cmd, timeout);
	i2c_cmd_link_delete(cmd);

	return ret;
}

esp_err_t i2c_master_reg_write(i2c_port_t i2c_port, uint8_t device_address, TickType_t timeout, uint8_t reg_address, uint8_t *data, size_t length)
{
	esp_err_t ret;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, device_address << 1 | I2C_MASTER_WRITE, true);
	i2c_master_write_byte(cmd, reg_address, true);
	i2c_master_write(cmd, data, length, true);
	i2c_master_stop(cmd);
	ret = i2c_master_cmd_begin(i2c_port, cmd, timeout);
	i2c_cmd_link_delete(cmd);

	return ret;
}
