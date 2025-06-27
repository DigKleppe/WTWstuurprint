#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdbool.h>
#include <stdio.h>

#include "settings.h"
#include "wifiConnect.h"
#include "ledTask.h"


esp_err_t init_spiffs(void);

static const char *TAG = "main";

extern "C" void app_main() {

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
	// strcpy ( wifiSettings.SSID, "kahjskljahs");  // test
	startLEDs();
	wifiConnect();

}
