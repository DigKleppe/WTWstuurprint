
#ifndef LEDTASK_H

#define LEDTASK_H

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

extern LEDcolor_t onBoardColor, D1color, D2color; // leds on ledprint
extern bool onBoardFlash, D1Flash, D2Flash;
extern int D2nrFlashes;

void startLEDs();
#endif
