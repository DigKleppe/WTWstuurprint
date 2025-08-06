#include "log.h"


#define NR_SENSORS          5    // 0 = reference sensor 
#define SENSOR_TIMEOUT      60  // seconds
#define _ERRORVALUE         9999 
int getRTMeasValuesScript(char *pBuffer, int count);
int getSensorInfoScript(char *pBuffer, int count);

void sensorTask(void *pvParameters);

int printLog(log_t *logToPrint, char *pBuffer, int idx);
int printLog(log_t *logToPrint, char *pBuffer); 

int getMaXCOValue ( void);  

typedef enum { SENSORSTATUS_NOTPRESENT,SENSORSTATUS_OK, SENSORSTATUS_NOCOMM, SENSORSTATUS_ERROR} sensorStatus_t;
typedef struct {
    sensorStatus_t status;
    int timeoutTmr;
    int CO2val;
    float temperature;
    int RH;
    int rssi;
    uint32_t messageCntr;
} sensorInfo_t;

extern sensorInfo_t sensorInfo[NR_SENSORS];



