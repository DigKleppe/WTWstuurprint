/*
 * brinkTask.cpp
 *
 *  Created on: aug 2, 2025
 *      Author: dig
 */

#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"
#include "freertos/queue.h"

#include "lwip/api.h"
#include "lwip/arch.h"
#include "lwip/mem.h"
#include "lwip/opt.h"
#include "settings.h"

#include <string.h>

#include "driver/gpio.h"
#include "keyDefs.h"
#include "keys.h"
#include "wifiConnect.h"
#include "sensorTask.h"
#include "pid.h"

extern time_t now;
struct tm timeinfo;

#define OUTPUT_BRINKON GPIO_NUM_33
#define CO2PERIOD 300 // sec  must be larger than interval transmitters

#define IMAX 10.0
#define P 1.0
#define I 1.0
#define D 0

extern int CO2max; // received from udp server

extern QueueHandle_t udpclientMssgbox;

int co2test = 100;
int manualLevel;
int brinkLevel = 0;
int lastCO2Value = 0;

int bathRoomTimer;
int bathRoomMaxTimer;
extern int scriptState;

void startBathroomTimer(void) {
	if (bathRoomMaxTimer < 0)
		bathRoomMaxTimer = userSettings.bathRoomFanMaxTime * 60; // once
	bathRoomTimer = userSettings.bathRoomFanTime * 60;			 // as long as switch is in
}

void brinkTask(void *pvParameters) {
	int udpTimer = 20;
	int debounce = 0;
	int testValue = 0;
    int CO2Value = 0;
	char buf[64];
    Pid pid;

	gpio_set_direction(OUTPUT_BRINKON, GPIO_MODE_OUTPUT);

    pid.setImaxImin(IMAX, -IMAX);
	pid.setPIDValues(I, D); // todo usersettings
    pid.setDesiredValue( 1100) ;


	while (1) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);






		CO2timer--;
		if (CO2timer == 0) {
			lastCO2Value = CO2max;
			CO2max = 0;
			CO2timer = CO2PERIOD;
		}
		if (lastCO2Value > 0) // highest value in CO2PERIOD
			testValue = lastCO2Value;
		else
			testValue = CO2max;

        
        printf("\nbrinklev %d ", brinkLevel);

		manualLevel = 0;
		bool switchIn = false;
		if (keysRT & SB2) { // badkamer
			switchIn = true;
			startBathroomTimer();
			if (manualLevel < 3) {
				manualLevel = 2;
			}
		}
		if (keysRT & SB3) {
			switchIn = true;
			manualLevel = 3;
			startBathroomTimer();
		}
		if (switchIn) // switch badkamer in
			startBathroomTimer();
		else
			bathRoomMaxTimer = -1;

		if (bathRoomTimer < 0)
			manualLevel = 0;
		else {
			if (bathRoomTimer > 0) // timer running. leave manualLevel
				bathRoomTimer--;
			else {
				manualLevel = 0; //
				bathRoomTimer = -1;
			}
		}
		if (bathRoomMaxTimer > 0)
			bathRoomMaxTimer--;
		else {				 // forgot to switch off SB2 SB3
			manualLevel = 0; //
		}

		if (keysRT & SK2) // keuken
            if ( manualLevel == 0)
			    manualLevel = 2;

		if (keysRT & SK3)
			manualLevel = 3;

		if (manualLevel > 0)
			brinkLevel = manualLevel;

		printf(" Final lev: %d ", brinkLevel);

		if (brinkLevel > 0) {
			gpio_set_level(OUTPUT_BRINKON, 1); // turn power to motors on
			printf("\nbrink on ");
		} else
			gpio_set_level(OUTPUT_BRINKON, 0);

		if (udpTimer)
			udpTimer--;
		else {
			sprintf(buf, "Brink: maxCO2Value:%d level:%d timer:%d\n\r", lastCO2Value, brinkLevel, bathRoomTimer);
			UDPsendMssg(5002, (void *)buf, strlen(buf));
			udpTimer = 20;
		}
	}
}

// set from CGI
void setManualLevel(int level) {
	manualLevel = level;
	switch (level) {
	case 0:
		bathRoomTimer = 0; // timer off
		bathRoomMaxTimer = 0;
		brinkLevel = 0;
		break;

	default:
		break;
	}
}
