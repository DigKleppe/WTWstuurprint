/* 2300 rpm = max 600 hz*/


#define LOG_LOCAL_LEVEL  ESP_LOG_INFO   //ESP_LOG_ERROR
#include "esp_log.h"

#include "motorControlTask.h"
#include "measureRPMtask.h"
#include "temperatureSensorTask.h"
#include "pid.h"
#include "settings.h"

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

#define PID_INTERVAL 1000 // ms

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

static bool forceNewCalibration; 

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
	static float oldrpm =0;
	if (rpm > MAXRPM)
		rpm = MAXRPM;
	if (rpm < MINRPM)
		rpm = MINRPM;
	motor[id].desiredRPM = rpm;
	if ( rpm != oldrpm) {
		ESP_LOGI(TAG,"RPM set to %f", rpm);
		oldrpm = rpm;
	}
}


void setRPMpercent(motorID_t id, int percent) {
	if ( percent > 100 )
		percent = 100;
		
	if (percent > 0) {
		setRPM(id, MINRPM + (float)percent * (MAXRPM - MINRPM) / 100.0);
	}
	else {
		motor[id].desiredRPM = 0; // off
	}
}

void motorControlTask(void *pvParameters) {
	int x = (int)pvParameters;
	motorID_t id = (motorID_t)x;

	float control;
	float setpointPWM;
	int minPWM = 13;
	bool found = false;
	int maxPWM = 65;
	int lastRPM = MAXRPM;
	int debounce = 0;
	int RPMSetpoint;
	int retries = 0;

	TickType_t xLastWakeTime;
	if (!PWMisInitialized) {
		PWMisInitialized = true;
		PWMinit();
		xTaskCreate(measureRPMtask, "measureRPM", 8000, NULL, 1, NULL);
	}
	motor[id].pid.setImaxImin(userSettings.motorPIDmaxI,-userSettings.motorPIDmaxI );
	motor[id].pid.setPIDValues(userSettings.motorPIDp , userSettings.motorPIDi, 0);

	// setPWMpercent(PWMchannelList[id], 15 + (id * 10));
	// while(1)
	//  	vTaskDelay(100);

	do {
		if (!userSettings.motorSettings[id].isCalibrated) {
			setPWMpercent(PWMchannelList[id], 100);
			retries = 0;
			// determine maximum PWM
			// motor has slowstart ramp up first
			do {
				vTaskDelay(2000 / portTICK_PERIOD_MS);
				printf("optrekken %d %d \r\n", (int)id, getRPM(id));
				motor[id].actualRPM =  getRPM(id);
				if (getRPM(id) < (MAXRPM - 700)) {
					lastRPM = getRPM(id);
				} else
					found = true;
				if (retries++ > 10)
					motor[id].status = MOTOR_FAIL;
			} while (!found);
			vTaskDelay(5000 / portTICK_PERIOD_MS);
			motor[id].status = MOTOR_OK;
			found = false;
			// then decrease PWM to find max effective PWM percentage
			lastRPM = getRPM(id);
			printf("Max AV%d  %d\n", (int)id, lastRPM);
			debounce = 3;
			do {
				setPWMpercent(PWMchannelList[id], maxPWM);
				vTaskDelay(2000 / portTICK_PERIOD_MS);
				if (getRPM(id) >= (lastRPM - 50)) {
					maxPWM--;
					printf("Max AV %d  %d \r\n", getRPM(id), maxPWM);
					debounce = 3;
				} else {
					if (debounce-- == 0)
						found = true;
				}
				motor[id].actualRPM =  getRPM(id);
			} while (!found);
			found = false;
			// find lowest effective PWM percentage

			setPWMpercent(PWMchannelList[id], minPWM);
			vTaskDelay(5000 / portTICK_PERIOD_MS);

			do {
				vTaskDelay(5000 / portTICK_PERIOD_MS);
				if (getRPM(id)) {
					minPWM--;
					printf("Min: %d %d %d\r\n", (int)id, minPWM, getRPM(id));
					setPWMpercent(PWMchannelList[id], minPWM);
				} else
					found = true;
				motor[id].actualRPM =  getRPM(id);
			} while (!found);

			// determine mimimum value to start motor
			found = false;
			do {
				if (!getRPM(id)) { //
					minPWM++;
					printf("Start Min %d %d\r\n", (int)id, minPWM);
					setPWMpercent(PWMchannelList[id], minPWM);
					motor[id].actualRPM =  getRPM(id);
					vTaskDelay(10000 / portTICK_PERIOD_MS);
				} else
					found = true;
			} while (!found);
			userSettings.motorSettings[id].isCalibrated = true;
			userSettings.motorSettings[id].maxPWM = maxPWM;
			userSettings.motorSettings[id].minPWM = minPWM;
			saveSettings();
		} // end if !calibrated

		else {
			maxPWM = userSettings.motorSettings[id].maxPWM;
			minPWM = userSettings.motorSettings[id].minPWM;
		}

	// control loop start 

		RPMSetpoint = motor[id].desiredRPM;
		motor[id].pid.setDesiredValue(RPMSetpoint);
		if (RPMSetpoint == 0) {
			setPWMpercent(PWMchannelList[id], 0);
			motor[id].actualRPM =  getRPM(id); 
			vTaskDelay(10 / portTICK_PERIOD_MS);
		} else {
			setpointPWM = ((float(RPMSetpoint) / MAXRPM) * (maxPWM - minPWM)) + minPWM;
			setPWMpercent(PWMchannelList[id], setpointPWM);
			printf("setpointPWM %d: %f\r\n", (int)id, setpointPWM);

			for (int n = 0; n < 20; n++) {
				lastRPM = getRPM(id);
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
				printf("coarse %d: %d %d %2.1f \r\n", (int)id, n + 1, getRPM(id), setpointPWM);
				motor[id].actualRPM = getRPM(id); // for CGI
			}
			xLastWakeTime = xTaskGetTickCount();
			if ( motor[id].actualRPM == 0) {
				ESP_LOGE(TAG, "Geen toerental");
				motor[id].status = MOTOR_FAIL;
			}
			else
				motor[id].status = MOTOR_OK;


		// control loop running 
			while ((motor[id].desiredRPM != 0) && ! forceNewCalibration) {
				forceNewCalibration = false;
				xTaskDelayUntil(&xLastWakeTime, PID_INTERVAL / portTICK_PERIOD_MS);
				
				motor[id].pid.setImaxImin(userSettings.motorPIDmaxI,-userSettings.motorPIDmaxI );  // in case these were changed
				motor[id].pid.setPIDValues(userSettings.motorPIDp , userSettings.motorPIDi, 0);
				RPMSetpoint = motor[id].desiredRPM;
				motor[id].pid.setDesiredValue(RPMSetpoint);

				control = motor[id].pid.update(getRPM(id)) + setpointPWM;
				if (control < minPWM)
					control = minPWM;
				printf("AV%d: %2.2f PID: %1.1f RPM:%d \n", (int)id, control, control - setpointPWM, getRPM(id));
				setPWMpercent(PWMchannelList[id], control);
				motor[id].actualRPM =  getRPM(id);
			}
		}
	} while (1);
}
// CGI 

