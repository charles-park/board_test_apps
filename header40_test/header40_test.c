//------------------------------------------------------------------------------
//
// 2022.09.28 ODROID-N2 Lite HEADER 40Pin Test. (chalres-park)
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <getopt.h>
#include <pthread.h>

#include "typedefs.h"
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
static char *tolowerstr (char *str)
{
	int i, len = strlen(str), space = 0;

	if (*str == 0x20)
		while ((*str++ != 0x20) && len--);

	for (i = 0; i < len; i++, str++)
		*str = tolower(*str);

	/* calculate input string(*str) start pointer */
	return	(str -len +space);
}

//------------------------------------------------------------------------------
static char *toupperstr (char *str)
{
	int i, len = strlen(str), space = 0;

	if (*str == 0x20)
		while ((*str++ != 0x20) && len--);

	for (i = 0; i < len; i++, str++)
		*str = toupper(*str);

	/* calculate input string(*str) start pointer */
	return	(str -len +space);
}

//------------------------------------------------------------------------------
// ODROID-N2/N2L Header to GPIO Number
// ref : https://docs.google.com/spreadsheets/d/18cRWfgj9xmlr1JQb91fNN7SQxrBZxkHoxOEJN6Yy4SI/edit#gid=1883790439
//------------------------------------------------------------------------------
const __u16	HeaderToGPIO[] = {
	  0,		// Not used
	  0,   0,	// |  1 : 3.3V      ||  2 : 5.0V      |
	493,   0,	// |  3 : I2C-2 SDA ||  4 : 5.0V      |
	494,   0,	// |  5 : I2C-2 SCL ||  6 : GND       |
	473, 488,	// |  7 : GPIOA.13  ||  8 : UART_TX_A |
	  0, 489,	// |  9 : GND       || 10 : UART_RX_A |
	479, 492,	// | 11 : GPIOX.3   || 12 : PWM_E     |
	480,   0,	// | 13 : GPIOX.4   || 14 : GND       |
	483, 476,	// | 15 : GPIOX.7   || 16 : GPIOX.0   |
	  0, 477,	// | 17 : 3.3V      || 18 : GPIOX.1   |
	484,   0,	// | 19 : SPI0_MOSI || 20 : GND       |
	485, 478,	// | 21 : SPI0_MISO || 22 : GPIOX.2   |
	487, 486,	// | 23 : SPI0_CLK  || 24 : SPI0_CS0  |
	  0, 464,	// | 25 : GND       || 26 : GPIOA.4   |
	474, 475,	// | 27 : I2C-3 SDA || 28 : I2C-3 SCL |
	490,   0,	// | 29 : GPIOX.14  || 30 : GND       |
	491, 472,	// | 31 : GPIOX.15  || 32 : GPIOA.12  |
	481,   0,	// | 33 : GPIOX.5   || 34 : GND       |
	482, 495,	// | 35 : GPIOX.6   || 36 : GPIOX.19  |
	  0,   0,	// | 37 : ADC.AIN3  || 38 : REF 1.8V  |
	  0,   0	// | 39 : GND       || 40 : ADC.AIN2  |
};

