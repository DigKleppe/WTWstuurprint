#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdbool.h>

#include "esp_log.h"

#include "ledTask.h"
#include "wifiConnect.h"


static const char *TAG = "ledTask";

#define LED_TYPE LED_STRIP_WS2812
//#define LED_GPIO1 GPIO_NUM_15 // on board
#define LED_GPIO1	GPIO_NUM_47 // on s3 board
#define LED_GPIO2 	GPIO_NUM_11 // on keyledprint

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


LEDcolor_t D1color, D2color;  // leds on ledprint

// S3 wemos mini on board LED 
// https://www.wemos.cc/en/latest/_static/files/sch_s3_mini_v1.0.0.pdf


void LED1task(void *pvParameters) {
	bool flash = false;

	led_strip_t strip = {
		.type = LED_TYPE,
		.is_rgbw = true,  
#ifdef LED_STRIP_BRIGHTNESS
		.brightness = 55,
#endif
		.length = 1,
		.gpio = LED_GPIO1,
		.channel = RMT_CHANNEL_0,
		.buf = NULL,
	};
	ESP_ERROR_CHECK(led_strip_init(&strip));

	int c = 0;
	while (1) {
		switch ((int)connectStatus) {
		case CONNECTING:
			ESP_LOGI(TAG, "CONNECTING");
			c = COLOR_RED;
			break;

		case WPS_ACTIVE:
			ESP_LOGI(TAG, "WPS_ACTIVE");
			flash = true;
			c = COLOR_BLUE; 
			break;

		case IP_RECEIVED:
			//     ESP_LOGI(TAG, "IP_RECEIVED");
			flash = false;
			c = COLOR_GREEN; // groen
			break;

		case CONNECTED:
			ESP_LOGI(TAG, "CONNECTED");
			c = COLOR_YELLOW; // geel
			break;

		default:
			ESP_LOGI(TAG, "default");
			break;
		}

		led_strip_fill(&strip, 0, strip.length, colorsOnboardLed[c]);
		led_strip_flush(&strip);

		if (flash) {
			vTaskDelay(pdMS_TO_TICKS(300));
			led_strip_fill(&strip, 0, strip.length, colorsOnboardLed[COLOR_OFF]);
			led_strip_flush(&strip);
			vTaskDelay(pdMS_TO_TICKS(300));
		} else
			vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

// external RGB LEDs 
void LED2task(void *pvParameters) {

	led_strip_t strip = {
		.type = LED_TYPE,
		.is_rgbw = false,  
#ifdef LED_STRIP_BRIGHTNESS
		.brightness = 55,
#endif
		.length = 2,
		.gpio = LED_GPIO2,
		.channel = RMT_CHANNEL_1,
		.buf = NULL,
	};
	ESP_ERROR_CHECK(led_strip_init(&strip));

    D1color = COLOR_GREEN;
    D2color = COLOR_RED;

	while(1) {
		led_strip_set_pixel(&strip, 0, colors[D1color]);
		led_strip_set_pixel(&strip, 1, colors[D2color]);

		led_strip_flush(&strip);

		// if (flash) {
		// 	vTaskDelay(pdMS_TO_TICKS(300));
		// 	led_strip_fill(&strip, 0, strip.length, colors[7]);
		// 	led_strip_flush(&strip);
		// 	vTaskDelay(pdMS_TO_TICKS(300));
		// } else
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

void startLEDs() {
    led_strip_install();
	xTaskCreate(LED1task, "LED1task", 3000, NULL, 5, NULL);
    xTaskCreate(LED2task, "LED2task", 3000, NULL, 5, NULL);
}