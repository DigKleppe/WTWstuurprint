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

#include "CGIcommonScripts.h"
#include "averager.h"
#include "cgiScripts.h"
#include "esp_log.h"
#include "freertos/task.h"
#include "log.h"
#include "sensorTask.h"
#include "settings.h"
#include "udpServer.h"
#include "wifiConnect.h"
#include "measureRPMtask.h"

#include <cstring>
#include <math.h>

static const char *TAG = "sensorTask";

#define SENSORMSSG_TIMEOUT 1000 // wait 1 sec
#define UDPSENSORPORT 5050
#define MAXLEN 128
#define SENSOR_TIMEOUT 60 // 60 seconds timeout for sensors
#define LOGINTERVAL 5	  // minutes
#define AVERAGES 10		  // number of values to average

const char *getFirmWareVersion();
extern int scriptState;
log_t lastVal;

sensorInfo_t sensorInfo[NR_SENSORS];
Averager Co2Averager[NR_SENSORS];
Averager tempAverager[NR_SENSORS];
Averager RHaverager[NR_SENSORS];
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
	int logPrescaler = 1;
	log_t tempLog;
	int lastminute = -1;
	time_t now = 0;
	struct tm timeinfo;

	for (int n = 0; n < NR_SENSORS; n++) {
		Co2Averager[n].setAverages(AVERAGES);
		tempAverager[n].setAverages(AVERAGES);
		RHaverager[n].setAverages(AVERAGES);
	}

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
						sensorInfo[sensorNr].CO2val = (int)sensorMssg.co2;
						sensorInfo[sensorNr].temperature = sensorMssg.temperature;
						sensorInfo[sensorNr].RH = (int)sensorMssg.hum;
						sensorInfo[sensorNr].rssi = sensorMssg.rssi;
						sensorInfo[sensorNr].ipAddress = udpMssg.ipAddress;
						Co2Averager[sensorNr].write((int)sensorMssg.co2 * 1000);
						tempAverager[sensorNr].write(sensorMssg.temperature * 1000.0);
						RHaverager[sensorNr].write((int)sensorMssg.hum * 1000);

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
			tempLog.timeStamp = timeStamp;
			for (int n = 0; n < NR_SENSORS; n++) {
				if (sensorInfo[n].status > SENSORSTATUS_NOTPRESENT) { // once sensor is detected
					sensorInfo[n].timeoutTmr++;
					if (sensorInfo[n].timeoutTmr >= (SENSOR_TIMEOUT * 100)) {
						sensorInfo[n].status = SENSORSTATUS_NOCOMM;
						sensorInfo[n].timeoutTmr--;
					}
				}
				if (sensorInfo[n].status == SENSORSTATUS_OK) {
					tempLog.co2[n] = Co2Averager[n].average() / 1000.0;
					tempLog.temperature[n] = tempAverager[n].average() / 1000.0;
					tempLog.hum[n] = RHaverager[n].average() / 1000.0;

				} else {
					tempLog.co2[n] = _ERRORVALUE;
					tempLog.temperature[n] = _ERRORVALUE;
					tempLog.hum[n] = _ERRORVALUE;
				}
			}
			tempLog.maxCO2 = getMaXCOValue();
			tempLog.RPM = getRPM(AFAN);
			lastVal = tempLog;
			lastVal.timeStamp = timeStamp;
			if (logPrescaler > 0) {
				logPrescaler--;
			} else {
				logPrescaler = LOGINTERVAL; // reset prescaler
				addToLog(tempLog);			// add to cyclic log buffer
			}
		}
		vTaskDelay( 10/portTICK_PERIOD_MS);
	}
}

