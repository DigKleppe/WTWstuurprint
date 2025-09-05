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
#include "updateTask.h"
#include "systemCheckTask.h"
#include "keys.h"
#include "keyDefs.h"

#define IPDIGITPIN GPIO_NUM_44
//#define USE_STATS 

#ifdef 	USE_STATS
	char statsBuf[1000];
#endif

esp_err_t init_spiffs(void);

static const char *TAG = "main";

uint32_t timeStamp = 1; // global timestamp for logging

myKey_t getKeyPins(void) { 
   uint32_t port =  REG_READ(GPIO_IN_REG);
   port =~ port;
   port &= (SK2 | SK3 | SB2 | SB3 | PB1 | PB2);
   
   return port; 
}

void initKeyPins (void) {
  gpio_set_direction( GPIO_NUM_4, GPIO_MODE_INPUT);
  gpio_set_pull_mode( GPIO_NUM_4, GPIO_PULLDOWN_ONLY);
  gpio_set_direction( GPIO_NUM_5, GPIO_MODE_INPUT);
  gpio_set_pull_mode( GPIO_NUM_5, GPIO_PULLDOWN_ONLY);
  gpio_set_direction( GPIO_NUM_6, GPIO_MODE_INPUT);
  gpio_set_pull_mode( GPIO_NUM_6, GPIO_PULLDOWN_ONLY);
  gpio_set_direction( GPIO_NUM_7, GPIO_MODE_INPUT);
  gpio_set_pull_mode( GPIO_NUM_7, GPIO_PULLDOWN_ONLY);
  gpio_set_direction( GPIO_NUM_12, GPIO_MODE_INPUT);
  gpio_set_pull_mode( GPIO_NUM_12, GPIO_PULLDOWN_ONLY);
  gpio_set_direction( GPIO_NUM_13, GPIO_MODE_INPUT);
  gpio_set_pull_mode( GPIO_NUM_13, GPIO_PULLDOWN_ONLY);
}

int cancelSettingsScript(char *pBuffer, int count); // dummy 

#define NO_TASKS 7
extern "C" void app_main() {
	time_t now = 0;
	struct tm timeinfo;
	int lastSecond = -1;

	TaskHandle_t taskHandles[NO_TASKS];

	gpio_set_direction( IPDIGITPIN, GPIO_MODE_INPUT);  // link for local test different fixed ip 
  	gpio_set_pull_mode( IPDIGITPIN, GPIO_PULLUP_ONLY);

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

	if (gpio_get_level ( IPDIGITPIN) == 0){ // then link placed on J2 9-10 for local test
		strcpy(userSettings.moduleName,"WTW2");
		advSettings.fixedIPdigit = CONFIG_FIXED_LAST_IP_DIGIT + 10; 
	}

	// strcpy ( wifiSettings.SSID, "kahjskljahs");  // test
	startLEDs();
	wifiConnect();

	xTaskCreate(sensorTask, "sensorTask", configMINIMAL_STACK_SIZE * 5, NULL, 1, &taskHandles[0]);
	xTaskCreate(temperatureSensorTask, "temperauurSensorTask", configMINIMAL_STACK_SIZE * 2 , NULL, 1, &taskHandles[1]);
	xTaskCreate(motorControlTask, "motorC1", 8000, (void *)AFAN, 1, &taskHandles[2]);
 	xTaskCreate(motorControlTask, "motorC2", 8000, (void *)TFAN, 1, &taskHandles[3]);
	xTaskCreate(brinkTask, "brinkTask", configMINIMAL_STACK_SIZE * 3, NULL, 1, &taskHandles[4]);
	xTaskCreate(&updateTask, "updateTask",2* 8192, NULL, 1, &taskHandles[5]);
	xTaskCreate(&systemCheckTask, "systemCheckTask",configMINIMAL_STACK_SIZE * 2, NULL, 1, &taskHandles[6]);
	initKeyPins();
 
	while (1) {
		vTaskDelay(pdMS_TO_TICKS(20)); //
		keysTimerHandler_ms(20);
		
		time(&now);
		localtime_r(&now, &timeinfo);
		if (lastSecond != timeinfo.tm_sec) {
			lastSecond = timeinfo.tm_sec; // every second
			timeStamp++;
			if (timeStamp == 0)
				timeStamp++;
		}

#ifdef 	USE_STATS
		vTaskGetRunTimeStats( statsBuf );
		printf("%s\n\r", statsBuf);
		for (int n = 0; n< NO_TASKS; n++)
		{
			printf("%s\t", pcTaskGetName( taskHandles[n]));
			printf("%d\n\r", uxTaskGetStackHighWaterMark( taskHandles[n]));
		}
		printf("\n\rFree:%d\n\r", xPortGetFreeHeapSize());
		vTaskDelay( 2000);
#endif

	}
}
