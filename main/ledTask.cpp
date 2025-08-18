#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdbool.h>

#include "esp_log.h"

#include "ledTask.h"
#include "wifiConnect.h"

static const char *TAG = "ledTask";

#define LED_TYPE LED_STRIP_WS2812
#define LED_GPIO1 GPIO_NUM_47 // on s3 board
#define LED_GPIO2 GPIO_NUM_11 // on keyledprint

#define BRIGHTNESS 30

static const rgb_t colors[] = {
	{.r = 0x0f, .g = 0x0f, .b = 0x0f}, // wit
	{.r = 0x00, .g = 0x00, .b = 0x2f}, // blauw
	{.r = 0x00, .g = 0x2f, .b = 0x00}, // groen
	{.r = 0x2f, .g = 0x00, .b = 0x00}, // rood
	{.r = 0x2f, .g = 0x2f, .b = 0x00}, // geel
	{.r = 0x2f, .g = 0x00, .b = 0x2f}, // paars
	{.r = 0x00, .g = 0x2f, .b = 0x2f}, // cyaan
	{.r = 0x00, .g = 0x00, .b = 0x00}, // uit
};

static const rgb_t colorsOnboardLed[] = {
	{.r = 0x0f, .g = 0x0f, .b = 0x0f}, // wit
	{.r = 0x00, .g = 0x00, .b = 0x2f}, // blauw
	{.r = 0x2f, .g = 0x00, .b = 0x00}, // rood
	{.r = 0x00, .g = 0x2f, .b = 0x00}, // groen
	{.r = 0x2f, .g = 0x2f, .b = 0x00}, // geel
	{.r = 0x2f, .g = 0x00, .b = 0x2f}, // paars
	{.r = 0x00, .g = 0x2f, .b = 0x2f}, // cyaan
	{.r = 0x00, .g = 0x00, .b = 0x00}, // uit
};

LEDcolor_t onBoardColor, D1color, D2color; // leds on ledprint
bool onBoardFlash, D1Flash, D2Flash;

// S3 wemos mini on board LED
// https://www.wemos.cc/en/latest/_static/files/sch_s3_mini_v1.0.0.pdf


void LEDtask(void *pvParameters) {
	int c = 0;

	led_strip_t strip = {
		.type = LED_TYPE,
		.is_rgbw = false,
#ifdef LED_STRIP_BRIGHTNESS
		.brightness = BRIGHTNESS,
#endif
		.length = 2,
		.gpio = LED_GPIO2,
		.channel = RMT_CHANNEL_1,
		.buf = NULL,
	};
	ESP_ERROR_CHECK(led_strip_init(&strip));


	led_strip_t onBoardStrip = {
		.type = LED_TYPE,
		.is_rgbw = true,
#ifdef LED_STRIP_BRIGHTNESS
		.brightness = BRIGHTNESS,
#endif
		.length = 1,
		.gpio = LED_GPIO1,
		.channel = RMT_CHANNEL_0,
		.buf = NULL,
	};
	ESP_ERROR_CHECK(led_strip_init(&onBoardStrip));

	D1color = COLOR_GREEN;
	D2color = COLOR_GREEN;
	onBoardColor = COLOR_GREEN;

	while (1) {
		led_strip_set_pixel(&onBoardStrip, 0, colorsOnboardLed[onBoardColor]);
		led_strip_set_pixel(&strip, 0, colors[D1color]);
		led_strip_set_pixel(&strip, 1, colors[D2color]);
		led_strip_flush(&onBoardStrip);
		led_strip_flush(&strip);
		vTaskDelay(pdMS_TO_TICKS(300));
		
		if (onBoardFlash) {
			led_strip_set_pixel(&onBoardStrip, 0, colorsOnboardLed[COLOR_OFF]);
		}
		if (D1Flash) {
			led_strip_set_pixel(&strip, 0, colors[COLOR_OFF]);
		}
		if (D2Flash) {
			led_strip_set_pixel(&strip, 1, colors[COLOR_OFF]);
		}
		led_strip_flush(&onBoardStrip);
		led_strip_flush(&strip);
		vTaskDelay(pdMS_TO_TICKS(300));
	}
}

void startLEDs() {
	led_strip_install();
	xTaskCreate(LEDtask, "LEDtask", 3000, NULL, tskIDLE_PRIORITY, NULL);
}