#define	PATTERNS_COUNT	4
#define	HEADER40_PINS	sizeof(HeaderToGPIO)
const __u16	Patterns[PATTERNS_COUNT][HEADER40_PINS] = {
	{
		// ALL High Pattern
		0,		// Not used
		0,   0,	// |  1 : 3.3V      ||  2 : 5.0V      |
		1,   0,	// |  3 : I2C-2 SDA ||  4 : 5.0V      |
		1,   0,	// |  5 : I2C-2 SCL ||  6 : GND       |
		1,   1,	// |  7 : GPIOA.13  ||  8 : UART_TX_A |
		0,   1,	// |  9 : GND       || 10 : UART_RX_A |
		1,   1,	// | 11 : GPIOX.3   || 12 : PWM_E     |
		1,   0,	// | 13 : GPIOX.4   || 14 : GND       |
		1,   1,	// | 15 : GPIOX.7   || 16 : GPIOX.0   |
		0,   1,	// | 17 : 3.3V      || 18 : GPIOX.1   |
		1,   0,	// | 19 : SPI0_MOSI || 20 : GND       |
		1,   1,	// | 21 : SPI0_MISO || 22 : GPIOX.2   |
		1,   1,	// | 23 : SPI0_CLK  || 24 : SPI0_CS0  |
		0,   1,	// | 25 : GND       || 26 : GPIOA.4   |
		1,   1,	// | 27 : I2C-3 SDA || 28 : I2C-3 SCL |
		1,   0,	// | 29 : GPIOX.14  || 30 : GND       |
		1,   1,	// | 31 : GPIOX.15  || 32 : GPIOA.12  |
		1,   0,	// | 33 : GPIOX.5   || 34 : GND       |
		1,   1,	// | 35 : GPIOX.6   || 36 : GPIOX.19  |
		0,   0,	// | 37 : ADC.AIN3  || 38 : REF 1.8V  |
		0,   0	// | 39 : GND       || 40 : ADC.AIN2  |
	},	{
		// ALL Clear Pattern
		0,		// Not used
		0,   0,	// |  1 : 3.3V      ||  2 : 5.0V      |
		0,   0,	// |  3 : I2C-2 SDA ||  4 : 5.0V      |
		0,   0,	// |  5 : I2C-2 SCL ||  6 : GND       |
		0,   0,	// |  7 : GPIOA.13  ||  8 : UART_TX_A |
		0,   0,	// |  9 : GND       || 10 : UART_RX_A |
		0,   0,	// | 11 : GPIOX.3   || 12 : PWM_E     |
		0,   0,	// | 13 : GPIOX.4   || 14 : GND       |
		0,   0,	// | 15 : GPIOX.7   || 16 : GPIOX.0   |
		0,   0,	// | 17 : 3.3V      || 18 : GPIOX.1   |
		0,   0,	// | 19 : SPI0_MOSI || 20 : GND       |
		0,   0,	// | 21 : SPI0_MISO || 22 : GPIOX.2   |
		0,   0,	// | 23 : SPI0_CLK  || 24 : SPI0_CS0  |
		0,   0,	// | 25 : GND       || 26 : GPIOA.4   |
		0,   0,	// | 27 : I2C-3 SDA || 28 : I2C-3 SCL |
		0,   0,	// | 29 : GPIOX.14  || 30 : GND       |
		0,   0,	// | 31 : GPIOX.15  || 32 : GPIOA.12  |
		0,   0,	// | 33 : GPIOX.5   || 34 : GND       |
		0,   0,	// | 35 : GPIOX.6   || 36 : GPIOX.19  |
		0,   0,	// | 37 : ADC.AIN3  || 38 : REF 1.8V  |
		0,   0	// | 39 : GND       || 40 : ADC.AIN2  |
	},	{
		// Cross Logic Pattern 1
		0,		// Not used
		0,   0,	// |  1 : 3.3V      ||  2 : 5.0V      |
		1,   0,	// |  3 : I2C-2 SDA ||  4 : 5.0V      |
		0,   0,	// |  5 : I2C-2 SCL ||  6 : GND       |
		1,   0,	// |  7 : GPIOA.13  ||  8 : UART_TX_A |
		0,   1,	// |  9 : GND       || 10 : UART_RX_A |
		1,   0,	// | 11 : GPIOX.3   || 12 : PWM_E     |
		0,   0,	// | 13 : GPIOX.4   || 14 : GND       |
		1,   0,	// | 15 : GPIOX.7   || 16 : GPIOX.0   |
		0,   1,	// | 17 : 3.3V      || 18 : GPIOX.1   |
		1,   0,	// | 19 : SPI0_MOSI || 20 : GND       |
		0,   1,	// | 21 : SPI0_MISO || 22 : GPIOX.2   |
		1,   0,	// | 23 : SPI0_CLK  || 24 : SPI0_CS0  |
		0,   1,	// | 25 : GND       || 26 : GPIOA.4   |
		1,   0,	// | 27 : I2C-3 SDA || 28 : I2C-3 SCL |
		0,   0,	// | 29 : GPIOX.14  || 30 : GND       |
		1,   0,	// | 31 : GPIOX.15  || 32 : GPIOA.12  |
		0,   0,	// | 33 : GPIOX.5   || 34 : GND       |
		1,   0,	// | 35 : GPIOX.6   || 36 : GPIOX.19  |
		0,   0,	// | 37 : ADC.AIN3  || 38 : REF 1.8V  |
		0,   0	// | 39 : GND       || 40 : ADC.AIN2  |
	},	{
		// Cross Logic Pattern 2
		0,		// Not used
		0,   0,	// |  1 : 3.3V      ||  2 : 5.0V      |
		0,   0,	// |  3 : I2C-2 SDA ||  4 : 5.0V      |
		1,   0,	// |  5 : I2C-2 SCL ||  6 : GND       |
		0,   1,	// |  7 : GPIOA.13  ||  8 : UART_TX_A |
		0,   0,	// |  9 : GND       || 10 : UART_RX_A |
		0,   1,	// | 11 : GPIOX.3   || 12 : PWM_E     |
		0,   0,	// | 13 : GPIOX.4   || 14 : GND       |
		0,   1,	// | 15 : GPIOX.7   || 16 : GPIOX.0   |
		0,   0,	// | 17 : 3.3V      || 18 : GPIOX.1   |
		0,   0,	// | 19 : SPI0_MOSI || 20 : GND       |
		1,   0,	// | 21 : SPI0_MISO || 22 : GPIOX.2   |
		0,   1,	// | 23 : SPI0_CLK  || 24 : SPI0_CS0  |
		0,   0,	// | 25 : GND       || 26 : GPIOA.4   |
		0,   1,	// | 27 : I2C-3 SDA || 28 : I2C-3 SCL |
		1,   0,	// | 29 : GPIOX.14  || 30 : GND       |
		0,   1,	// | 31 : GPIOX.15  || 32 : GPIOA.12  |
		1,   0,	// | 33 : GPIOX.5   || 34 : GND       |
		0,   1,	// | 35 : GPIOX.6   || 36 : GPIOX.19  |
		0,   0,	// | 37 : ADC.AIN3  || 38 : REF 1.8V  |
		0,   0	// | 39 : GND       || 40 : ADC.AIN2  |
	},
};

