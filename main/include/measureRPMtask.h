#ifndef MEASURERPMTASK_H
#define MEASURERPMTASK_H
#include "motorControlTask.h"

void measureRPMtask(void *pvParameters);
int getRPM( motorID_t id);
int getAVGRPM(motorID_t id);

#endif