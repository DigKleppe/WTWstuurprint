/* 2300 rpm = max 600 hz*/

#define LOG_LOCAL_LEVEL ESP_LOG_INFO // ESP_LOG_ERROR
#include "esp_log.h"

#include "measureRPMtask.h"
#include "motorControlTask.h"
#include "pid.h"
#include "settings.h"
#include "temperatureSensorTask.h"

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/ledc.h"
#include "driver/pulse_cnt.h"
#include "esp_err.h"
#include "nvs_flash.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>
#include <stdio.h>

esp_err_t init_spiffs(void);
static const char *TAG = "MCTask";

extern int scriptState;

#define PID_INTERVAL 	100 // ms
#define RPMBIGSTEP 		200

// PWM
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT // Set duty resolution to 13 bits
#define LEDC_DUTY (4096)				// Set duty to 50%. (2 ** 13) * 50% = 4096
#define LEDC_FREQUENCY (1250)			// Frequency in Hertz
#define AV_PWM_PIN GPIO_NUM_16
#define TV_PWM_PIN GPIO_NUM_18



static motor_t motor[2];
static bool PWMisInitialized;

const int MAXDUTYCYCLE = ((2 << (LEDC_DUTY_RES - 1)) - 1);

// using ledc peripheral for PWM

static ledc_channel_config_t TV_PWMchannel = { // toevoer
	.gpio_num = TV_PWM_PIN,
	.speed_mode = LEDC_MODE,
	.channel = LEDC_CHANNEL_0,
	.intr_type = LEDC_INTR_DISABLE,
	.timer_sel = LEDC_TIMER,
	.duty = 0,
	.hpoint = 0,
	.sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
	.flags = {0}};

static ledc_channel_config_t AV_PWMchannel = { // afvoer
	.gpio_num = AV_PWM_PIN,
	.speed_mode = LEDC_MODE,
	.channel = LEDC_CHANNEL_1,
	.intr_type = LEDC_INTR_DISABLE,
	.timer_sel = LEDC_TIMER,
	.duty = 0,
	.hpoint = 0,
	.sleep_mode = LEDC_SLEEP_MODE_NO_ALIVE_NO_PD,
	.flags = {0}};

static ledc_channel_config_t *PWMchannelList[] = {&AV_PWMchannel, &TV_PWMchannel};

esp_err_t setPWMpercent(ledc_channel_config_t *channel, float percent) {
	esp_err_t err;
	if (percent > 100)
		percent = 100;
	if (percent < 0)
		percent = 0;
	int duty = MAXDUTYCYCLE * percent / 100.0;
	err = ledc_set_duty(LEDC_MODE, channel->channel, duty);
	if (!err)
		err = ledc_update_duty(LEDC_MODE, channel->channel);
	return err;
}

static void PWMinit(void) {
	// Prepare and then apply the LEDC PWM timer configuration
	ledc_timer_config_t ledc_timer = {.speed_mode = LEDC_MODE,
									  .duty_resolution = LEDC_DUTY_RES,
									  .timer_num = LEDC_TIMER,
									  .freq_hz = LEDC_FREQUENCY, // Set output frequency at 4 kHz
									  .clk_cfg = LEDC_AUTO_CLK};
	TV_PWMchannel.flags.output_invert = true;
	AV_PWMchannel.flags.output_invert = true;
	ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));
	ESP_ERROR_CHECK(ledc_channel_config(&AV_PWMchannel));
	ESP_ERROR_CHECK(ledc_channel_config(&TV_PWMchannel));
}

void setRPM(motorID_t id, float rpm) {
	static float oldrpm = 0;
	if (rpm > MAXRPM)
		rpm = MAXRPM;
	if (rpm < MINRPM)
		rpm = MINRPM;
	motor[id].desiredRPM = rpm;
	if (rpm != oldrpm) {
		ESP_LOGI(TAG, "RPM set to %f", rpm);
		oldrpm = rpm;
	}
}

void setRPMpercent(motorID_t id, int percent) {
	if (percent > 100)
		percent = 100;

	if (percent > 0) {
		setRPM(id, MINRPM + (float)percent * (MAXRPM - MINRPM) / 100.0);
	} else {
		motor[id].desiredRPM = 0; // off
	}
}
void calcRPMpercent( motorID_t id) {
	motor[id].actualRPMpercent = ((float) motor[id].actualRPM /MAXRPM) * 100.0;
}

int getRPMpercent (motorID_t id) {
	return (int) motor[id].actualRPMpercent;
}

