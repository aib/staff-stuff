#include "i2c.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_err.h>
#include <esp_log.h>
#include <esp_system.h>

#define ERROR_PAUSE (1000 / portTICK_RATE_MS)

#define I2C_PORT I2C_NUM_0
#define I2C_SDA_GPIO_PIN 14
#define I2C_SCL_GPIO_PIN 2
#define I2C_TIMEOUT (1000 / portTICK_RATE_MS)

#define MPU6050_ADDRESS 0x68
#define MPU6050_WHOAMI_REGISTER 0x75
#define MPU6050_WHOAMI_RESPONSE 0x68

static const char *TAG = "mpu_osc";

static esp_err_t mpu6050_init(i2c_port_t i2c_port, uint8_t address)
{
	esp_err_t ret;
	uint8_t reg, data;

	reg = 26; //CONFIG
	data = 4; //DLPF_CFG = 4
	if ((ret = i2c_master_reg_write(i2c_port, address, I2C_TIMEOUT, reg, &data, 1)) != ESP_OK) {
		return ret;
	}

	reg = 107; //PWR_MGMT_1
	data = 1;  //CLKSEL = 1 (GYRO_X)
	if ((ret = i2c_master_reg_write(i2c_port, address, I2C_TIMEOUT, reg, &data, 1)) != ESP_OK) {
		return ret;
	}

	return ESP_OK;
}

static esp_err_t check_mpu6050(i2c_port_t i2c_port, uint8_t address)
{
	esp_err_t ret;
	uint8_t whoami;

	ret = i2c_master_reg_read(i2c_port, address, I2C_TIMEOUT, MPU6050_WHOAMI_REGISTER, &whoami, 1);
	if (ret != ESP_OK) {
		return ret;
	}

	if (whoami != MPU6050_WHOAMI_RESPONSE) {
		return ESP_ERR_INVALID_RESPONSE;
	}

	return ESP_OK;
}

static esp_err_t mpu_read_task_init()
{
	esp_err_t ret;

	ret = i2c_master_init(I2C_PORT, I2C_SDA_GPIO_PIN, I2C_SCL_GPIO_PIN);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Unable to initialize I²C");
		return ret;
	}

	vTaskDelay(500 / portTICK_RATE_MS);

	ret = mpu6050_init(I2C_PORT, MPU6050_ADDRESS);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Unable to initialize MPU");
		return ret;
	}

	return ESP_OK;
}

static void mpu_read_task(void *arg)
{
	esp_err_t ret;
	uint8_t sensor_data[14];

	ret = mpu_read_task_init();
	if (ret != ESP_OK) {
		vTaskDelay(ERROR_PAUSE);
		esp_restart();
	}

	ESP_LOGI(TAG, "Initialized");

	while (1) {
		ret = check_mpu6050(I2C_PORT, MPU6050_ADDRESS);
		if (ret != ESP_OK) {
			ESP_LOGE(TAG, "MPU not found");
			vTaskDelay(ERROR_PAUSE);
			continue;
		}

		ret = i2c_master_reg_read(I2C_PORT, MPU6050_ADDRESS, I2C_TIMEOUT, 0x3B, sensor_data, 14);
		if (ret != ESP_OK) {
			ESP_LOGE(TAG, "Bad response from MPU");
			vTaskDelay(ERROR_PAUSE);
			continue;
		}

		ESP_LOGD(TAG, "sensor_data: %6d %6d %6d %6d %6d %6d %6d",
			(int16_t)((sensor_data[0] << 8) | sensor_data[1]),
			(int16_t)((sensor_data[2] << 8) | sensor_data[3]),
			(int16_t)((sensor_data[4] << 8) | sensor_data[5]),
			(int16_t)((sensor_data[6] << 8) | sensor_data[7]),
			(int16_t)((sensor_data[8] << 8) | sensor_data[9]),
			(int16_t)((sensor_data[10] << 8) | sensor_data[11]),
			(int16_t)((sensor_data[12] << 8) | sensor_data[13])
		);

		vTaskDelay(50 / portTICK_RATE_MS);
	}
}

void app_main(void)
{
	xTaskCreate(mpu_read_task, "mpu_read_task", 1024, NULL, 10, NULL);
}
