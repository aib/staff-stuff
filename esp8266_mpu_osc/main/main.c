#include "i2c.h"

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <esp_err.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_system.h>
#include <esp_wifi.h>

#include <lwip/sockets.h>

#define WIFI_SSID "ssid"
#define WIFI_PASSWORD "password"
#define TARGET_HOST "192.168.1.100"
#define TARGET_PORT 12345

#define ERROR_PAUSE (1000 / portTICK_RATE_MS)

#define I2C_PORT I2C_NUM_0
#define I2C_SDA_GPIO_PIN 14
#define I2C_SCL_GPIO_PIN 2
#define I2C_TIMEOUT (1000 / portTICK_RATE_MS)

#define MPU6050_ADDRESS 0x68
#define MPU6050_WHOAMI_REGISTER 0x75
#define MPU6050_WHOAMI_RESPONSE 0x68

static const char *TAG = "mpu_osc";

static volatile bool networkReady = false;

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
		esp_wifi_connect();
		return;
	}

	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
		networkReady = false;
		esp_wifi_connect();
		return;
	}

	if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
		networkReady = true;
		return;
	}
}

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
	int iret;
	uint8_t sensor_data[14];
	int sock;
	struct sockaddr_in sock_dest;
	TickType_t last_process_time;

	ret = mpu_read_task_init();
	if (ret != ESP_OK) {
		vTaskDelay(ERROR_PAUSE);
		esp_restart();
	}

	sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock < 0) {
		ESP_LOGE(TAG, "Unable to create socket, errno = %d", errno);
		vTaskDelay(ERROR_PAUSE);
		esp_restart();
	}

	sock_dest.sin_addr.s_addr = inet_addr(TARGET_HOST);
	sock_dest.sin_family = AF_INET;
	sock_dest.sin_port = htons(TARGET_PORT);

	last_process_time = xTaskGetTickCount();

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

		ESP_LOGD(TAG, "Net: %s Sensor Data: %6d %6d %6d %6d %6d %6d %6d",
			(networkReady ? "yes" : "no"),
			(int16_t)((sensor_data[0] << 8) | sensor_data[1]),
			(int16_t)((sensor_data[2] << 8) | sensor_data[3]),
			(int16_t)((sensor_data[4] << 8) | sensor_data[5]),
			(int16_t)((sensor_data[6] << 8) | sensor_data[7]),
			(int16_t)((sensor_data[8] << 8) | sensor_data[9]),
			(int16_t)((sensor_data[10] << 8) | sensor_data[11]),
			(int16_t)((sensor_data[12] << 8) | sensor_data[13])
		);

		if (networkReady) {
			iret = sendto(sock, sensor_data, 14, 0, (struct sockaddr *) &sock_dest, sizeof(sock_dest));
			if (iret < 0) {
				ESP_LOGE(TAG, "Unable to send network packet, errno = %d", errno);
				vTaskDelay(ERROR_PAUSE);
				continue;
			}
		}

		vTaskDelayUntil(&last_process_time, 50 / portTICK_RATE_MS);
	}
}

static void wifi_init(void)
{
	ESP_LOGI(TAG, "Initializing WiFi, SSID is %s", WIFI_SSID);

	wifi_init_config_t wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&wifi_init_config));
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, event_handler, NULL));
	ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_handler, NULL));

	wifi_config_t wifi_config = {
		.sta = {
			.ssid = WIFI_SSID,
			.password = WIFI_PASSWORD
		}
	};

	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());
}

void app_main(void)
{
	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(esp_netif_init());

	wifi_init();

	xTaskCreate(mpu_read_task, "mpu_read_task", 4096, NULL, 10, NULL);
}
