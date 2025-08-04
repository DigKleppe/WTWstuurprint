
#include "cgiScripts.h"
#include "settings.h"
#include "wifiConnect.h"
#include <string.h>

#include "esp_log.h"
const char * TAG = "commonscripts";

#include "esp_log.h"
extern int scriptState;


const CGIdesc_t writeVarDescriptorTable[] = {
    {"Badkamerventilatie nalooptijd ( min)", &userSettings.bathRoomFanTime, INT, 1},
    {"Badkamerventilatie max nalooptijd ( min) ", &userSettings.bathRoomFanMaxTime, INT, 1},
    {"Minimum toerental (%%)", &userSettings.motorSpeedMin, INT, 1},
    {"Maximum toerental (%%)", &userSettings.motorSpeedMax, INT, 1},
    {"Toerental zonder sensoren (%%)", &userSettings.fixedSpeed[0], INT, 1},
    {"Toerental stand 2 schakelaars (%%) ", &userSettings.fixedSpeed[1], INT, 1},
    {"Toerental stand 3 schakelaars {%%)", &userSettings.fixedSpeed[2], INT, 1},
    {"CO2 grenswaarde (ppm)", &userSettings.CO2setpoint, INT, 1},
    {"PID P waarde", &userSettings.PIDp, FLT, 1},
    {"PID I waarde", &userSettings.PIDi, FLT, 1},
    {"PID Imax waarde", &userSettings.PIDmaxI, INT, 1},
    {NULL, NULL, INT, 1},
};


#define NRWRITEVARDESCRIPTORS (sizeof (writeVarDescriptorTable) / sizeof (CGIdesc_t))

int getSettingsTableScript(char *pBuffer, int count) {
	int len = 0;
    const CGIdesc_t * descr = writeVarDescriptorTable;
	switch (scriptState) {
	case 0:
		scriptState++;
        len = sprintf(pBuffer,"Parameter, Waarde,Stel in\n");
        do {
            len += sprintf(pBuffer + len, "%s\n",descr++->name);
        } while (*descr->name != NULL) ;
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

void parseCGIWriteData(char *buf, int received) {

	bool save = false;

	if (strncmp(buf, "forgetWifi", 10) == 0) {
		ESP_LOGI(TAG, "Wifisettings erased");
		strcpy(wifiSettings.SSID, "xx");
		strcpy(wifiSettings.pwd, "xx");
		saveSettings();
		esp_restart();
    }

	if (strncmp(buf, "setVal:", 7) == 0) { // calvalues are written , in sensirionTasks write these to SCD30
		if (readActionScript(&buf[7], writeVarDescriptorTable, NRWRITEVARDESCRIPTORS)) {
			save = true;
		}
	}
	if (save)
		saveSettings();}
