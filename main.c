#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include "clock.h"
#include "usart.h"
#include <stdio.h>
#include "kbd.h"
#include "usbhid.h"

static void init_devices(void)
{
	clock_init();
	usart_setup();
}


int main(void)
{
	init_devices();
	kbd_init();
//	kbd_test2();
	usb_hid_init();
	while(1);
	return 0;
}
