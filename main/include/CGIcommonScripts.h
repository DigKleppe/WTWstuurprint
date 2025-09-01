#ifndef CGICOMMONSCRIPTS
#define CGICOMMONSCRIPTS

#include "cgiScripts.h"

int getSettingsScript(char *pBuffer, int count);
int getAdvSettingsScript(char *pBuffer, int count);
int getCGItable (const CGIdesc_t *descr, char *pBuffer, int count);
int getCommonInfoScript (char *pBuffer, int count);
int getFanInfoScript(char *pBuffer, int count);
int setUserDefaultsScript(char *pBuffer, int count);
int setAdvUserDefaultsScript(char *pBuffer, int count);
int checkUpdatesScript(char *pBuffer, int count);
int forgetWifiScript(char *pBuffer, int count);

#endif