/*
 * sensorTask.cpp
 *
 *  Created on: june 28 2025
 *      Author: dig
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"

#include "cgiScripts.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "log.h"
#include "sensorTask.h"
#include "settings.h"
#include "udpServer.h"
#include "wifiConnect.h"

#include <cstring>
#include <math.h>

static const char *TAG = "sensorTask";

#define SENSORMSSG_TIMEOUT 1000 // wait 1 sec
#define UDPSENSORPORT 5050
#define MAXLEN 128

const char *getFirmWareVersion();
extern int scriptState;
log_t lastVal;

typedef struct {
	float co2;
	float temperature;
	float hum;
	int rssi;
	//	int seqNr;
} sensorMssg_t;

char buf[500];

void testLog(void) {
	log_t tempLog;
	tempLog.co2[0] = 450;
	tempLog.co2[1] = 460;
	tempLog.co2[2] = 470;
	tempLog.co2[3] = 480;
	tempLog.hum[0] = 40;
	tempLog.hum[1] = 50;
	tempLog.hum[2] = 60;
	tempLog.hum[3] = 70;
	tempLog.temperature[0] = 20;
	tempLog.temperature[1] = 22;
	tempLog.temperature[2] = 24;
	tempLog.temperature[3] = 26;

	for (int n = 0; n < 3; n++) {
		timeStamp++;
		addToLog(tempLog);
		tempLog.co2[0] += 1;
		tempLog.co2[1] += 1;
		tempLog.co2[2] += 1;
		tempLog.co2[3] += 1;
		tempLog.hum[0] += 0.5;
		tempLog.hum[1] += 0.5;
		tempLog.hum[2] += 0.5;
		tempLog.hum[3] += 0.5;
		tempLog.temperature[0] += 1;
		tempLog.temperature[1] += 1;
		tempLog.temperature[2] += 1;
		tempLog.temperature[3] += 1;
	}
	
	getAllLogsScript(buf, 100);


}


// creates UDP task for receiving sensordata
// parses the messages

void sensorTask(void *pvParameters) {
	udpMssg_t udpMssg;
	sensorMssg_t sensorMssg;
	int sensorNr;
	log_t tempLog;
	int lastminute = -1;
	time_t now = 0;
	struct tm timeinfo;

	const udpTaskParams_t udpTaskParams = {.port = UDPSENSORPORT, .maxLen = MAXLEN};

	xTaskCreate(udpServerTask, "udpServerTask", configMINIMAL_STACK_SIZE * 5, (void *)&udpTaskParams, 5, NULL);
	vTaskDelay(100);
//	testLog();

	while (1) {
		if (xQueueReceive(udpMssgBox, &udpMssg, 0)) { // wait for messages from sensors to arrive
			if (udpMssg.mssg) {
				ESP_LOGI(TAG, "%s", udpMssg.mssg);
				if (sscanf(udpMssg.mssg, "S%d,%f,%f,%f,%d", &sensorNr, &sensorMssg.co2, &sensorMssg.temperature, &sensorMssg.hum, &sensorMssg.rssi) == 5) {
					if ((sensorNr > 0) && (sensorNr < 4)) { // add values to temporary log
						tempLog.co2[sensorNr-1] = sensorMssg.co2;
						tempLog.temperature[sensorNr-1] = sensorMssg.temperature;
						tempLog.hum[sensorNr-1] = sensorMssg.hum;
					} else
						ESP_LOGE(TAG, "Wrong sensornr (%d)", sensorNr);
				} else
					ESP_LOGE(TAG, "Error reading sensor%d: %s", sensorNr, udpMssg.mssg);
				free(udpMssg.mssg);

			} else
				ESP_LOGE(TAG, "Error reading sensor");
		}

		time(&now);
		localtime_r(&now, &timeinfo);
		if ( lastminute == -1) {
			lastminute = timeinfo.tm_min; 
		}
		if (lastminute != timeinfo.tm_min) {
			lastminute = timeinfo.tm_min; // every minute
			addToLog(tempLog); // add to cyclic log buffer
			lastVal = tempLog;
			lastVal.timeStamp = timeStamp;
			memset(&tempLog, 0, sizeof(tempLog));
		}
		vTaskDelay(10);
	}
}

// CGI stuff

int printLog(log_t *logToPrint, char *pBuffer, int idx) {
	int len;
	len = sprintf(pBuffer, "%ld,", logToPrint->timeStamp);
	len += sprintf(pBuffer + len, "%3.0f,", logToPrint->co2[idx]);
	len += sprintf(pBuffer + len, "%3.2f,", logToPrint->temperature[idx]);
	len += sprintf(pBuffer + len, "%3.2f\n", logToPrint->hum[idx]);
	return len;
}

int printLog(log_t *logToPrint, char *pBuffer) {
	int len = 0;
	for (int idx = 0; idx < 4; idx++) {
		len += sprintf(pBuffer + len, "S%d,", idx+1);
		len += sprintf(pBuffer + len, "%ld,", logToPrint->timeStamp);
		len += sprintf(pBuffer + len, "%3.0f,", logToPrint->co2[idx]);
		len += sprintf(pBuffer + len, "%3.2f,", logToPrint->temperature[idx]);
		len += sprintf(pBuffer + len, "%3.2f ", logToPrint->hum[idx]);
	}
	len += sprintf(pBuffer + len, "\n");
	return len;
}
int getRTMeasValuesScript( char *pBuffer, int count) {
	int len = 0;
	switch (scriptState) {
	case 0:
		scriptState++;
		len += printLog(&lastVal, pBuffer);
		return len;
		break;
	default:
		break;
	}
	return 0;
}

// prints last measurement values from sensor 1 .. 4

// int getRTMeasValuesScript1(char *pBuffer, int count) { return getRTMeasValuesScript(1, pBuffer, count); }
// int getRTMeasValuesScript2(char *pBuffer, int count) { return getRTMeasValuesScript(2, pBuffer, count); }
// int getRTMeasValuesScript3(char *pBuffer, int count) { return getRTMeasValuesScript(3, pBuffer, count); }
// int getRTMeasValuesScript4(char *pBuffer, int count) { return getRTMeasValuesScript(4, pBuffer, count); }

int getInfoValuesScript(char *pBuffer, int count) {
	int len = 0;
	switch (scriptState) {
	case 0:
		scriptState++;
		len = sprintf(pBuffer, "%s\n", "Naam,Waarde");
		// len += sprintf(pBuffer + len, "%s,%s\n", "Sensornaam", userSettings.moduleName);
		// len += sprintf(pBuffer + len, "%s,%3.0f\n", "CO2", lastVal.co2);
		// len += sprintf(pBuffer + len, "%s,%3.2f\n", "temperatuur", lastVal.temperature);
		// len += sprintf(pBuffer + len, "%s,%3.0f\n", "Vochtigheid", lastVal.hum);
		return len;
	case 1:
		scriptState++;
		//		len = sprintf(pBuffer + len, "%s,%1.2f\n", "temperatuur offset", userSettings.temperatureOffset);
		len += sprintf(pBuffer + len, "%s,%d\n", "RSSI", getRssi());
		len += sprintf(pBuffer + len, "%s,%s\n", "firmwareversie", getFirmWareVersion());
		len += sprintf(pBuffer + len, "%s,%s\n", "SPIFFS versie", wifiSettings.SPIFFSversion);
		return len;
		break;
	default:
		break;
	}
	return 0;
}

// only build javascript table

int getCalValuesScript(char *pBuffer, int count) {
	int len = 0;
	switch (scriptState) {
	case 0:
		scriptState++;
		len = sprintf(pBuffer, "%s\n", "Meting,Referentie,Stel in");
		len += sprintf(pBuffer + len, "%s\n", "CO2\n temperatuur");
		return len;
		break;
	default:
		break;
	}
	return 0;
}

int saveSettingsScript(char *pBuffer, int count) {
	saveSettings();
	return 0;
}

int cancelSettingsScript(char *pBuffer, int count) {
	loadSettings();
	return 0;
}

void parseCGIWriteData(char *buf, int received) {

	bool save = false;

	// if (sscanf(buf, "setName:moduleName=%s", userSettings.moduleName) == 1) {
	//  	ESP_LOGI(TAG, "Hostname set to %s", userSettings.moduleName);
	//  	save = true;
	// }

	if (strncmp(buf, "forgetWifi", 10) == 0) {
		ESP_LOGI(TAG, "Wifisettings erased");
		strcpy(wifiSettings.SSID, "xx");
		strcpy(wifiSettings.pwd, "xx");
		saveSettings();
		esp_restart();
	}

	// if (strncmp(buf, "setCal:", 7) == 0) { // calvalues are written , in sensirionTasks write these to SCD30
	// 	if (readActionScript(&buf[7], calibrateDescriptors, NR_CALDESCRIPTORS)) {
	// 		calvaluesReceived = true;
	// 		ESP_LOGI(TAG, "calvalues received: %f, %f", calValues.CO2, calValues.temperature);

	// 		if (calValues.temperature != NOCAL) { // then real temperature received
	// 			float measuredTemp = lastVal.temperature + userSettings.temperatureOffset;
	// 			userSettings.temperatureOffset = measuredTemp - calValues.temperature;
	// 			save = true;
	// 		}
	// 	}
	// }
	if (save)
		saveSettings();
}
