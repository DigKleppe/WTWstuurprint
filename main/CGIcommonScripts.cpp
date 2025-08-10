
#include "cgiScripts.h"
#include "motorControlTask.h"
#include "sensorTask.h"
#include "settings.h"
#include "wifiConnect.h"
#include "softwareVersions.h"

#include <string.h>

#include "esp_log.h"
const char *TAG = "commonscripts";

#include "esp_log.h"
extern int scriptState;
static int rssi;

const char firmWareVersion[] = FIRMWARE_VERSION;

const CGIdesc_t writeVarDescriptorTable[] = {
	{"Badkamerventilatie nalooptijd (min)", &userSettings.bathRoomFanTime, INT, 1},
	{"Badkamerventilatie max nalooptijd (min) ", &userSettings.bathRoomFanMaxTime, INT, 1},
	{"Minimum toerental (%%)", &userSettings.motorSpeedMin, INT, 1},
	{"Maximum toerental (%%)", &userSettings.motorSpeedMax, INT, 1},
	{"Toerental zonder sensoren (%%)", &userSettings.fixedSpeed[0], INT, 1},
	{"Toerental stand 2 schakelaars (%%) ", &userSettings.fixedSpeed[1], INT, 1},
	{"Toerental stand 3 schakelaars {%%)", &userSettings.fixedSpeed[2], INT, 1},
	{"CO2 grenswaarde (ppm)", &userSettings.CO2setpoint, INT, 1},
	{"PID P waarde", &userSettings.PIDp, FLT, 1},
	{"PID I waarde", &userSettings.PIDi, FLT, 1},
	{"PID Imax waarde", &userSettings.PIDmaxI, INT, 1},
	{"Min buitentemperatuur bypass", &userSettings.MinBuitenTemperatuurbypass, INT, 1},
	{"Max binnentemperatuur bypass", &userSettings.MaxBuitenTemperatuurbypass, INT, 1},
	{NULL, NULL, INT, 1},
};

const CGIdesc_t commonInfoTable[] = {
	{ "firmwareversie", (void *) firmWareVersion, STR, 1},
	{ "SPIFFS versieVersion", wifiSettings.SPIFFSversion, STR, 1},
	{ "RSSI", (void*) &rssi, INT, 1},
	{NULL, NULL, INT , 1}
};


int getCGItable (const CGIdesc_t *descr, char *pBuffer, int count) {
	int len = 0;
	do {
		len += sprintf(pBuffer + len, "%s,", descr->name);
		switch (descr->type) {
		case INT:
			len += sprintf(pBuffer + len, "%d\n", *(int *)descr->pValue);
			break;
		case FLT:
			len += sprintf(pBuffer + len, "%1.2f\n,", *(float *)descr->pValue);
			break;
		case STR:
			len += sprintf(pBuffer + len, "%s\n", (char *) descr->pValue);
			break;

		default:
			break;
		}
		descr++;
	} while (descr->name != NULL);
	return len;
}

int getSettingsScript(char *pBuffer, int count) {
	int len = 0;
	switch (scriptState) {
	case 0:
		scriptState++;
		len = sprintf(pBuffer, "Parameter, Waarde,Stel in\n");
		len += getCGItable( writeVarDescriptorTable, pBuffer+len, count);
		return len;
		break;
	default:
		break;
	}
	return 0;
}

int getCommonInfoScript(char *pBuffer, int count) {
	int len = 0;
	switch (scriptState) {
	case 0:
		scriptState++;
		len = sprintf(pBuffer, "Parameter, Waarde\n");
		rssi = getRssi();
		len += getCGItable( commonInfoTable, pBuffer+len, count);
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

#define NRWRITEVARDESCRIPTORS (sizeof(writeVarDescriptorTable) / sizeof(CGIdesc_t))

void parseCGIWriteData(char *buf, int received) {
	bool save = false;

	if (strncmp(buf, "forgetWifi", 10) == 0) {
		ESP_LOGI(TAG, "Wifisettings erased");
		strcpy(wifiSettings.SSID, "xx");
		strcpy(wifiSettings.pwd, "xx");
		saveSettings();
		esp_restart();
	}
	if (strncmp(buf, "setVal:", 7) == 0) { // values are written , in sensirionTasks write these to SCD30
		if (readActionScript(&buf[7], writeVarDescriptorTable, NRWRITEVARDESCRIPTORS)) {
			save = true;
		}
	}
	if (save)
		saveSettings();
}
