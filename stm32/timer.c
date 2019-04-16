#include "stm32f4xx.h"

#define SYSTEMTICK_PERIOD_MS  10

__IO uint32_t	LocalTime = 0;	/* this variable is used to create a time reference incremented by 10ms */
uint32_t	timingdelay;

void Delay(uint32_t nCount) {

	timingdelay = LocalTime + nCount;
	while (timingdelay > LocalTime);

}

void timer_handle_it(void) {
	LocalTime += SYSTEMTICK_PERIOD_MS;
}

uint32_t get_local_time(void) {
	return (LocalTime);
}
