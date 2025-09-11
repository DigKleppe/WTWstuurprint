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
#define USERSETTINGS_CHECKSTR "test"
#define ADVUSERSETTINGS_CHECKSTR "test2"

typedef struct {
	char moduleName[MAX_STRLEN + 1];
	int CO2setpoint;
	int bathRoomFanTime;
	int bathRoomFanMaxTime;
	int motorSpeedMin;
	int motorSpeedMax;
	int fixedSpeed[3];
	int MinBuitenTemperatuurbypass;
	int MinBinnenTemperatuurbypass;
	int nrSensors;
	char checkstr[MAX_STRLEN + 1];
} userSettings_t;

typedef struct {
	float CO2PIDp;
	float CO2PIDi;
	int CO2PIDmaxI;
	motorSettings_t motorSettings[2];
	float motorPIDp;
	float motorPIDi;
	int motorPIDmaxI;
	float buitenTemperatuurOffset;
	float binnenTemperatuurOffset;
	int fixedIPdigit;
	char checkstr[MAX_STRLEN + 1];
} advancedSettings_t;


typedef struct {
	varType_t varType;
	int size;
	void *pValue;
	int minValue;
	int maxValue;
} settingsDescr_t;

extern bool settingsChanged;
extern userSettings_t userSettings;
extern advancedSettings_t advSettings;

#ifdef __cplusplus
extern "C" {
#endif
esp_err_t saveSettings(void);
esp_err_t loadSettings(void);
void setUserDefaults(void);
void setAdvDefaults(void);

#ifdef __cplusplus
}
#endif

#endif /* SETTINGS_H_ */