//------------------------------------------------------------------------------
struct header40_test_t {
	bool	is_file;
	bool	status;
};

static struct header40_test_t header40[HEADER40_PINS];

//------------------------------------------------------------------------------
#define	GPIO_CONTROL_PATH	"/sys/class/gpio"
#define	GPIO_DIR_OUT		1
#define	GPIO_DIR_IN			0

//------------------------------------------------------------------------------
static int gpio_export (int gpio)
{
	char fname[256];
	FILE *fp;

	memset (fname, 0x00, sizeof(fname));
	sprintf (fname, "%s/export", GPIO_CONTROL_PATH);
	if ((fp = fopen (fname, "w")) != NULL) {
		char gpio_num[4];
		memset (gpio_num, 0x00, sizeof(gpio_num));
		sprintf (gpio_num, "%d", gpio);
		fwrite (gpio_num, strlen(gpio_num), 1, fp);
		fclose (fp);
		return	0;
	}
	return	1;
}

//------------------------------------------------------------------------------
static int gpio_direction (int gpio, int status)
{
	char fname[256];
	FILE *fp;

	memset (fname, 0x00, sizeof(fname));
	sprintf (fname, "%s/gpio%d/direction", GPIO_CONTROL_PATH, gpio);
	if ((fp = fopen (fname, "w")) != NULL) {
		char gpio_status[4];
		memset (gpio_status, 0x00, sizeof(gpio_status));
		sprintf (gpio_status, "%s", gpio_status ? "out" : "in");
		fwrite (gpio_status, strlen(gpio_status), 1, fp);
		fclose (fp);
		return	0;
	}
	return	1;
}
//------------------------------------------------------------------------------
static int gpio_set_value (int gpio, int s_value)
{
	char fname[256];
	FILE *fp;

	memset (fname, 0x00, sizeof(fname));
	sprintf (fname, "%s/gpio%d/value", GPIO_CONTROL_PATH, gpio);
	if ((fp = fopen (fname, "w")) != NULL) {
		fputc (s_value ? '1' : '0', fp);
		fclose (fp);
		return	0;
	}
	return	1;
}