int getMaXCOValue(void) {
	int max = -1;
	for (int sensorNr = 1; sensorNr < NR_SENSORS; sensorNr++) {
		if (sensorInfo[sensorNr].status == SENSORSTATUS_OK) {
			if (sensorInfo[sensorNr].CO2val > max)
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
	len += sprintf(pBuffer + len, "CO2,%d,", logToPrint->maxCO2);
	len += sprintf(pBuffer + len, "RPM,%d", logToPrint->RPM);
	return len;
}

int printLog(log_t *logToPrint, char *pBuffer) {
	int len = 0;
	for (int idx = 0; idx < NR_SENSORS; idx++) {
		len += sprintf(pBuffer + len, "S%d,", idx);
		len += sprintf(pBuffer + len, "%ld,", logToPrint->timeStamp);
		len += sprintf(pBuffer + len, "%3.0f,", logToPrint->co2[idx]);
		len += sprintf(pBuffer + len, "%3.2f,", logToPrint->temperature[idx]);
		len += sprintf(pBuffer + len, "%3.2f,", logToPrint->hum[idx]);
	}
	len += sprintf(pBuffer + len, "CO2,%d,", logToPrint->maxCO2);
	len += sprintf(pBuffer + len, "RPM,%d", logToPrint->RPM);
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

const CGIdesc_t sensorInfoDescriptorTable[][7] = {{{"RefSensor CO2 (ppm)", &sensorInfo[0].CO2val, INT, 1},
												   {"RefSensor temperatuur (°C)", &sensorInfo[0].temperature, FLT, 1},
												   {"RefSensor RH (%)", &sensorInfo[0].RH, INT, 1},
												   {"RefSensor RSSI", &sensorInfo[0].rssi, INT, 1},
												   {"RefSensor berichten", &sensorInfo[0].messageCntr, INT, 1},
												   {"RefSensor IPadres", &sensorInfo[0].ipAddress, IPADDR, 1},
												   {NULL, NULL, INT, 1}},
												  {{"Sensor 1 CO2 (ppm)", &sensorInfo[1].CO2val, INT, 1},
												   {"Sensor 1 temperatuur (°C)", &sensorInfo[1].temperature, FLT, 1},
												   {"Sensor 1 RH (%)", &sensorInfo[1].RH, INT, 1},
												   {"Sensor 1 RSSI", &sensorInfo[1].rssi, INT, 1},
												   {"Sensor 1 berichten", &sensorInfo[1].messageCntr, INT, 1},
												   {"Sensor 1 IPadres", &sensorInfo[1].ipAddress, IPADDR, 1},
												   {NULL, NULL, INT, 1}},
												  {{"Sensor 2 CO2 (ppm)", &sensorInfo[2].CO2val, INT, 1},
												   {"Sensor 2 temperatuur (°C)", &sensorInfo[2].temperature, FLT, 1},
												   {"Sensor 2 RH (%)", &sensorInfo[2].RH, INT, 1},
												   {"Sensor 2 RSSI", &sensorInfo[2].rssi, INT, 1},
												   {"Sensor 2 berichten", &sensorInfo[2].messageCntr, INT, 1},
												   {"Sensor 2 IPadres", &sensorInfo[2].ipAddress, IPADDR, 1},
												   {NULL, NULL, INT, 1}},
												  {{"Sensor 3 CO2 (ppm)", &sensorInfo[3].CO2val, INT, 1},
												   {"Sensor 3 temperatuur (°C)", &sensorInfo[3].temperature, FLT, 1},
												   {"Sensor 3 RH (%)", &sensorInfo[3].RH, INT, 1},
												   {"Sensor 3 RSSI", &sensorInfo[3].rssi, INT, 1},
												   {"Sensor 3 berichten", &sensorInfo[3].messageCntr, INT, 1},
												   {"Sensor 3 IPadres", &sensorInfo[3].ipAddress, IPADDR, 1},
												   {NULL, NULL, INT, 1}},
												  {{"Sensor 4 CO2 (ppm)", &sensorInfo[4].CO2val, INT, 1},
												   {"Sensor 4 temperatuur (°C)", &sensorInfo[4].temperature, FLT, 1},
												   {"Sensor 4 RH (%)", &sensorInfo[4].RH, INT, 1},
												   {"Sensor 4 RSSI", &sensorInfo[4].rssi, INT, 1},
												   {"Sensor 4 berichten", &sensorInfo[4].messageCntr, INT, 1},
												   {"Sensor 4 IPadres", &sensorInfo[4].ipAddress, IPADDR, 1},
												   {NULL, NULL, INT, 1}}};

int getSensorInfoScript(char *pBuffer, int count) {
	int len = 0;
	bool sensorFound = false;
	switch (scriptState) {
	case 0:
		scriptState++;
		len = sprintf(pBuffer, "Parameter, Waarde\n");
		for (int n = 0; n < NR_SENSORS; n++) {
			if (sensorInfo[n].status != SENSORSTATUS_NOTPRESENT) {
				len += getCGItable(sensorInfoDescriptorTable[n], pBuffer + len, count);
				sensorFound = true;
			}
		}
		if (!sensorFound) {
			len += sprintf(pBuffer + len, "Fout, Geen sensoren gevonden\n");
		}
		return len;
	default:
		break;
	}
	return 0;
}

int getInfoValuesScript(char *pBuffer, int count) {
	int len = 0;
	switch (scriptState) {
	case 0:
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
