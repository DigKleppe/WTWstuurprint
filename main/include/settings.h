/*
 * settings.h
 *
 *  Created on: Nov 30, 2017
 *      Author: dig
 */

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <stdbool.h>
#include "esp_system.h"
#include <time.h>
#include <sys/time.h>

#include "cgiScripts.h"
#include "motorControlTask.h"


#define MAX_STRLEN				 32
#define USERSETTINGS_CHECKSTR 	 "test-4"

typedef struct {
	char moduleName[MAX_STRLEN+1];
	motorSettings_t motorSettings[2];
	int bathRoomFanTime;
	int bathRoomFanMaxTime;
	int motorSpeedMin;
	int motorSpeedMax;
	int fixedSpeed[3]; 
	int CO2setpoint;
	float PIDp;
	float PIDi;
	int PIDmaxI;
	char checkstr[MAX_STRLEN+1];
}userSettings_t;



typedef struct {
	varType_t varType;
	int size;
	void * pValue;
	int minValue;
	int maxValue;
} settingsDescr_t;

extern bool settingsChanged;
extern userSettings_t userSettings;

#ifdef __cplusplus
extern "C" {
#endif
	esp_err_t saveSettings( void);
	esp_err_t loadSettings( void);

#ifdef __cplusplus
}
#endif

#endif /* SETTINGS_H_ */
