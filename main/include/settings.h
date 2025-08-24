/*
 * settings.h
 *
 *  Created on: Nov 30, 2017
 *      Author: dig
 */

#ifndef SETTINGS_H_
#define SETTINGS_H_

#include "esp_system.h"
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>

#include "cgiScripts.h"
#include "motorControlTask.h"

#define MAX_STRLEN 32
#define USERSETTINGS_CHECKSTR "test-4"

typedef struct {
	char moduleName[MAX_STRLEN + 1];
	int CO2setpoint;
	float CO2PIDp;
	float CO2PIDi;
	int CO2PIDmaxI;
	int bathRoomFanTime;
	int bathRoomFanMaxTime;
	int motorSpeedMin;
	int motorSpeedMax;
	int fixedSpeed[3];
	motorSettings_t motorSettings[2];
	float motorPIDp;
	float motorPIDi;
	int motorPIDmaxI;
	int MinBuitenTemperatuurbypass;
	int MaxBuitenTemperatuurbypass;
	float buitenTemperatuurOffset;
	float binnenTemperatuurOffset;
	int nrSensors;
	int fixedIPdigit;
	char checkstr[MAX_STRLEN + 1];
} userSettings_t;

typedef struct {
	varType_t varType;
	int size;
	void *pValue;
	int minValue;
	int maxValue;
} settingsDescr_t;

extern bool settingsChanged;
extern userSettings_t userSettings;

#ifdef __cplusplus
extern "C" {
#endif
esp_err_t saveSettings(void);
esp_err_t loadSettings(void);
void setUserDefaults(void);

#ifdef __cplusplus
}
#endif

#endif /* SETTINGS_H_ */
