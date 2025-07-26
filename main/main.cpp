#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdbool.h>
#include <stdio.h>

#include "ledTask.h"
#include "sensorTask.h"
#include "settings.h"
#include "wifiConnect.h"

esp_err_t init_spiffs(void);

static const char *TAG = "main";

const char firmWareVersion[] = {"0.0"};

const char *getFirmWareVersion() { return firmWareVersion; }

uint32_t timeStamp = 1; // global timestamp for logging


extern "C" void app_main() {
	time_t now = 0;
	struct tm timeinfo;
	int lastSecond = -1;

	esp_err_t err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
		ESP_LOGI(TAG, "nvs flash erased");
	}
	ESP_ERROR_CHECK(err);

	ESP_ERROR_CHECK(esp_event_loop_create_default());
	err = init_spiffs();
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(err));
		return;
	}

	err = loadSettings();
	strcpy ( userSettings.moduleName, CONFIG_MDNS_HOSTNAME ); // for wtw monitor only 
	// strcpy ( wifiSettings.SSID, "kahjskljahs");  // test
	startLEDs();
	wifiConnect();

	xTaskCreate(sensorTask, "sensorTask", configMINIMAL_STACK_SIZE * 5, NULL, 1, NULL);

	while (1) {
		//	int rssi = getRssi();
		//	ESP_LOGI(TAG, "RSSI: %d", rssi);
		vTaskDelay(pdMS_TO_TICKS(200)); //
		time(&now);
		localtime_r(&now, &timeinfo);
		if (lastSecond != timeinfo.tm_sec) {
			lastSecond = timeinfo.tm_sec; // every second
			timeStamp++;
			if (timeStamp == 0)
				timeStamp++;
		}
	}
}
