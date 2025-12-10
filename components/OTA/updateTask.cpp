/*
 * updateTask.cpp
 *
 *  Created on: Jan 8, 2024
 *      Author: dig
 */

#include "errno.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <inttypes.h>
#include <string.h>

#include "httpsReadFile.h"
#include "settings.h"
#include "updateFirmWareTask.h"
#include "updateSpiffsTask.h"
#include "updateTask.h"
#include "wifiConnect.h"

static const char *TAG = "updateTask";

volatile updateStatus_t updateStatus;
volatile bool getNewVersionTaskFinished;
volatile bool forceUpdate;

typedef struct {
	char *infoFileName;
	char *dest;
} versionInfoParam_t;

esp_err_t getNewVersion(char *infoFileName, char *newVersion) {
	char url[96];
	int len;

	strcpy(url, wifiSettings.upgradeURL);
	strcat(url, "/");
	strcat(url, infoFileName);

	len = httpsReadFile(url, newVersion, MAX_STORAGEVERSIONSIZE - 1);

	if (len > 0) {
		newVersion[len] = 0;
		return ESP_OK;
	}

	else
		return ESP_FAIL;
}
// must run with DHCP! DNS does not work , depending on modem

void updateTask(void *pvParameter) {
//	int prescaler = CONFIG_CHECK_FIRMWARWE_UPDATE_INTERVAL * 60 * 60;
	bool doUpdate;
	bool error = false;
	char newVersion[MAX_STORAGEVERSIONSIZE];
	TaskHandle_t updateFWTaskh;
	TaskHandle_t updateSPIFFSTaskh;

	ESP_LOGI(TAG, "Running");

	if ((strcmp(wifiSettings.upgradeFileName, CONFIG_FIRMWARE_UPGRADE_FILENAME) != 0) || (strcmp(wifiSettings.upgradeURL, CONFIG_DEFAULT_FIRMWARE_UPGRADE_URL) != 0)) {
		strcpy(wifiSettings.upgradeFileName, CONFIG_FIRMWARE_UPGRADE_FILENAME);
		strcpy(wifiSettings.upgradeURL, CONFIG_DEFAULT_FIRMWARE_UPGRADE_URL);
		saveSettings();
	}

	// while (connectStatus != CONNECT_READY)
	// 	vTaskDelay(1000 / portTICK_PERIOD_MS);

	const esp_partition_t *update_partition = NULL;
	const esp_partition_t *configured = esp_ota_get_boot_partition();
	const esp_partition_t *running = esp_ota_get_running_partition();

	if (configured != running) {
		ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08" PRIx32 ", but running from offset 0x%08" PRIx32, configured->address, running->address);
		ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
	}
	ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08" PRIx32 ")", running->type, running->subtype, running->address);

//	while (1) {
		doUpdate = false;
		error = false;
		getNewVersion(BINARY_INFO_FILENAME, newVersion);
		if (newVersion[0] != 0) {
			if (strcmp(newVersion, wifiSettings.firmwareVersion) != 0) {
				ESP_LOGI(TAG, "New firmware version available: %s", newVersion);
				doUpdate = true;
			} else
				ESP_LOGI(TAG, "Firmware up to date: %s", newVersion);
		} else {
			ESP_LOGI(TAG, "Reading New firmware info failed");
			error = true;
		}

		if (doUpdate) {
			ESP_LOGI(TAG, "Updating firmware to version: %s", newVersion);
			xTaskCreate(&updateFirmwareTask, "updateFirmwareTask", 2 * 8192, NULL, 5, &updateFWTaskh);
			vTaskDelay(100 / portTICK_PERIOD_MS);
			while (updateStatus == UPDATE_BUSY)
				vTaskDelay(100 / portTICK_PERIOD_MS);

			if (updateStatus == UPDATE_RDY) {
				strcpy(wifiSettings.firmwareVersion, newVersion);
				saveSettings();
				ESP_LOGI(TAG, "Update successfull, restarting system!");
				vTaskDelay(100 / portTICK_PERIOD_MS);
				esp_restart();
			} else {
				ESP_LOGI(TAG, "Update firmware failed!");
				vTaskDelay(10000 / portTICK_PERIOD_MS);
				error = true;
			}
		}

		// ********************* SPIFFS update ***************************

		doUpdate = false;
		getNewVersion(SPIFFS_INFO_FILENAME, newVersion);
		if (newVersion[0] != 0) {
			if (strcmp(newVersion, wifiSettings.SPIFFSversion) != 0) {
				ESP_LOGI(TAG, "New SPIFFS version available: %s", newVersion);
				doUpdate = true;
			} else
				ESP_LOGI(TAG, "SPIFFS up to date: %s", newVersion);
		} else {
			ESP_LOGI(TAG, "Reading New SPIFFS info failed");
			error = true;
		}

		if (doUpdate) {
			ESP_LOGI(TAG, "Updating SPIFFS to version: %s", newVersion);
			xTaskCreate(&updateSpiffsTask, "updateSpiffsTask", 2 * 8192, NULL, 5, &updateSPIFFSTaskh);
			vTaskDelay(100 / portTICK_PERIOD_MS);

			while (updateStatus == UPDATE_BUSY) // wait for task to finish
				vTaskDelay(100 / portTICK_PERIOD_MS);

			if (updateStatus == UPDATE_RDY) {
				ESP_LOGI(TAG, "SPIFFS flashed OK");
				strcpy(wifiSettings.SPIFFSversion, newVersion);
				saveSettings();
			} else
				ESP_LOGI(TAG, "Update SPIFFS failed!");
		}
		vTaskDelete(NULL); 

	// 	if  (error) {
	// 		prescaler = 60 * 60; // check over 1 hour
	// 	}
	// 	do {
	// 		vTaskDelay(1000 / portTICK_PERIOD_MS);
	// 	} while (prescaler-- && !forceUpdate);
	// 	forceUpdate = false;
	// 	prescaler = CONFIG_CHECK_FIRMWARWE_UPDATE_INTERVAL * 60 * 60;
	// }
}
