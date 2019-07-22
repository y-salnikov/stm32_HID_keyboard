typedef struct
{
	uint32_t port;
	uint16_t pin_number;
} pin;

#define N_LINES 26
#define N_ROWS 8
#define N_COLS 18

void kbd_test(void);
void kbd_init(void);
char* get_pressed(void);
void kbd_test2(void);
int kbd_get_report(char *report);
