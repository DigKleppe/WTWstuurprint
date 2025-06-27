
#include <led_strip.h>


#define COLORS_TOTAL (sizeof(colors) / sizeof(rgb_t))

typedef enum {
	COLOR_WHITE,
	COLOR_BLUE,
    COLOR_GREEN,
	COLOR_RED,
	COLOR_YELLOW,
	COLOR_PURPLE,
	COLOR_CYANE,
	COLOR_OFF
}LEDcolor_t;

void startLEDs();