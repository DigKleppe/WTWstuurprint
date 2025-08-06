#ifndef MOTORCONTROL_H
#define MOTORCONTROL_H

#include "cgiScripts.h"
#include "../../components/pid/include/pid.h"

#define MAXRPM 2900 //  2300 for R3G140-AW05-12 EBM-Papst, measured on old PCB: 2940
#define MINRPM 750  
typedef enum { AFAN, TFAN, BOTHFANS} motorID_t;
typedef enum { MOTOR_K, MOTOR_FAIL} motorStatus_t;
void motorControlTask(void *pvParameters);
typedef struct {
    int desiredRPM;
    int actualRPM;
    motorStatus_t status;
    Pid pid;
} motor_t;

typedef struct {
    int minPWM;
    int maxPWM;
    bool isCalibrated;
} motorSettings_t;

extern motorSettings_t motorsettings[2];
void setRPM(motorID_t id, float rpm);
void setRPMpercent(motorID_t id, int percent);

extern const CGIdesc_t motorInfoDescriptorTable[];

#endif