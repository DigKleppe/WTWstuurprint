#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdbool.h>

#include "esp_log.h"

#include "brinkTask.h"
#include "ledTask.h"
#include "measureRPMtask.h"
#include "motorControlTask.h"
#include "sensorTask.h"
#include "settings.h"
#include "temperatureSensorTask.h"
#include "updateTask.h"
#include "wifiConnect.h"

static const char *TAG = "systemCheck";

static int err;

// Task to check system status, e.g., temperature sensors
void systemCheckTask(void *pvParameters) {
	int nrSensors;

	while (1) {
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

			case IP_RECEIVED:
				onBoardFlash = false;
				D1Flash = false;
				onBoardColor = COLOR_GREEN;
				D1color = COLOR_GREEN;
				break;

			case CONNECTED:
				onBoardColor = COLOR_YELLOW;
				D1color = COLOR_YELLOW;
				break;

			default:
				break;
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

		nrSensors = 0;
		for (int n = 0; n < NR_SENSORS; n++) {
			if (sensorInfo[n].status == SENSORSTATUS_OK)
				nrSensors++;
		}
		if (nrSensors < userSettings.nrSensors) {
			// snprintf(tempMessage + strlen(tempMessage), BUFSIZE, "Sensor fout\n");
			err = 5;
		}
		if (!brinkOff) {
			if (getMotorStatus(AFAN) != MOTOR_OK) {
				//	snprintf(tempMessage + strlen(tempMessage), BUFSIZE, "AfvoerVentilator fout\n");
				err = 2;
			}
			if (getMotorStatus(TFAN) != MOTOR_OK) {
				//	snprintf(tempMessage + strlen(tempMessage), BUFSIZE, "ToevoeVentilator fout\n");
				err = 1;
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