/*
Application entry point
*/

#include "esp_err.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "wifi_app.h"
#include "DHT22.h"

void app_main(void)
{

	//intialize nvs
	esp_err_t ret = nvs_flash_init();
	if(ret==ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND){
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
	
	//init wifi
	wifi_start_app();
	
	//start dht22 sensor task
	DHT22_task_start();
	
}
