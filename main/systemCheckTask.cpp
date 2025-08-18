#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdbool.h>

#include "esp_log.h"

#include "ledTask.h"
#include "motorControlTask.h"
#include "sensorTask.h"
#include "settings.h"
#include "temperatureSensorTask.h"
#include "wifiConnect.h"

static const char *TAG = "systemCheck";

static int err; 

// Task to check system status, e.g., temperature sensors
void systemCheckTask(void *pvParameters) {
	int nrSensors;

	while (1) {
		err = 0;
		switch ((int)connectStatus) {
		case CONNECTING:
			//	ESP_LOGI(TAG, "CONNECTING");
			onBoardColor = COLOR_RED;
            D1color = COLOR_RED;
			break;

		case WPS_ACTIVE:
			//	ESP_LOGI(TAG, "WPS_ACTIVE");
			onBoardFlash = true;
            D1Flash = true;
            onBoardColor = COLOR_BLUE; 
           	D1color = COLOR_BLUE;
			break;

		case IP_RECEIVED:
			//     ESP_LOGI(TAG, "IP_RECEIVED");
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
			ESP_LOGI(TAG, "default");
			break;
		}


//		memset(tempMessage, 0, BUFSIZE);
		if (binnenTemperatuur == ERRORTEMP) {
		//	snprintf(tempMessage, BUFSIZE, "Binnentemperatuursensor fout\n");
			err = 1;
		}
		if (buitenTemperatuur == ERRORTEMP) {
		//	snprintf(tempMessage + strlen(tempMessage), BUFSIZE, "Buitentemperatuursensor fout\n");
			err = 1;
		}

		nrSensors = 0;
		for (int n = 0; n < NR_SENSORS; n++) {
			if (sensorInfo[n].status == SENSORSTATUS_OK)
				nrSensors++;
		}
		if (NR_SENSORS < userSettings.nrSensors) {
			//snprintf(tempMessage + strlen(tempMessage), BUFSIZE, "Sensor fout\n");
			err = 2;
		}

		if (getMotorStatus(AFAN) != MOTOR_OK) {
		//	snprintf(tempMessage + strlen(tempMessage), BUFSIZE, "AfvoerVentilator fout\n");
			err = 3;
		}
		if (getMotorStatus(TFAN) != MOTOR_OK) {
		//	snprintf(tempMessage + strlen(tempMessage), BUFSIZE, "ToevoeVentilator fout\n");
			err = 3;
		}
		switch (err) {
		case 0:
			D2color = COLOR_GREEN;
			D2Flash = false;
		//	memset(errorMessage, 0, sizeof(errorMessage));
			break;
		default:
			D2Flash = true;
			D2color = COLOR_RED;
			//strcpy(errorMessage, tempMessage);
			//ESP_LOGE(TAG, "%s", errorMessage);
			break;
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
int getSystemError(void) {
    return err;
}