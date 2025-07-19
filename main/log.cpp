/*
 * log.cpp
 *
 *  Created on: Nov 3, 2023
 *      Author: dig
 */
#include "log.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <esp_heap_caps.h>

int printLog(log_t *logToPrint, char *pBuffer);
extern int scriptState;

int logRxIdx;
int logTxIdx;

static log_t measLog[ MAXLOGVALUES];

// log_t * measLog;

// log_t * initLogBuffer ( void) {
// 	void *p = heap_caps_malloc( MAXLOGVALUES * sizeof ( log_t),  MALLOC_CAP_SPIRAM);
// 	measLog = (log_t *)p;
// 	return measLog;
// }

void addToLog(log_t logValue) {
	measLog[logTxIdx] = logValue;
	measLog[logTxIdx].timeStamp = timeStamp;
	logTxIdx++;
	if (logTxIdx >= MAXLOGVALUES)
		logTxIdx = 0;
}

// reads all available data from log
// issued as first request.
static int blocks;
int getAllLogsScript(char *pBuffer, int count) {
	static uint32_t oldTimeStamp = 0;
	static int logsToSend = 0;
	int left, len = 0;
	int n;
	if (scriptState == 0) { // find oldest value in cyclic logbuffer
		logRxIdx = 0;
		blocks = 0;
		if (measLog[logRxIdx].timeStamp == 0)
			return 0; // empty

		oldTimeStamp = 0;
		for (n = 0; n < MAXLOGVALUES; n++) {
			if (measLog[logRxIdx].timeStamp < oldTimeStamp)
				break;
			else {
				oldTimeStamp = measLog[logRxIdx++].timeStamp;
			}
		}
		if (measLog[logRxIdx].timeStamp == 0) { // then log not full
			// not written yet?
			logRxIdx = 0;
			logsToSend = n;
		} else
			logsToSend = MAXLOGVALUES;
		scriptState++;
	}
	if (scriptState == 1) { // send complete buffer
		if (logsToSend) {
			len = 0;
			blocks++;
	
			do {
				len += printLog(&measLog[logRxIdx], pBuffer + len);
				logRxIdx++;
				if (logRxIdx >= MAXLOGVALUES)
					logRxIdx = 0;
				left = count - len;
				logsToSend--;

			} while (logsToSend && (left > (len + 50))); 
		//	printf(" Sending block %d, logsToSend: %d  left:%d\r\n", blocks, logsToSend,  left);
		}
	}
	return len;
}

// these functions only work for one user!

int getNewLogsScript(char *pBuffer, int count) {

	int left, len = 0;
	if (logRxIdx != (logTxIdx)) {  // something to send?
		do {
			len += printLog(&measLog[logRxIdx], pBuffer + len);
			logRxIdx++;
			if (logRxIdx >= MAXLOGVALUES)
				logRxIdx = 0;
			left = count - len;

		} while ((logRxIdx != logTxIdx) && (left > 40));

	}
	return len;
}

int clearLogScript(char *pBuffer, int count) {
	if (scriptState == 0) { // find oldest value in cyclic logbuffer
		logRxIdx = 0;
		logTxIdx = 0;
		memset(&measLog, 0, sizeof( measLog));
		strcpy(pBuffer, "OK");
		scriptState++;
		return 3;
	}
	return 0;
}

// void testLog( void) {
// 	log_t testlog;
// 	testlog.unit[0]= 'V';
// 	testlog.unit[1]= 0;
// 	for ( int n = 0; n < MAXLOGVALUES; n++) {
// 		testlog.measValue = sin ((float)n / 100);
// 		timeStamp++;
// 		addToLog(testlog);
// 	}
// }
