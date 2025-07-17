#include "log.h"


int getRTMeasValuesScript(char *pBuffer, int count);
// int getRTMeasValuesScript1(char *pBuffer, int count);
// int getRTMeasValuesScript2(char *pBuffer, int count);
// int getRTMeasValuesScript3(char *pBuffer, int count);
// int getRTMeasValuesScript4(char *pBuffer, int count);

void sensorTask(void *pvParameters);

int printLog(log_t *logToPrint, char *pBuffer, int idx);
int printLog(log_t *logToPrint, char *pBuffer); 