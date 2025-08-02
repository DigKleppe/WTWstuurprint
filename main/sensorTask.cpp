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

sensorInfo_t sensorInfo[NR_SENSORS];

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

char buffer[8 * 1024];

void sensorTask(void *pvParameters) {
	udpMssg_t udpMssg;
	sensorMssg_t sensorMssg;
	int sensorNr;
	log_t tempLog;
	int lastminute = -1;
	time_t now = 0;
	struct tm timeinfo;
	// if (initLogBuffer() == NULL) {
	// 	ESP_LOGE(TAG, "Init logBuffer failed");
	// 	vTaskDelete(NULL);
	// }

	// for (int p = 0; p < 24* 60; p++) {
	// 	timeStamp += 60;
	// 	addToLog(tempLog);
	// }

	// 	scriptState = 0;
	// 	do {
	// 		len = getAllLogsScript(buffer, sizeof(buffer));
	// 		vTaskDelay(10);
	// 	} while (len > 0);
	// 	vTaskDelay( 200);
	// } while (1);

	const udpTaskParams_t udpTaskParams = {.port = UDPSENSORPORT, .maxLen = MAXLEN};

	xTaskCreate(udpServerTask, "udpServerTask", configMINIMAL_STACK_SIZE * 5, (void *)&udpTaskParams, 5, NULL);
	vTaskDelay(100);
	//	testLog();

	while (1) {
		if (xQueueReceive(udpMssgBox, &udpMssg, 0)) {
			if (udpMssg.mssg) {
				ESP_LOGI(TAG, "%s", udpMssg.mssg);
				if (sscanf(udpMssg.mssg, "S%d,%f,%f,%f,%d", &sensorNr, &sensorMssg.co2, &sensorMssg.temperature, &sensorMssg.hum, &sensorMssg.rssi) == 5) {
					if ((sensorNr >= 0) && (sensorNr < NR_SENSORS)) { // add values to temporary log   sensor0 is reference
						sensorInfo[sensorNr].messageCntr++;
						sensorInfo[sensorNr].status = SENSORSTATUS_OK;
						sensorInfo[sensorNr].timeoutTmr = 0;
						sensorInfo[sensorNr].CO2val = (int) sensorMssg.co2;
						tempLog.co2[sensorNr] = sensorMssg.co2;
						tempLog.temperature[sensorNr] = sensorMssg.temperature;
						tempLog.hum[sensorNr] = sensorMssg.hum;
						if (tempLog.co2[sensorNr] >= _ERRORVALUE)
							sensorInfo[sensorNr].status = SENSORSTATUS_ERROR;
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
		if (lastminute == -1) {
			lastminute = timeinfo.tm_min;
		}
		if (lastminute != timeinfo.tm_min) {
			lastminute = timeinfo.tm_min; // every minute
			addToLog(tempLog);			  // add to cyclic log buffer
			lastVal = tempLog;
			lastVal.timeStamp = timeStamp;
			memset(&tempLog, 0, sizeof(tempLog));
		}
		for (int n = 1; n < NR_SENSORS; n++) {					  // timeouts for regular sensors 1 .. 4, sensor 0 is temporary reference and calibrator
			if (sensorInfo[n].status > SENSORSTATUS_NOTPRESENT) { // once sensor is detected
				sensorInfo[n].timeoutTmr++;
				if (sensorInfo[n].timeoutTmr >= (SENSOR_TIMEOUT * 100)) {
					sensorInfo[n].status = SENSORSTATUS_NOCOMM;
					sensorInfo[n].timeoutTmr--;
				}
			}
		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	}
}

int getMaXCOValue ( void) {
	int max = -1;
	for (int sensorNr = 1; sensorNr < NR_SENSORS; sensorNr++) {	
		if ( sensorInfo[sensorNr].status == SENSORSTATUS_OK ) {
			if (sensorInfo[sensorNr].CO2val > max )
				max = sensorInfo[sensorNr].CO2val;
		}
	}
	return max;
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
	for (int idx = 0; idx < NR_SENSORS; idx++) {
		len += sprintf(pBuffer + len, "S%d,", idx);
		len += sprintf(pBuffer + len, "%ld,", logToPrint->timeStamp);
		len += sprintf(pBuffer + len, "%3.0f,", logToPrint->co2[idx]);
		len += sprintf(pBuffer + len, "%3.2f,", logToPrint->temperature[idx]);
		len += sprintf(pBuffer + len, "%3.2f ", logToPrint->hum[idx]);
	}
	len += sprintf(pBuffer + len, "\n");
	return len;
}
int getRTMeasValuesScript(char *pBuffer, int count) {
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

int getSensorStatusScript(char *pBuffer, int count) {
	int len = 0;
	switch (scriptState) {
	case 0:
		scriptState++;
		for (int idx = 0; idx < NR_SENSORS; idx++) {
			len += sprintf(pBuffer + len, "S%d,", idx );
			len += sprintf(pBuffer + len, "%d,", (int)sensorInfo[idx].status);
			len += sprintf(pBuffer + len, "%u", (unsigned int)sensorInfo[idx].messageCntr);
		}
		return len;
	default:
		break;
	}
	return 0;
}

int getInfoValuesScript(char *pBuffer, int count) {
	int len = 0;
	bool sensorFound = false;
	switch (scriptState) {
	case 0:
		scriptState++;
		len = sprintf(pBuffer, "%s\n", "Naam,Waarde");
		for (int idx = 0; idx < NR_SENSORS; idx++) {
			if (sensorInfo[idx].status != SENSORSTATUS_NOTPRESENT) {
				sensorFound = true;
				len += sprintf(pBuffer + len, "Sensor, %d\n", idx + 1);
				len += sprintf(pBuffer + len, "CO2,%3.0f\n", lastVal.co2[idx]);
				len += sprintf(pBuffer + len, "temperatuur,%3.2f\n", lastVal.temperature[idx]);
				len += sprintf(pBuffer + len, "RV,%3.0f\n", lastVal.hum[idx]);
				len += sprintf(pBuffer + len, "status, %d,", (int)sensorInfo[idx].status);
				len += sprintf(pBuffer + len, "berichten, %u\n", (unsigned int) sensorInfo[idx].messageCntr);
			}
		}
		if (!sensorFound)
			len += sprintf(pBuffer + len, "Fout, Geen Sensoren gevonden\n");
		return len;
	case 1:
		scriptState++;
		//		len = sprintf(pBuffer + len, "%s,%1.2f\n", "temperatuur offset", userSettings.temperatureOffset);
		len += sprintf(pBuffer + len, "%s,%s\n", "firmwareversie", getFirmWareVersion());
		len += sprintf(pBuffer + len, "%s,%s\n", "SPIFFS versie", wifiSettings.SPIFFSversion);
		len += sprintf(pBuffer + len, "%s,%d\n", "RSSI", getRssi());
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
