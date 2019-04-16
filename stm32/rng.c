#include "stm32f4xx.h"
#include "stm32f4xx_rng.h"

#include "libc.h"
#include "stm32.h"

void rng_init(void) {
	RCC_AHB2PeriphClockCmd(RCC_AHB2Periph_RNG, ENABLE);
	RNG_Cmd(ENABLE);
}

uint32_t rng_rand(void) {
	uint32_t	number;

	while(RNG_GetFlagStatus(RNG_FLAG_DRDY)== RESET);

	number = RNG_GetRandomNumber();

	return (number);
}
