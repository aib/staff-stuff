#include <stdio.h>

#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

void app_main()
{
	printf("Hello, World!\n");

	vTaskDelay(3000 / portTICK_PERIOD_MS);
	esp_restart();
}
