

/*******************
 *  measure RPM
 * inputs: tacho signals
 * period of signals is measeured
 *
 * outputs:
 *  actual rpm
 *
 */

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

#define TP_PIN GPIO_NUM_33 // pin 3 J2

static uint32_t Acntr; // for timeouts
static uint32_t Tcntr; // for timeouts
static uint32_t AtmrVal;
static uint32_t TtmrVal;
static uint32_t AtimeoutTmr;
static uint32_t TtimeoutTmr;

static uint32_t TIMERFREQ = 10 * 1000 * 1000;
static int PRMTIMEOUT = 20; // * 10ms

static int DEBOUNCES = 200;

static gptimer_handle_t gptimer;


static uint32_t RPMtoevoer, RPMAfvoer;

static void IRAM_ATTR gpio_isr_handler(void *arg) {
	uint64_t newTmrVal;
	static uint64_t oldTmrVal;

	uint32_t level = 0;
	//	gpio_set_level(TP_PIN, 1);
	for (int n = 0; n < DEBOUNCES; n++) {
		if (gpio_get_level(TTACHO_PIN) == 1)
			level++;
	}
	if (level < (DEBOUNCES / 4)) // skip erronouns IRQ on positive egde
	{
		gptimer_get_raw_count(gptimer, &newTmrVal);
		if (newTmrVal > oldTmrVal) { // skip if timer overflowed
			TtmrVal = newTmrVal - oldTmrVal;
			Tcntr++;
		}
		oldTmrVal = newTmrVal;
		//		gpio_set_level(TP_PIN, 0);
	}
}

static void IRAM_ATTR Agpio_isr_handler(void *arg) {
	uint64_t newTmrVal;
	static uint64_t oldTmrVal;
	uint32_t level = 0;
	for (int n = 0; n < DEBOUNCES; n++) {
		if (gpio_get_level(ATACHO_PIN) == 1)
			level++;
	}
	if (level < (DEBOUNCES / 4)) // skip erronouns IRQ on positive egde
	{
		gptimer_get_raw_count(gptimer, &newTmrVal);
		if (newTmrVal > oldTmrVal) { // skip if timer overflowed
			AtmrVal = newTmrVal - oldTmrVal;
			Acntr++;
		}
		oldTmrVal = newTmrVal;
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
		return (60 * TIMERFREQ) / timerVal;
	else
		return -1;
}

void measureRPMtask(void *pvParameters) {
	// int gpioNum;
	uint32_t oldAcntr = 0;
	uint32_t oldTcntr = 0;

	printf("measureRPMtask started\r\n");

//	gpio_set_direction(TP_PIN, GPIO_MODE_OUTPUT); // testpoint

	timerInit();
	gpio_set_direction(ATACHO_PIN, GPIO_MODE_INPUT);
	gpio_set_direction(TTACHO_PIN, GPIO_MODE_INPUT);

	gpio_install_isr_service(1 << 3);
	gpio_isr_handler_add(TTACHO_PIN, gpio_isr_handler, NULL);
	gpio_isr_handler_add(ATACHO_PIN, Agpio_isr_handler, NULL);
	gpio_set_intr_type(TTACHO_PIN, GPIO_INTR_NEGEDGE);
	gpio_set_intr_type(ATACHO_PIN, GPIO_INTR_NEGEDGE);

	while (1) {
		if (oldAcntr != Acntr) {
			if (AtimeoutTmr != 0) { // skip first value after timeout
				RPMAfvoer = toRPM(AtmrVal);
			}

			AtimeoutTmr = PRMTIMEOUT;
			oldAcntr = Acntr;
			//	printf(">RPMA:%d\r\n",(int) AVaverager.average());
			//	printf("AV Counter: %5u  RPM: %d\r\n", (int)AtmrVal, (int)temp);

		} else {
			if (AtimeoutTmr > 0) {
				AtimeoutTmr--;
			}
		}
		if (oldTcntr != Tcntr) {
			if (TtimeoutTmr != 0) { // skip first value after timeout
				RPMtoevoer = toRPM(TtmrVal);
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
	if (id == TFAN) {
		if (TtimeoutTmr > 0)
			return (int)RPMtoevoer;
		else
			return 0;
	} else {
		if (AtimeoutTmr > 0)
			return (int)RPMAfvoer;
		else
			return 0;
	}
}

