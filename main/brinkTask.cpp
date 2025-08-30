/*
 * brinkTask.cpp
 *
 *  Created on: aug 2, 2025
 *      Author: dig
 */

#define LOG_LOCAL_LEVEL ESP_LOG_INFO // ESP_LOG_ERROR
#include "esp_log.h"

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include <string.h>

#include "keyDefs.h"
#include "keys.h"
#include "pid.h"
#include "settings.h"

#include "motorControlTask.h"
#include "sensorTask.h"
#include "temperatureSensorTask.h"
#include "udpClient.h"
#include "wifiConnect.h"

static const char *TAG = "brinkTask";

// #define MAXRPM 2900 //  2300 for R3G140-AW05-12 EBM-Papst, measured on old PCB: 2940
// #define MINRPM 750
// typedef enum { AFAN, TFAN, BOTHFANS } motorID_t;
// typedef enum { MOTOR_K, MOTOR_FAIL } motorStatus_t;
// int aanvoerTemperatuur = -10;
// int afVoerTemperatuur = 10;
// int co2 = 1200;
// void setRPMpercent(motorID_t id, int percent) {};
// int getMaXCOValue(void) { return co2; }

#define OUTPUT_BRINKON GPIO_NUM_15
#define OUTPUT_BYPASS GPIO_NUM_9
#define FREEZETEMPMIN 		-20
#define CO2POSTTIME 		(15*60)	// seconds 
#define	CO2POSTTIMESPEED 	1 // % speed in posttime
#define ONTIME				60 // after 60 seconds inactive switch off motors

int overrideLevel; // CGI 0=  permanent off!
int manualLevel;   // switches
int bathRoomTimer;
int bathRoomMaxTimer = -1;
int onTimer;
int overrideTimer;

extern int scriptState;



// als aanvoertemperatuur < 0 wordt de aanvoer beperkt en een onbalans gecreeerd
// bij - 20 maximale onbalans
int antifreeze(int percent) {
	float derate = 100;
	if (buitenTemperatuur < 0) {
		derate = 100.0 - (100.0 * (FREEZETEMPMIN - buitenTemperatuur)) / FREEZETEMPMIN;
		ESP_LOGI(TAG, " derate %1.2f", derate);
		percent = (percent * derate) / 100.0;
	}
	return percent;
}

