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

#include "esp_adc/adc_continuous.h"
#include "hal/adc_types.h"
#include <esp_adc/adc_filter.h>

#include "averager.h"
#include "settings.h"
#include "temperatureSensorTask.h"

const static char *TAG = "tSens";

/*---------------------------------------------------------------
		ADC General Macros
---------------------------------------------------------------*/
// ADC1 Channels

// ADC_CHANNEL_0  	 ts binnen
// ADC_CHANNEL_1 	 ts buiten
//  ADC_CHANNEL_2	 ref 1/3 VCC

#define NR_ADC_CHANNELS 3
#define ADC_ATTEN ADC_ATTEN_DB_6
#define AVERAGES 2 // 16

#define RREF 1200.0 // pullup sensors
#define R25 550.0
#define TC -1.96

int binnenTemperatuur, buitenTemperatuur;

typedef enum { TSIN, TSOUT, TSREF } sensorID_t;

static adc_channel_t adcChannel[]{ADC_CHANNEL_0, ADC_CHANNEL_1, ADC_CHANNEL_2};

static adc_cali_handle_t adcCaliHandle[NR_ADC_CHANNELS];
static int voltage[NR_ADC_CHANNELS][10];
static bool adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);

Averager averager[NR_ADC_CHANNELS];

#define EXAMPLE_ADC_UNIT ADC_UNIT_1
#define EXAMPLE_ADC_CONV_MODE ADC_CONV_SINGLE_UNIT_1
#define EXAMPLE_ADC_BIT_WIDTH SOC_ADC_DIGI_MAX_BITWIDTH

#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
#define EXAMPLE_ADC_OUTPUT_TYPE ADC_DIGI_OUTPUT_FORMAT_TYPE1
#define EXAMPLE_ADC_GET_CHANNEL(p_data) ((p_data)->type1.channel)
#define EXAMPLE_ADC_GET_DATA(p_data) ((p_data)->type1.data)
#else
#define EXAMPLE_ADC_OUTPUT_TYPE ADC_DIGI_OUTPUT_FORMAT_TYPE2
#define EXAMPLE_ADC_GET_CHANNEL(p_data) ((p_data)->type2.channel)
#define EXAMPLE_ADC_GET_DATA(p_data) ((p_data)->type2.data)
#endif

#define EXAMPLE_READ_LEN 256

static TaskHandle_t s_task_handle;
adc_iir_filter_handle_t filter_handle1 = NULL;
adc_iir_filter_handle_t filter_handle2 = NULL;

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

static bool IRAM_ATTR s_conv_done_cb(adc_continuous_handle_t handle, const adc_continuous_evt_data_t *edata, void *user_data) {
	BaseType_t mustYield = pdFALSE;
	// Notify that ADC continuous driver has done enough number of conversions
	vTaskNotifyGiveFromISR(s_task_handle, &mustYield);

	return (mustYield == pdTRUE);
}

static void continuous_adc_init(adc_channel_t *channel, uint8_t channel_num, adc_continuous_handle_t *out_handle) {
	adc_continuous_handle_t handle = NULL;

	adc_continuous_handle_cfg_t adc_config = {
		.max_store_buf_size = 1024,
		.conv_frame_size = EXAMPLE_READ_LEN,
	};
	ESP_ERROR_CHECK(adc_continuous_new_handle(&adc_config, &handle));

	adc_continuous_config_t dig_cfg = {
		.sample_freq_hz = 20 * 1000,
		.conv_mode = EXAMPLE_ADC_CONV_MODE,
		.format = EXAMPLE_ADC_OUTPUT_TYPE,
	};

	adc_digi_pattern_config_t adc_pattern[SOC_ADC_PATT_LEN_MAX] = {0};
	dig_cfg.pattern_num = channel_num;
	for (int i = 0; i < channel_num; i++) {
		adc_pattern[i].atten = ADC_ATTEN;
		adc_pattern[i].channel = channel[i] & 0x7;
		adc_pattern[i].unit = EXAMPLE_ADC_UNIT;
		adc_pattern[i].bit_width = EXAMPLE_ADC_BIT_WIDTH;

		ESP_LOGI(TAG, "adc_pattern[%d].atten is :%" PRIx8, i, adc_pattern[i].atten);
		ESP_LOGI(TAG, "adc_pattern[%d].channel is :%" PRIx8, i, adc_pattern[i].channel);
		ESP_LOGI(TAG, "adc_pattern[%d].unit is :%" PRIx8, i, adc_pattern[i].unit);
	}
	dig_cfg.adc_pattern = adc_pattern;
	ESP_ERROR_CHECK(adc_continuous_config(handle, &dig_cfg));

	*out_handle = handle;
}

int findChannelIndex(int ch) {
	int idx;
	for (idx = 0; idx < sizeof(adcChannel) / sizeof(adc_channel_t); idx++) {
		if (adcChannel[idx] == ch)
			break;
	}
	return idx;
}

adc_continuous_iir_filter_config_t iir1 = {
	.unit = EXAMPLE_ADC_UNIT,
	.channel = adcChannel[0],
	.coeff = ADC_DIGI_IIR_FILTER_COEFF_64,
};

