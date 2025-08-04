#include "driver/gpio.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdbool.h>
#include <stdio.h>
#include "soc/gpio_reg.h"

#include "ledTask.h"
#include "sensorTask.h"
#include "settings.h"
#include "wifiConnect.h"
#include "motorControlTask.h"
#include "temperatureSensorTask.h"
#include "brinkTask.h"
#include "keys.h"
#include "keyDefs.h"

esp_err_t init_spiffs(void);

static const char *TAG = "main";

const char firmWareVersion[] = {"0.0"};

const char *getFirmWareVersion() { return firmWareVersion; }

uint32_t timeStamp = 1; // global timestamp for logging


myKey_t getKeyPins(void) { 
   uint32_t port =  REG_READ(GPIO_IN_REG);
   port =~ port;
   port &= (SK2 | SK3 | SB2 | SB3 | PB1 | PB2);
   
   return port; 
}

void initKeyPins (void) {
  gpio_set_direction( GPIO_NUM_4, GPIO_MODE_INPUT);
  gpio_set_pull_mode( GPIO_NUM_4, GPIO_FLOATING);
  gpio_set_direction( GPIO_NUM_5, GPIO_MODE_INPUT);
  gpio_set_pull_mode( GPIO_NUM_5, GPIO_FLOATING);
  gpio_set_direction( GPIO_NUM_6, GPIO_MODE_INPUT);
  gpio_set_pull_mode( GPIO_NUM_6, GPIO_FLOATING);
  gpio_set_direction( GPIO_NUM_7, GPIO_MODE_INPUT);
  gpio_set_pull_mode( GPIO_NUM_7, GPIO_FLOATING);
  gpio_set_direction( GPIO_NUM_12, GPIO_MODE_INPUT);
  gpio_set_pull_mode( GPIO_NUM_12, GPIO_FLOATING);
  gpio_set_direction( GPIO_NUM_13, GPIO_MODE_INPUT);
  gpio_set_pull_mode( GPIO_NUM_13, GPIO_FLOATING);
}

int cancelSettingsScript(char *pBuffer, int count); // dummy 

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
	// strcpy ( wifiSettings.SSID, "kahjskljahs");  // test
	startLEDs();
	wifiConnect();
	cancelSettingsScript(NULL, 0);

	xTaskCreate(sensorTask, "sensorTask", configMINIMAL_STACK_SIZE * 5, NULL, 1, NULL);
	xTaskCreate(temperatureSensorTask, "temperauurSesorTask", configMINIMAL_STACK_SIZE , NULL, 1, NULL);
	xTaskCreate(motorControlTask, "motorC1", 8000, (void *)AFAN, 1, NULL);
 	xTaskCreate(motorControlTask, "motorC2", 8000, (void *)TFAN, 1, NULL);
	xTaskCreate(brinkTask, "brinkTask", configMINIMAL_STACK_SIZE * 3, NULL, 1, NULL);
 	initKeyPins();

	while (1) {
		//	int rssi = getRssi();
		//	ESP_LOGI(TAG, "RSSI: %d", rssi);
		vTaskDelay(pdMS_TO_TICKS(20)); //
		keysTimerHandler_ms(pdMS_TO_TICKS(20));
		
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