void brinkTask(void *pvParameters) {
	int udpTimer = 2;
	int CO2postTimer = 0;
	int CO2Value = 0;
	char buf[64];
	Pid pid;
	int tempRPMToevoer = 0, tempRPMafvoer = 0;
	gpio_set_direction(OUTPUT_BRINKON, GPIO_MODE_OUTPUT);
	gpio_set_drive_capability(OUTPUT_BRINKON, GPIO_DRIVE_CAP_3);

	gpio_set_direction(OUTPUT_BYPASS, GPIO_MODE_OUTPUT);

	while (1) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
		tempRPMToevoer = 0;
		tempRPMafvoer = 0;

	//	pid.setImaxImin(userSettings.CO2PIDmaxI, -userSettings.CO2PIDmaxI);
		pid.setImaxImin(userSettings.CO2PIDmaxI, -userSettings.CO2PIDmaxI/2);
		pid.setPIDValues(userSettings.CO2PIDp, userSettings.CO2PIDi, 0);
		pid.setDesiredValue(userSettings.CO2setpoint);

		bool switchIn = false;
		if (keysRT & SB2) { // badkamer
			switchIn = true;
			if (manualLevel < 3) {
				manualLevel = 1;
			}
		}
		if (keysRT & SB3) {
			switchIn = true;
			manualLevel = 2;
		}
		if (switchIn) { // switch badkamer in
			if (userSettings.bathRoomFanTime > 0)
				bathRoomTimer = userSettings.bathRoomFanTime * 60; // as long as switch is in
			else
				bathRoomTimer = -1;

			if (bathRoomMaxTimer < 0)
				bathRoomMaxTimer = userSettings.bathRoomFanMaxTime * 60; // once

		} else {
			bathRoomMaxTimer = -1; // ready to start
			if (bathRoomTimer <= 0) // switch off direct without bathroomTimer
				manualLevel = 0;
		}

		if (bathRoomTimer > 0) { // timer running. leave manualLevel
			bathRoomTimer--;
			if (bathRoomTimer == 0) {
				manualLevel = 0; //
				bathRoomTimer = -1;
				bathRoomMaxTimer = -1;
			}
		}

		if (bathRoomMaxTimer > 0) {
			bathRoomMaxTimer--;
			if (bathRoomMaxTimer == 0)
				manualLevel = 0; // forgot to switch off SB2 SB3
		}

		if (keysRT & SK2) // keuken
			if(bathRoomTimer <= 0)
				manualLevel = 1;

		if (keysRT & SK3)
			manualLevel = 2;

		CO2Value = getMaXCOValue(); // from sensors

		if (CO2Value > 0) { // minstens 1 sensor actief?
			tempRPMafvoer = -pid.update(CO2Value);
			if (tempRPMafvoer < 0) { // below CO2 setpoint
				if (CO2postTimer  > 0 )
					tempRPMafvoer = CO2POSTTIMESPEED;
				if ( tempRPMafvoer < userSettings.motorSpeedMin)
					tempRPMafvoer = userSettings.motorSpeedMin; 
			}
			if ( CO2Value > userSettings.CO2setpoint) {
				CO2postTimer = CO2POSTTIME;
			}
			tempRPMToevoer = tempRPMafvoer;
		} else {
			tempRPMafvoer = userSettings.fixedSpeed[0]; // oude stand 1
			tempRPMToevoer = tempRPMafvoer;
		}

		if (CO2postTimer >0)
			CO2postTimer--;

		if (manualLevel > 0) { // set by switches
			if (userSettings.fixedSpeed[manualLevel] > tempRPMafvoer) {
				tempRPMafvoer = userSettings.fixedSpeed[manualLevel];
				tempRPMToevoer = userSettings.fixedSpeed[manualLevel];
			}
		}

		if (tempRPMafvoer == 0) { // vorstbeveiliging als aanvoertemp < 0 en stilstaande motoren
			if (binnenTemperatuur < -2)
				tempRPMafvoer = 1; // laagste toerental
		} else
			tempRPMToevoer = antifreeze(tempRPMToevoer); // derate toevoer indien nodig

		switch (overrideLevel) { // from CGI website
		case 0:
			break;
		case 1:
		case 2:
			tempRPMafvoer = userSettings.fixedSpeed[overrideLevel - 1];
			tempRPMToevoer = userSettings.fixedSpeed[overrideLevel - 1];
			if (overrideTimer-- <= 0)
				overrideLevel = 0;
			break;

		case -1: // permanent off!
			tempRPMToevoer = 0;
			tempRPMafvoer = 0;
			break;
		default:
			break;
		}
		setRPMpercent(TFAN, tempRPMToevoer);
		setRPMpercent(AFAN, tempRPMafvoer);

		if ((!userSettings.motorSettings[TFAN].isCalibrated) || (!userSettings.motorSettings[AFAN].isCalibrated)) // motors on to calibrate
			gpio_set_level(OUTPUT_BRINKON, 1);																	
		else {
			if ( onTimer) {
				onTimer--;
				gpio_set_level(OUTPUT_BRINKON, 1); // turn power to motors on if needed
			}
			else
				gpio_set_level(OUTPUT_BRINKON, 0); // turn power to motors off

			if (tempRPMafvoer > 0)  
				onTimer = ONTIME;
		}
		if (udpTimer)
			udpTimer--;
		else {
			sprintf(buf, "maxCO2:%d Toe: %d Af:%d BRtimer:%d BRmaxTmr:%d \n\r", CO2Value, tempRPMToevoer, tempRPMafvoer, bathRoomTimer, bathRoomMaxTimer);
			UDPsendMssg(5002, (void *)buf, strlen(buf));
		//	ESP_LOGI(TAG, "%s", buf);
			udpTimer = 2;
		}

		if ((binnenTemperatuur > userSettings.MaxBuitenTemperatuurbypass) && (binnenTemperatuur > userSettings.MinBuitenTemperatuurbypass)) {
			if (buitenTemperatuur < binnenTemperatuur) {
				gpio_set_level(OUTPUT_BYPASS, 1);
			//	ESP_LOGI(TAG, "bypass on");
			} else
				gpio_set_level(OUTPUT_BYPASS, 0);
		} else
			gpio_set_level(OUTPUT_BYPASS, 0);
	}
}

// set from CGI , -1 is permanent off!
void setManualLevel(int level) {
	overrideLevel = level;
	switch (level) {
	case 0:
	case 1:
		overrideTimer = 60 * 60; // 1 hour max
		break;

	default:
		break;
	};
}
