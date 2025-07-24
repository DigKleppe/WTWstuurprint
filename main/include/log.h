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


#define MAXLOGVALUES (24 * 60)


typedef struct {
	unsigned long timeStamp;
	float co2[4];
	float temperature[4];
	float hum[4];
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