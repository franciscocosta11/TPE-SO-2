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

	// Yield activo: ceder control periódicamente mientras esperamos
	while (ticks < target) {
		// Ceder control al scheduler para que otros procesos puedan ejecutarse
		_hlt();
	}
}

void sleep(int seconds) {
	sleepTicks(seconds * SECONDS_TO_TICKS);
	return;
}
