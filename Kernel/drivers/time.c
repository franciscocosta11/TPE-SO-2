// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#include <time.h>
#include <interrupts.h>

#include <fonts.h>
#include <cursor.h>
#include <scheduler.h>

static unsigned long ticks = 0;

void timer_handler() {
	ticks++;
	toggleCursor();
	schedulerOnTick();
}

int ticks_elapsed() {
	return ticks;
}

int seconds_elapsed() {
	return ticks / SECONDS_TO_TICKS;
}

void sleepTicks(uint64_t sleep_t) {
	if (sleep_t == 0) {
		return;
	}

	unsigned long start = ticks;
	unsigned long target = start + sleep_t;

	while (ticks < target) {
		_hlt();
	}
}

void sleep(int seconds) {
	sleepTicks(seconds * SECONDS_TO_TICKS);
	return;
}