//------------------------------------------------------------------------------
static int gpio_get_value (int gpio, int *g_value)
{
	char fname[256];
	FILE *fp;

	memset (fname, 0x00, sizeof(fname));
	sprintf (fname, "%s/gpio%d/value", GPIO_CONTROL_PATH, gpio);
	if ((fp = fopen (fname, "r")) != NULL) {
		*g_value = (fgetc (fp) - '0');
		fclose (fp);
		return	0;
	}
	return	1;
}
//------------------------------------------------------------------------------
static int gpio_unexport (int gpio)
{
	char fname[256];
	FILE *fp;

	memset (fname, 0x00, sizeof(fname));
	sprintf (fname, "%s/unexport", GPIO_CONTROL_PATH);
	if ((fp = fopen (fname, "w")) != NULL) {
		char gpio_num[4];
		memset (gpio_num, 0x00, sizeof(gpio_num));
		sprintf (gpio_num, "%d", gpio);
		fwrite (gpio_num, strlen(gpio_num), 1, fp);
		fclose (fp);
		return	0;
	}
	return	1;
}

//------------------------------------------------------------------------------
static int header40_set_pattern (__u16 pattern)
{
	int i;
	for (i = 0; i < HEADER40_PINS; i++) {
		if (!HeaderToGPIO[i])
			continue;

		if (header40[i].is_file)
			header40[i].status =
				!gpio_set_value (HeaderToGPIO[i], Patterns[pattern][i]);

		if (!header40[i].status) {
			info ("Error Set Value GPIO%d, %d\n",
					HeaderToGPIO[i], Patterns[pattern][i]);
			return	0;
		}
	}
	return	1;
}

//------------------------------------------------------------------------------
void header40_test_init(void)
{
	int i;

	memset (header40, 0x00, sizeof(header40));
	for (i = 0; i < HEADER40_PINS; i++) {
		if (HeaderToGPIO[i]) {
			header40[i].is_file = !gpio_export (HeaderToGPIO[i]);
			if (header40[i].is_file)
				gpio_direction (HeaderToGPIO[i], GPIO_DIR_OUT);
		}
	}
}

//------------------------------------------------------------------------------
enum ITEM_PATTERNS {
	ITEM_PATTERN_0 = 0,
	ITEM_PATTERN_1,
	ITEM_PATTERN_2,
	ITEM_PATTERN_3,
};

//------------------------------------------------------------------------------
int header40_test_func (char *msg, char *resp_msg)
{
	char	*ptr, msg_clone[20], item, item_is_port;

	memset (msg_clone, 0x00, sizeof(msg_clone));
	sprintf (msg_clone, "%s", msg);

	if ((ptr = strtok (msg_clone, ",")) != NULL) {
		ptr = toupperstr(ptr);

		if 		(!strncmp(ptr, "PATTERN_0", sizeof("PATTERN_0")))
			item = ITEM_PATTERN_0;
		else if	(!strncmp(ptr, "PATTERN_1", sizeof("PATTERN_1")))
			item = ITEM_PATTERN_1;
		else if	(!strncmp(ptr, "PATTERN_2", sizeof("PATTERN_2")))
			item = ITEM_PATTERN_2;
		else if	(!strncmp(ptr, "PATTERN_3", sizeof("PATTERN_3")))
			item = ITEM_PATTERN_3;
		else {
			info ("err : unknown msg! %s\n", ptr);
			return -1;
		}

		/* resp msg : [status, get_speed-read mb/s */
		sprintf (resp_msg, "%d,%d",	header40_set_pattern(item), item);
		return	0;
	}
	info ("msg null pointer err!\n");
	return -1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void header40_test_exit (void)
{
	int i;
	info ("%s\n", __func__);

	memset (header40, 0x00, sizeof(header40));
	for (i = 0; i < HEADER40_PINS; i++) {
		if (HeaderToGPIO[i]) {
			header40[i].is_file = !gpio_unexport (HeaderToGPIO[i]);
		}
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define	TEST_MSG_COUNT	5
__s8 test_msg[TEST_MSG_COUNT][20] = {
	"pattern_0",
	"pattern_1",
	"pattern_2",
	"pattern_3",
	"unknown",
};

//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	int count = 0, ret = 0;
	__s8 resp_msg[20];

	memset (resp_msg, 0x00, sizeof(resp_msg));
	header40_test_init();

	while (true) {
		if (!(ret = header40_test_func(test_msg[count % TEST_MSG_COUNT], resp_msg))) {
			info ("count = %d, msg_len = %02d, resp msg = %s\n", count, (int)strlen(resp_msg), resp_msg);
			memset (resp_msg, 0x00, sizeof(resp_msg));
		} else {
			info ("header40 device error! %d\n", count);
		}
		count++;
		if (count > TEST_MSG_COUNT-1)
			count = 0;
		sleep(2);
	}

	header40_test_exit ();
	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