void motorControlTask(void *pvParameters) {
	int x = (int)pvParameters;
	motorID_t id = (motorID_t)x;
	float control;
	float setpointPWM;
	int minPWM;
	int maxPWM;
	int lastRPM = MAXRPM;
	int RPMSetpoint;
	int lastRPMSetpoint[] = {0,0};
	TickType_t xLastWakeTime;
	

	if (!PWMisInitialized) {
		PWMisInitialized = true;
		PWMinit();
		xTaskCreate(measureRPMtask, "measureRPM", 8000, NULL, configMAX_PRIORITIES - 1, NULL);
	}

	// setPWMpercent(PWMchannelList[id], 15 + (id * 10));

	//		gpio_set_level(GPIO_NUM_15, 1); // turn power to motors on if needed
	// while(1) {
	// 	  	vTaskDelay(10);
	// 		control = advSettings.motorSettings[id].minPWM;
	// 		setPWMpercent(PWMchannelList[id], control);
	// 		motor[id].actualRPM = getRPM(id);
	// 		printf(">RPM%d:%d,AVG%d:%d\r\n", (int)id, getRPM(id), (int)id, getAVGRPM(id));
	// }

	do {
		maxPWM = advSettings.motorSettings[id].maxPWM;
		minPWM = advSettings.motorSettings[id].minPWM;
		motor[id].pid.setImaxImin(advSettings.motorPIDmaxI, -advSettings.motorPIDmaxI);
		motor[id].pid.setPIDValues(advSettings.motorPIDp, advSettings.motorPIDi, 0);
		RPMSetpoint = motor[id].desiredRPM;
		lastRPMSetpoint[id] = RPMSetpoint;
		motor[id].pid.setDesiredValue(RPMSetpoint);

		if (RPMSetpoint == 0) {
			setPWMpercent(PWMchannelList[id], 0);
			motor[id].actualRPM = getRPM(id);
			vTaskDelay(10 / portTICK_PERIOD_MS);
		} else {
			setpointPWM = ((float(RPMSetpoint) / MAXRPM) * (maxPWM - minPWM)) + minPWM;
			setPWMpercent(PWMchannelList[id], setpointPWM);
			motor[id].actualPWM = setpointPWM;
			printf("setpointPWM %d: %f\r\n", (int)id, setpointPWM);

			for (int n = 0; n < 20; n++) {
				lastRPM = getRPM(id);
				if (lastRPM == 0)
					motor[id].status = MOTOR_FAIL;
				else
					motor[id].status = MOTOR_OK;

				motor[id].actualRPM = lastRPM; // for CGI
				vTaskDelay(1000 / portTICK_PERIOD_MS);
				if (lastRPM > 100) {
					if (abs(getRPM(id) - lastRPM) < 10)
						break;
					if (getRPM(id) > RPMSetpoint) { // coarse
						setpointPWM--;
						setPWMpercent(PWMchannelList[id], setpointPWM);
					}
				}
				printf("stabiliseren %d: %d %d\r\n", (int)id, n + 1, getRPM(id));
			}

			for (int n = 0; n < 10; n++) {
				vTaskDelay(2500 / portTICK_PERIOD_MS);
				if ((setpointPWM > minPWM) && (getRPM(id) > RPMSetpoint)) // coarse
					setpointPWM--;
				else
					setpointPWM++;
				setPWMpercent(PWMchannelList[id], setpointPWM);
				motor[id].actualPWM = setpointPWM;
				printf("coarse %d: %d %d %2.1f \r\n", (int)id, n + 1, getRPM(id), setpointPWM);
				motor[id].actualRPM = getRPM(id); // for CGI
				calcRPMpercent(id);
			}
			xLastWakeTime = xTaskGetTickCount();

			if (motor[id].actualRPM == 0) {
				ESP_LOGE(TAG, "Geen toerental");
				motor[id].status = MOTOR_FAIL;
			} else
				motor[id].status = MOTOR_OK;

			// control loop running
			while (motor[id].desiredRPM != 0) {
				maxPWM = advSettings.motorSettings[id].maxPWM;
				minPWM = advSettings.motorSettings[id].minPWM;
				motor[id].pid.setImaxImin(advSettings.motorPIDmaxI, -advSettings.motorPIDmaxI); // in case these were changed
				motor[id].pid.setPIDValues(advSettings.motorPIDp, advSettings.motorPIDi, 0);
				RPMSetpoint = motor[id].desiredRPM;
				motor[id].pid.setDesiredValue(RPMSetpoint);

				xTaskDelayUntil(&xLastWakeTime, PID_INTERVAL / portTICK_PERIOD_MS);

				control = motor[id].pid.update(getRPM(id)) + setpointPWM;
				if (control < minPWM)
					control = minPWM;

				motor[id].actualPWM = control;
				if (id == AFAN)
					printf(">AV,PID%d:%1.1f,RPM%d:%d, Setp:%d \r\n", (int)id, control, (int)id, getRPM(id) , RPMSetpoint);

				//	printf("AV%d, %2.2f PID: %1.1f RPM:%d \n", (int)id, control, control - setpointPWM, getRPM(id));
				setPWMpercent(PWMchannelList[id], control);
				motor[id].actualRPM = getRPM(id);
				calcRPMpercent(id);
				if (motor[id].actualRPM == 0)
					motor[id].status = MOTOR_FAIL;
				else
					motor[id].status = MOTOR_OK;

				if ( abs( lastRPMSetpoint[id] - RPMSetpoint) > RPMBIGSTEP )
					break; // force leaving low speed loop	
			}
		}
	} while (1);
}

// CGI

const CGIdesc_t motorInfoDescriptorTable[] = {
	{"Afvoermotor toerental(%)", &motor[AFAN].actualRPMpercent, INT, 1},
	{"Afvoermotor PWM", &motor[AFAN].actualPWM, FLT, 1},
	{"Afvoermotor minPWM (%)", &advSettings.motorSettings[AFAN].minPWM, INT, 1},
	{"Afvoermotor maxPWM (%)", &advSettings.motorSettings[AFAN].maxPWM, INT, 1},
	{"Toevoermotor toerental(%)", &motor[TFAN].actualRPMpercent, INT, 1},
	{"Toevoermotor PWM", &motor[TFAN].actualPWM, FLT, 1},
	{"Toevoermotor minPWM (%)", &advSettings.motorSettings[TFAN].minPWM, INT, 1},
	{"Toevoermotor maxPWM (%)", &advSettings.motorSettings[TFAN].maxPWM, INT, 1},
	{NULL, NULL, FLT, 1},
};

motorStatus_t getMotorStatus(motorID_t id) { return motor[id].status; }
