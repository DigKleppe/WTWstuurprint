
#include "cgiScripts.h"
#include "motorControlTask.h"
#include "sensorTask.h"
#include "settings.h"
#include "softwareVersions.h"
#include "temperatureSensorTask.h"
#include "wifiConnect.h"
#include "updateTask.h"

#include "esp_netif_ip_addr.h"

#include <string.h>

#include "esp_log.h"
const char *TAG = "commonscripts";

#include "esp_log.h"
extern int scriptState;
static int rssi;

const char firmWareVersion[] = FIRMWARE_VERSION;

const CGIdesc_t writeVarDescriptorTable[] = {
	{"Badkamerventilatie nalooptijd (min)", &userSettings.bathRoomFanTime, INT, 1},
	{"Badkamerventilatie maximumtijd (min)", &userSettings.bathRoomFanMaxTime, INT, 1},
	{"Minimum toerental (%)", &userSettings.motorSpeedMin, INT, 1},
	{"Maximum toerental (%)", &userSettings.motorSpeedMax, INT, 1},
	{"Toerental zonder sensoren (%)", &userSettings.fixedSpeed[0], INT, 1},
	{"Toerental stand 2 schakelaars (%)", &userSettings.fixedSpeed[1], INT, 1},
	{"Toerental stand 3 schakelaars (%)", &userSettings.fixedSpeed[2], INT, 1},
	{"CO2 grenswaarde (ppm)", &userSettings.CO2setpoint, INT, 1},
	{"Aantal sensoren", &userSettings.nrSensors, INT, 1},
	{"Min buitentemperatuur bypass", &userSettings.MinBuitenTemperatuurbypass, INT, 1},
	{"Max binnentemperatuur bypass", &userSettings.MaxBuitenTemperatuurbypass, INT, 1},
	{"CO2 PID P waarde", &userSettings.CO2PIDp, FLT, 1},
	{"CO2 PID I waarde", &userSettings.CO2PIDi, FLT, 1},
	{"CO2 PID Imax waarde", &userSettings.CO2PIDmaxI, INT, 1},
//	{"Motor PID P waarde", &userSettings.motorPIDp, FLT, 1},
//	{"Motor PID I waarde", &userSettings.motorPIDi, FLT, 1},
//	{"Motor PID Imax waarde", &userSettings.motorPIDmaxI, INT, 1},
	{"Offset buitentemperatuur", &userSettings.buitenTemperatuurOffset, FLT, 1},
	{"Offset binnentemperatuur", &userSettings.binnenTemperatuurOffset, FLT, 1},
	{"Laaste IPdigit",&userSettings.fixedIPdigit, INT, 1}, 
	{NULL, NULL, INT, 1},
};

const CGIdesc_t commonInfoTable[] = {
	{"firmwareversie", (void *)firmWareVersion, STR, 1},
	{"SPIFFS versie", wifiSettings.SPIFFSversion, STR, 1},
	{"RSSI", (void *)&rssi, INT, 1},
	{"Binnentemperatuur (°C)", &binnenTemperatuur, FLT, 1},
	{"Buitentemperatuur (°C)", &buitenTemperatuur, FLT, 1},
	{NULL, NULL, INT, 1}
};

int getCGItable(const CGIdesc_t *descr, char *pBuffer, int count) {
	int len = 0;
	do {
		len += sprintf(pBuffer + len, "%s,", descr->name);
		switch (descr->type) {
		case INT:
			len += sprintf(pBuffer + len, "%d\n", *(int *)descr->pValue);
			break;
		case FLT:
			len += sprintf(pBuffer + len, "%1.2f\n", *(float *)descr->pValue);
			break;
		case STR:
			len += sprintf(pBuffer + len, "%s\n", (char *)descr->pValue);
			break;

		case IPADDR: {
			esp_ip4_addr_t addr = (esp_ip4_addr_t) * (uint32_t *)descr->pValue;
			len += sprintf(pBuffer + len, IPSTR, IP2STR(&addr));
			len += sprintf(pBuffer + len, "\n");
		} break;

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
		len += getCGItable(writeVarDescriptorTable, pBuffer + len, count);
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
		len += getCGItable(commonInfoTable, pBuffer + len, count);
		return len;
		break;
	default:
		break;
	}
	return 0;
}

int getFanInfoScript(char *pBuffer, int count) {
	int len = 0;
	switch (scriptState) {
	case 0:
		scriptState++;
		len = sprintf(pBuffer, "Parameter, Waarde\n");
		len += getCGItable(motorInfoDescriptorTable, pBuffer + len, count);
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

int setUserDefaultsScript(char *pBuffer, int count) {
	setUserDefaults();
	return 0;
}

int checkUpdatesScript(char *pBuffer, int count) {
	forceUpdate = true;
	return 0;
}


int forgetWifiScript(char *pBuffer, int count) {
	strcpy(wifiSettings.SSID, "xx");
	strcpy(wifiSettings.pwd, "xx");
	saveSettings();
	esp_restart();
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
		if (readActionScript(&buf[7], writeVarDescriptorTable, NRWRITEVARDESCRIPTORS) >= 0) {
			save = true;
		}
	}
	if (save)
		saveSettings();
}
