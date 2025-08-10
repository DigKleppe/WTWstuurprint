/*
 * log.h
 *
 *  Created on: Nov 3, 2023
 *      Author: dig
 */

#ifndef MAIN_INCLUDE_LOG_H_
#define MAIN_INCLUDE_LOG_H_

#include <stdint.h>
#include <time.h>


#define MAXLOGVALUES (24 * 60 / 5) // 24 hours, 5 minutes interval`
typedef struct {
	unsigned long timeStamp;
	float co2[5];
	float temperature[5];
	float hum[5];
	int maxCO2;
	int RPM;
} log_t;

log_t * initLogBuffer ( void); // in psRAM

extern int logRxIdx;
extern int logTxIdx;
extern unsigned long timeStamp;

int getAllLogsScript(char *pBuffer, int count);
int getNewLogsScript(char *pBuffer, int count);
int clearLogScript(char *pBuffer, int count);
void addToLog( log_t logValue);
void testLog( void) ;


#endif /* MAIN_INCLUDE_LOG_H_ */