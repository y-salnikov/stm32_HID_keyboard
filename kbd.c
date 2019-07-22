#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <stdio.h>
#include "kbd.h"
#include "led.h"
#include "timer.h"
#include "usb_hid_keys.h"
#include "keymatrix.h"

static const pin lines[26]={{GPIOC,GPIO13},	{GPIOC,GPIO14},	{GPIOC,GPIO15},	{GPIOB, GPIO9},
	                 {GPIOA,GPIO0},		{GPIOA,GPIO1},	{GPIOB, GPIO8},	{GPIOB, GPIO7},
	                 {GPIOB, GPIO6},	{GPIOA,GPIO2},	{GPIOB, GPIO5},	{GPIOA,GPIO3},
	                 {GPIOB, GPIO4},	{GPIOA,GPIO4},	{GPIOB, GPIO3},	{GPIOA,GPIO5},
	                 {GPIOA, GPIO15},	{GPIOA,GPIO6},	{GPIOB, GPIO14},{GPIOA,GPIO7},
	                 {GPIOB, GPIO11},	{GPIOB,GPIO0},	{GPIOA,GPIO8}, {GPIOB,GPIO1},
	                 {GPIOB, GPIO15},	{GPIOB, GPIO10}};

static const pin* rows=lines; // rows= lines [0-7]

static const pin* cols=lines+8; // cols=lines[8-25]

static char s[64];
static char pressed[N_ROWS*N_COLS];
static char changed=0;

static void kbd_scan_row(void); 


static void kbd_test_init(void)
{
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON,0);
}

void kbd_init(void)
{
	uint16_t i;
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);
	gpio_primary_remap(AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON,0);
	led_init();
	tim_setup(kbd_scan_row);
	for(i=0;i<N_ROWS;i++)
	{
		gpio_set_mode(rows[i].port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_OPENDRAIN, rows[i].pin_number);
		gpio_set(rows[i].port,rows[i].pin_number);
	}
	for(i=0;i<N_COLS;i++)
	{
		gpio_set_mode(cols[i].port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, cols[i].pin_number);
		gpio_set(cols[i].port,cols[i].pin_number);
	}
	for(i=0;i<(N_ROWS*N_COLS);i++) pressed[i]=0;
	changed=0;
}


static void kbd_test_activate_line(uint16_t n)
{
	#define DELAY 100
	uint16_t i;
	for(i=0;i<N_LINES;i++)
	{
		if(i==n)
		{
			gpio_set_mode(lines[i].port, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, lines[i].pin_number);
			gpio_clear(lines[i].port,lines[i].pin_number);
		}
		else
		{
			gpio_set_mode(lines[i].port, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, lines[i].pin_number);
			gpio_set(lines[i].port,lines[i].pin_number);
		}
	}
	for(i=0;i<DELAY;i++) __asm__("nop");
}

static void kbd_activate_row(uint16_t n)
{
	#define DELAY 100
	uint16_t i;
	for(i=0;i<N_ROWS;i++)
	{
		if(i==n)
		{
			gpio_clear(rows[i].port,rows[i].pin_number);
		}
		else
		{
			gpio_set(rows[i].port,rows[i].pin_number);
		}
	}
	for(i=0;i<DELAY;i++) __asm__("nop");
}

void kbd_test(void)
{
	uint16_t l,p;
	kbd_test_init();
	printf("\033[2J");
	while(1)
	{
		printf("\033[1;1H\033[?25l");
		printf("  |\033[100m000000000\033[44m1111111111\033[45m2222222\033[0m\r\n");
		printf("N |\033[100m123456789\033[44m0123456789\033[45m0123456\033[0m\r\n");
		printf("--+--------------------------\r\n");
		for(l=0;l<N_LINES;l++)
		{
			kbd_test_activate_line(l);
			for(p=0;p<N_LINES;p++)
			{
				s[p]=gpio_get(lines[p].port,lines[p].pin_number) ? '.' : 'O';
			}
			s[N_LINES]=0;
			printf("%2d|%s\r\n",l+1,s);
		}
	}
}

static void kbd_scan_row(void)
{
	static char row=0;
	uint16_t col;
	char v;
	kbd_activate_row(row);
	for(col=0;col<N_COLS;col++)
	{
		v=gpio_get(cols[col].port,cols[col].pin_number) ? 0:1;
		if(pressed[(row*N_COLS)+col]!=v) changed=1;
		pressed[(row*N_COLS)+col]=v;
	}
	row++;
	if(row>=N_ROWS) row=0;
}

char* get_pressed(void)
{
	return pressed;
}

void kbd_test2(void)
{
	uint16_t l,p;
	printf("\033[2J\033[?25l\n");
	while(1)
	{
		printf("\033[1;1H");
		printf("  |\033[100m000000000\033[44m111111111\033[0m\r\n");
		printf("N |\033[100m123456789\033[44m012345678\033[0m\r\n");
		printf("--+------------------\r\n");
		for(l=0;l<N_ROWS;l++)
		{
			for(p=0;p<N_COLS;p++)
			{
				s[p]=pressed[(l*N_COLS)+p]? 'O':'.';
			}
			s[N_LINES]=0;
			printf("%2d|%s\r\n",l+1,s);
		}
	}
}

int kbd_get_report(char *report)
{
	uint16_t row,col,n;
	char key,flags;
	if(changed)
	{
		for(n=0;n<8;n++) report[n]=0;
		n=0;
		flags=0;
		report[0]=1;
		for(row=0;row<N_ROWS;row++)
		{
			for(col=0;col<N_COLS;col++)
			{
				if(pressed[(row*N_COLS)+col])
				{
					key=keymatrix[(row*N_COLS)+col];
					switch(key)
					{
						case KEY_LEFTCTRL:
							flags|=KEY_MOD_LCTRL ;
							break;
						case KEY_LEFTSHIFT:
							flags|=KEY_MOD_LSHIFT;
							break;
						case KEY_LEFTALT:
							flags|=KEY_MOD_LALT  ;
							break;
						case KEY_LEFTMETA:
							flags|=KEY_MOD_LMETA ;
							break;
						case KEY_RIGHTCTRL:
							flags|=KEY_MOD_RCTRL ;
							break;
						case KEY_RIGHTSHIFT:
							flags|=KEY_MOD_RSHIFT;
							break;
						case KEY_RIGHTALT:
							flags|=KEY_MOD_RALT  ;
							break;
						case KEY_RIGHTMETA:
							flags|=KEY_MOD_RMETA ;
							break;
						default:
							if(key!=KEY_NONE)
							{
								if(n<7)
								{
									report[n+2]=key;
									n++;
								} else
								{
									for(n=2;n<8;n++) report[n]=KEY_ERR_OVF;
								}
							}
							break;
					}
				}
			}
		}
		report[1]=flags;
		changed=0;
		return 1;
	}
	else return 0;
}