const CGIdesc_t motorInfoDescriptorTable[] = {
	{"Afvoermotor toerental(RPM)", &motor[AFAN].actualRPM , INT, 1 },
	{"Afvoermotor minPWM (%)", &userSettings.motorSettings[AFAN].minPWM , INT, 1},
	{"Afvoermotor maxPWM (%)", &userSettings.motorSettings[AFAN].maxPWM, INT, 1 },
	{"Toevoermotor toerental(RPM)", &motor[TFAN].actualRPM, INT, 1},
	{"Toevoermotor minPWM (%)", &userSettings.motorSettings[TFAN].minPWM, INT, 1 },
	{"Toevoermotor maxPWM (%)", &userSettings.motorSettings[TFAN].maxPWM, INT, 1 },
	{NULL, NULL, FLT,1},
};

int resetFanLimitsScript(char *pBuffer, int count)  {
	int len = 0;
	switch (scriptState) {
	case 0:
		scriptState++;
		userSettings.motorSettings[TFAN].isCalibrated = false;
		userSettings.motorSettings[AFAN].isCalibrated = false;
		forceNewCalibration = true;

		len = sprintf(pBuffer, "OK\n");
		return len;
		break;
	default:
		break;
	}
	return 0;
}
motorStatus_t getMotorStatus ( motorID_t id) {
	return motor[id].status;
}



// extern "C" void app_main(void) {
// 	int state = 0;
// 	int delay = 0;
// 	int old = -1;
// 	esp_err_t err = nvs_flash_init();
// 	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
// 		ESP_ERROR_CHECK(nvs_flash_erase());
// 		err = nvs_flash_init();
// 		ESP_LOGI(TAG, "nvs flash erased");
// 	}
// 	ESP_ERROR_CHECK(err);

// 	err = init_spiffs();
// 	if (err != ESP_OK) {
// 		ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(err));
// 		return;
// 	}

// 	err = loadSettings();
// 	xTaskCreate(motorControlTask, "motorC1", 8000, (void *)AFAN, 1, NULL);
// 	xTaskCreate(motorControlTask, "motorC2", 8000, (void *)TFAN, 1, NULL);
// 	while (1) {
// 		vTaskDelay(10000);

// 		state++;
// 		switch (state) {
// 		case 3:
// 			motor[AFAN].desiredRPM = 1000;
// 			break;
// 		case 6:
// 			motor[AFAN].desiredRPM = 1200;
// 			break;

// 		case 10:
// 			state--;
// 			if (delay) {
// 				delay--;
// 			} else {
// 				delay = 10;
// 				motor[AFAN].desiredRPM += 100;
// 				if (motor[AFAN].desiredRPM > MAXRPM) {
// 					state++;
// 					motor[AFAN].desiredRPM = 2000;
// 					delay = 0;
// 				}
// 			}
// 			break;

// 		case 20:
// 			if (delay) {
// 				delay--;
// 			} else {
// 				delay = 10;
// 				motor[AFAN].desiredRPM -= 100;
// 				if (motor[AFAN].desiredRPM < MINRPM) {
// 					state = 0;
// 					motor[AFAN].desiredRPM = 0;
// 				}
// 			}
// 			break;
// 		default:
// 			break;
// 		}
// 		if (old != motor[AFAN].desiredRPM) {
// 			printf("  ***  %d *** ", motor[AFAN].desiredRPM);
// 			old = motor[AFAN].desiredRPM;
// 		}
// 	}
// }