adc_continuous_iir_filter_config_t iir2 = {
	.unit = EXAMPLE_ADC_UNIT,
	.channel = adcChannel[1],
	.coeff = ADC_DIGI_IIR_FILTER_COEFF_64,
};

void temperatureSensorTask(void *pvParameter) {
	esp_err_t ret;
	uint32_t ret_num = 0;
	uint32_t chan_num = 0;
	uint32_t idx = 0;

	uint8_t result[EXAMPLE_READ_LEN] = {0};
	memset(result, 0xcc, EXAMPLE_READ_LEN);

	for (int n = 0; n < NR_ADC_CHANNELS; n++) {
		adc_calibration_init(ADC_UNIT_1, adcChannel[n], ADC_ATTEN, &adcCaliHandle[n]);
		averager[n].setAverages(EXAMPLE_READ_LEN);
	}

	s_task_handle = xTaskGetCurrentTaskHandle();

	adc_continuous_handle_t handle = NULL;
	continuous_adc_init(adcChannel, sizeof(adcChannel) / sizeof(adc_channel_t), &handle);
	adc_new_continuous_iir_filter(handle, &iir1, &filter_handle1);
	adc_new_continuous_iir_filter(handle, &iir2, &filter_handle2);

	adc_continuous_iir_filter_enable(filter_handle1);
	adc_continuous_iir_filter_enable(filter_handle2);

	adc_continuous_evt_cbs_t cbs = {
		.on_conv_done = s_conv_done_cb,
	};
	ESP_ERROR_CHECK(adc_continuous_register_event_callbacks(handle, &cbs, NULL));
	ESP_ERROR_CHECK(adc_continuous_start(handle));

	while (1) {

		/**
		 * This is to show you the way to use the ADC continuous mode driver event callback.
		 * This `ulTaskNotifyTake` will block when the data processing in the task is fast.
		 * However in this example, the data processing (print) is slow, so you barely block here.
		 *
		 * Without using this event callback (to notify this task), you can still just call
		 * `adc_continuous_read()` here in a loop, with/without a certain block timeout.
		 */
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		while (1) {
			ret = adc_continuous_read(handle, result, EXAMPLE_READ_LEN, &ret_num, 0);
			if (ret == ESP_OK) {
				//    ESP_LOGI("TASK", "ret is %x, ret_num is %"PRIu32" bytes", ret, ret_num);
				for (int i = 0; i < ret_num; i += SOC_ADC_DIGI_RESULT_BYTES) {
					adc_digi_output_data_t *p = (adc_digi_output_data_t *)&result[i];
					chan_num = EXAMPLE_ADC_GET_CHANNEL(p);
					uint32_t data = EXAMPLE_ADC_GET_DATA(p);
					/* Check the channel number validation, the data is invalid if the channel num exceed the maximum channel */
					if (chan_num < SOC_ADC_CHANNEL_NUM(EXAMPLE_ADC_UNIT)) {
						//    ESP_LOGI(TAG, "Unit: %s, Channel: %"PRIu32", Value: %"PRIx32, unit, chan_num, data);
						idx = findChannelIndex(chan_num);
						averager[idx].write(data);
					} else {
						//	ESP_LOGW(TAG, "Invalid data [" PRIu32 ", " PRIx32 "]", chan_num, data);
					}
				}
				for (int n = 0; n < NR_ADC_CHANNELS; n++) {
					float avg = averager[n].average();

					//	voltage[n][0] = 3300 * avg / 4096.0;
					adc_cali_raw_to_voltage(adcCaliHandle[n], (int)avg, &voltage[n][0]);
					//	printf("%d %5.2f %d \r\n", n, avg, voltage[n][0]);
				}
				float mVCC = voltage[TSREF][0] * 3.0; // reference = 1/3 VCC
				float Rin = voltage[TSIN][0] * RREF / (mVCC - voltage[TSIN][0]);
				if ((Rin > 600) || (Rin < 400))
					binnenTemperatuur = ERRORTEMP;
				else
					binnenTemperatuur = 25 + (R25 - Rin) / TC; //  + userSettings.binnenTemperatuurOffset;

				float Rout = voltage[TSOUT][0] * RREF / (mVCC - voltage[TSOUT][0]);
				if ((Rout > 600) || (Rout < 400))
					buitenTemperatuur = ERRORTEMP;
				else
					buitenTemperatuur = 25 + (R25 - Rout) / TC; //  + userSettings.buitenTemperatuurOffset;

			//	printf("tin: %1.2f tout: %2.2f\n\r", binnenTemperatuur, buitenTemperatuur);
				printf("tin: %d tout: %d\n\r", binnenTemperatuur, buitenTemperatuur);

				//				ESP_LOGI(TAG, "%5.2f\t%5.2f\t%5.2f", averager[0].average(), averager[1].average(), averager[2].average());

			} else if (ret == ESP_ERR_TIMEOUT) {
				// We try to read `EXAMPLE_READ_LEN` until API returns timeout, which means there's no available data
				//	break;
				ESP_LOGE(TAG, "Timeout");
			}
			vTaskDelay(pdMS_TO_TICKS(1000));
		}
	}
}


