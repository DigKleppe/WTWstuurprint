/*
 * tSensorTask.cpp
 *
 *  Created on: july 26, 2025
 *      Author: dig
 */

#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "averager.h"

const static char *TAG = "tSens";

/*---------------------------------------------------------------
		ADC General Macros
---------------------------------------------------------------*/
// ADC1 Channels

// ADC_CHANNEL_0  	// ts binnen
// ADC_CHANNEL_1 	// ref 1/3 VCC
//  ADC_CHANNEL_2	// ts buiten

#define NR_ADC_CHANNELS 3
#define ADC_ATTEN ADC_ATTEN_DB_6
#define AVERAGES 16

#define RREF 	1200.0 // pullup sensors
#define R25		550.0
#define TC 		-1.96

float binnenTemperatuur, buitenTemperatuur;

typedef enum { TSIN, TSOUT, TSREF } sensorID_t;

static const adc_channel_t adcChannel[]{ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2};

static adc_cali_handle_t adcCaliHandle[NR_ADC_CHANNELS];
static int adc_raw[NR_ADC_CHANNELS][10];
static int voltage[NR_ADC_CHANNELS][10];
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);

Averager averager[NR_ADC_CHANNELS];

/*---------------------------------------------------------------
		ADC Calibration
---------------------------------------------------------------*/
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle) {
	adc_cali_handle_t handle = NULL;
	esp_err_t ret = ESP_FAIL;
	bool calibrated = false;

	if (!calibrated) {
		ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
		adc_cali_curve_fitting_config_t cali_config = {
			.unit_id = unit,
			.chan = channel,
			.atten = atten,
			.bitwidth = ADC_BITWIDTH_DEFAULT,
		};
		ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
		if (ret == ESP_OK) {
			calibrated = true;
		}
	}

	*out_handle = handle;
	if (ret == ESP_OK) {
		ESP_LOGI(TAG, "Calibration Success");
	} else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
		ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
	} else {
		ESP_LOGE(TAG, "Invalid arg or no memory");
	}
	return calibrated;
}

void temperatureSensorTask(void *pvParameter) {

	//-------------ADC1 Init---------------//
	adc_oneshot_unit_handle_t adc1_handle;
	adc_oneshot_unit_init_cfg_t init_config1 = {
		.unit_id = ADC_UNIT_1,
	};
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

	//-------------ADC1 Config---------------//
	adc_oneshot_chan_cfg_t config = {
		.atten = ADC_ATTEN,
		.bitwidth = ADC_BITWIDTH_DEFAULT,
	};
	for (int n = 0; n < NR_ADC_CHANNELS; n++) {
		ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, adcChannel[n], &config));
		adc_calibration_init(ADC_UNIT_1, adcChannel[n], ADC_ATTEN, &adcCaliHandle[n]);
		averager[n].setAverages(AVERAGES);
	}

	while (1) {
		for (int n = 0; n < NR_ADC_CHANNELS; n++) {
			ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, adcChannel[n], &adc_raw[n][0]));
			averager[n].write(adc_raw[n][0]);
			int avg = (int)averager[n].average();
			adc_cali_raw_to_voltage(adcCaliHandle[n], avg, &voltage[n][0]);
		}
		float mVCC = voltage[TSREF][0] * 3.0; // reference = 1/3 VCC
		float Rin = voltage[TSIN][0] * RREF / (mVCC - voltage[TSIN][0]);
		if (( Rin > 600) || ( Rin < 400))
			binnenTemperatuur = 9999;
		else
			binnenTemperatuur = 25+ (R25-Rin)/TC;

		float Rout = voltage[TSOUT][0] * RREF / (mVCC - voltage[TSOUT][0]);

		if (( Rout > 600) || ( Rout < 400))
			buitenTemperatuur = 9999;
		else
			buitenTemperatuur = 25+ (R25-Rout)/TC;

	//	printf( "tin: %1.2f tout: %2.2f\n\r" ,binnenTemperatuur, buitenTemperatuur);
		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

