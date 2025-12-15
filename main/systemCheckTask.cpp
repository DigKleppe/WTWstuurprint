#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdbool.h>

#include "esp_log.h"

#include "brinkTask.h"
#include "keyDefs.h"
#include "keys.h"
#include "ledTask.h"
#include "measureRPMtask.h"
#include "motorControlTask.h"
#include "sensorTask.h"
#include "settings.h"
#include "temperatureSensorTask.h"
#include "updateTask.h"
#include "wifiConnect.h"

#include "ping.h"

static const char *TAG = "systemCheck";
#define MOTORERRORREACTTIME 120 // seconds to make motorError permanent
#define WIFITIMEOUT (30 * 60)	// reboot after this time in seconds

static int err;
static bool noSensorsReceived;

void checkWifi(void) {
	static bool onAir = false;
	static int sensorTimeoutCntr = 0;
	static int oldPingOkCntr;
	static int pingTimeoutCntr = 0;
	bool reboot = false;
	// if (connectStatus == CONNECT_READY)
	// 	onAir = true;
	// if (onAir) {

		if( oldPingOkCntr != pingOKCntr) { // from pingTask
			oldPingOkCntr = pingOKCntr;
			pingTimeoutCntr = 0;
		}
		else
			pingTimeoutCntr+=1; 

		if (pingTimeoutCntr > WIFITIMEOUT) {
			systemInfo.pingTimeOuts++;
			ESP_LOGI(TAG, "Wifi ping failed, restarting system");
			reboot = true;
		}
		if (noSensorsReceived)
			sensorTimeoutCntr++;
		else
			sensorTimeoutCntr = 0;

		if (sensorTimeoutCntr > WIFITIMEOUT) {
			ESP_LOGI(TAG, "No sensors received, restarting system");
			systemInfo.sensorTimeOuts++;
			reboot = true;
		}
		if (reboot) {
			saveSettings();
			vTaskDelay(200 / portTICK_PERIOD_MS);
			esp_restart();
		}
//	}
}

// Task to check system status, e.g., temperature sensors and wifi
void systemCheckTask(void *pvParameters) {
	int nrSensors;
	int motorErrorTimer = MOTORERRORREACTTIME;
	int motorError = 0;
	bool motorErrorAccepted = false;

	xTaskCreate(pingTask, "pingTask", configMINIMAL_STACK_SIZE * 5, NULL, 1, NULL);

	while (1) {
		checkWifi();
		err = 0;
		if (updateStatus == UPDATE_BUSY)
			D1color = COLOR_YELLOW;
		else {
			switch ((int)connectStatus) {
			case CONNECTING:
				onBoardColor = COLOR_RED;
				D1color = COLOR_RED;
				break;

			case WPS_ACTIVE:
				onBoardFlash = true;
				D1Flash = true;
				onBoardColor = COLOR_BLUE;
				D1color = COLOR_BLUE;
				break;

			case CONNECTED:
				onBoardColor = COLOR_YELLOW;
				D1color = COLOR_YELLOW;
				break;

			case CONNECT_READY:
				onBoardFlash = false;
				D1Flash = false;
				onBoardColor = COLOR_GREEN;
				D1color = COLOR_GREEN;
				break;

			default:
				break;
			}
		}
		if (getMotorStatus(AFAN) != MOTOR_OK) {
			//	snprintf(tempMessage + strlen(tempMessage), BUFSIZE, "AfvoerVentilator fout\n");
			err = 2;
		}
		if (getMotorStatus(TFAN) != MOTOR_OK) {
			//	snprintf(tempMessage + strlen(tempMessage), BUFSIZE, "ToevoeVentilator fout\n");
			err = 1;
		}
		if (err == 0) {
			motorErrorTimer = MOTORERRORREACTTIME;
			motorErrorAccepted = false;
		}

		else {
			if (motorErrorTimer <= 0)
				motorError = err;
			else
				motorErrorTimer--;
		}
		if (motorError) {
			if (motorErrorAccepted)
				err = 0;
			else {
				err = motorError;	// make motorError permanent
				if (keysRT & PB1) { // clear
					motorError = 0;
					motorErrorTimer = MOTORERRORREACTTIME;
					motorErrorAccepted = true;
				}
			}
		}
		//		memset(tempMessage, 0, BUFSIZE);
		if (binnenTemperatuur == ERRORTEMP) {
			//	snprintf(tempMessage, BUFSIZE, "Binnentemperatuursensor fout\n");
			err = 3;
		}
		if (buitenTemperatuur == ERRORTEMP) {
			//	snprintf(tempMessage + strlen(tempMessage), BUFSIZE, "Buitentemperatuursensor fout\n");
			err = 4;
		}
		if (connectStatus == CONNECT_READY) { // only check when wifi
			nrSensors = 0;
			for (int n = 0; n < NR_SENSORS; n++) {
				if (sensorInfo[n].status == SENSORSTATUS_OK)
					nrSensors++;
			}
			if (nrSensors < userSettings.nrSensors) {
				// snprintf(tempMessage + strlen(tempMessage), BUFSIZE, "Sensor fout\n");
				err = 5;
				if (nrSensors == 0)
					noSensorsReceived = true;
				else
					noSensorsReceived = false;
			}

			for (int n = 1; n < userSettings.nrSensors; n++) { // sensor[0] is reference , skip
				if ((sensorInfo[n].CO2val < 300) || (sensorInfo[n].CO2val > 20000))
					err = 5;
			}
		}

		D2nrFlashes = err;
		switch (err) {
		case 0:
			if (brinkOff) {
				D2color = COLOR_YELLOW;
				D2Flash = true;
			} else {
				D2Flash = false;
				if (getRPM(AFAN) > 0)
					D2color = COLOR_GREEN;
				else
					D2color = COLOR_OFF;
			}
			break;
		default:
			D2color = COLOR_RED;
			// strcpy(errorMessage, tempMessage);
			// ESP_LOGE(TAG, "%s", errorMessage);
			break;
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
int getSystemError(void) { return err; }