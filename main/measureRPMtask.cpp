

/*******************
 *  measure RPM
 * inputs: tacho signals
 * period of signals is measeured
 *
 * outputs:
 *  actual rpm
 *
 */

#include "averager.h"

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "motorControlTask.h"

// #include "esp_err.h"
// #include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TIMER_BASE_CLK                                                                                                                                                             \
	(APB_CLK_FREQ)									 /*!< Frequency of the clock on the input of the timer groups                                                                  \
													  */
#define TIMER_DIVIDER (32)							 //  Hardware timer clock divider
#define TIMER_SCALE (TIMER_BASE_CLK / TIMER_DIVIDER) // convert counter value to seconds

#define ATACHO_PIN GPIO_NUM_17
#define TTACHO_PIN GPIO_NUM_21

static uint32_t Acntr; // for timeouts
static uint32_t Tcntr; // for timeouts
static uint32_t AtmrVal;
static uint32_t TtmrVal;
static uint32_t AtimeoutTmr;
static uint32_t TtimeoutTmr;

static uint32_t TIMERFREQ = 10 * 1000 * 1000;
static int PRMTIMEOUT =  20; // * 10ms 

static gptimer_handle_t gptimer;
static Averager AVaverager(32);
static Averager TVaverager(32);

static Averager AVaverager2(256);
static Averager TVaverager2(256);

static void IRAM_ATTR gpio_isr_handler(void *arg) {
	uint64_t newTmrVal;
	static uint64_t ToldTmrVal;

	gptimer_get_raw_count(gptimer, &newTmrVal);
	if (newTmrVal > ToldTmrVal) { // skip if timer overflowed
		TtmrVal = newTmrVal - ToldTmrVal;
		ToldTmrVal = newTmrVal;
		Tcntr++;
	}
}

static void IRAM_ATTR Agpio_isr_handler(void *arg) {
	uint64_t newTmrVal;
	static uint64_t AoldTmrVal;

	gptimer_get_raw_count(gptimer, &newTmrVal);
	if (newTmrVal > AoldTmrVal) { // skip if timer overflowed
		AtmrVal = newTmrVal - AoldTmrVal;
		AoldTmrVal = newTmrVal;
		Acntr++;
	}
}

static void timerInit() {
	gptimer_config_t timer_config = {
		.clk_src = GPTIMER_CLK_SRC_DEFAULT,
		.direction = GPTIMER_COUNT_UP,
		.resolution_hz = TIMERFREQ,
		.intr_priority = 3,
		.flags = {0},
	};
	ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, &gptimer));
	ESP_ERROR_CHECK(gptimer_enable(gptimer));
	ESP_ERROR_CHECK(gptimer_start(gptimer));
}

int toRPM(uint32_t timerVal) {
	if (timerVal > 0)
		return (30 * TIMERFREQ) / timerVal;
	else
		return -1;
}
int AVskip; // test
int TVskip;
void measureRPMtask(void *pvParameters) {
	// int gpioNum;
	uint32_t oldAcntr = 0;
	uint32_t oldTcntr = 0;
	uint32_t Atmr = 0;
	uint32_t Ttmr = 0;
	uint32_t temp;

	printf("measureRPMtask started\r\n");

	timerInit();
	gpio_set_direction(ATACHO_PIN, GPIO_MODE_INPUT);
	gpio_set_direction(TTACHO_PIN, GPIO_MODE_INPUT);

	gpio_install_isr_service(1 << 3);
	gpio_isr_handler_add(TTACHO_PIN, gpio_isr_handler, NULL);
	gpio_isr_handler_add(ATACHO_PIN, Agpio_isr_handler, NULL);
	gpio_set_intr_type(TTACHO_PIN, GPIO_INTR_ANYEDGE); // both edgdes
	gpio_set_intr_type(ATACHO_PIN, GPIO_INTR_ANYEDGE);

	while (1) {
		if (oldAcntr != Acntr) {
			if (AtimeoutTmr != 0) { // skip first value after timeout
				temp = toRPM(AtmrVal);
				if ((temp > 0) && (temp < MAXRPM + 500)) { // skip spike
					AVaverager.write(temp);
				} else
					AVskip++;
			}
			AtimeoutTmr = PRMTIMEOUT;
			oldAcntr = Acntr;
		//	printf("AV Counter: %5u  RPM: %d\r\n", (int)TtmrVal, (int)temp);

		} else {
			if (AtimeoutTmr > 0) {
				AtimeoutTmr--;
			}
		}
		if (oldTcntr != Tcntr) {
			if (TtimeoutTmr != 0) { // skip first value after timeout
				temp = toRPM(TtmrVal);
				if ((temp > 0) && (temp < MAXRPM + 500)) { // skip spike
					TVaverager.write(temp);
				} else
					TVskip++;
			}
		//	printf("TV Counter: %5u  RPM: %d\r\n", (int)TtmrVal, (int)temp);
			TtimeoutTmr = PRMTIMEOUT;
			oldTcntr = Tcntr;

		} else {
			if (TtimeoutTmr > 0) {
				TtimeoutTmr--;
			}
		}
		vTaskDelay(10 / portTICK_PERIOD_MS);
	};
}

int getRPM(motorID_t id) {
	float avg;
	if (id == TFAN) {
		if (TtimeoutTmr > 0) {
			avg = TVaverager.average();
			TVaverager2.write( (int) avg);
			return (int)avg;
		}
		else
			return 0;
	} else {
		if (AtimeoutTmr > 0) {
			avg = AVaverager.average();
			AVaverager2.write( (int) avg);
			return (int)avg;
		}
		else
			return 0;
	}
}

int getAVGRPM(motorID_t id) {
	if (id == TFAN) {
		if (TtimeoutTmr > 0)
			return (int)TVaverager2.average();
		else
			return 0;
	} else {
		if (AtimeoutTmr > 0)
			return (int)AVaverager2.average();
		else
			return 0;
	}
}