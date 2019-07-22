#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include "led.h"

void led_init(void)
{
	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO13);
	gpio_clear(GPIOC,GPIO13);
}

void led(char s)
{
	if(s==0) gpio_set(GPIOC,GPIO13);
	else gpio_clear(GPIOC,GPIO13);
}

void led_toggle(void)
{
	gpio_toggle(GPIOC,GPIO13);
}
