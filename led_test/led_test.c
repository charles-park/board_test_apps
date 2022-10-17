//------------------------------------------------------------------------------
//
// 2022.09.22 ODROID-N2 Lite LED test. (chalres-park)
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
//------------------------------------------------------------------------------
#define	GPIO_CTRL_PATH		"/sys/class/gpio"
#define LED_POWER_GPIO		502
#define LED_POWER_CTRL		"value"
#define	LED_POWER_ON		1
#define	LED_POWER_OFF		0

#define	LED_ALIVE_PATH		"/sys/class/leds/blue:heartbeat"
#define LED_ALIVE_TRIGGER	"trigger"
#define	LED_ALIVE_CTRL		"brightness"
#define	LED_ALIVE_ON		255
#define	LED_ALIVE_OFF		0

#define	LED_POWER		0
#define	LED_ALIVE		1

struct led_test_t {
	bool	is_file;
	bool	status;
	char	filename[256];
};

struct led_test_t led[2];

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void led_test_init (void)
{
	FILE	*fp;
	char	cmd[128];
	
	memset (led, 0x00, sizeof(led));

	memset (cmd, 0x00, sizeof(cmd));
	sprintf (cmd, "echo none > %s/%s && sync",
				LED_ALIVE_PATH, LED_ALIVE_TRIGGER);
	if (NULL == (fp = popen(cmd, "w")))	{
		err("popen() error!\n");
		led[LED_ALIVE].is_file = led[LED_POWER].is_file = false;
		return;
	}
	memset (cmd, 0x00, sizeof(cmd));
	sprintf (cmd, "echo %d > %s/export && sync\n",
				LED_POWER_GPIO, GPIO_CTRL_PATH);
	fprintf (fp, "%s", cmd);

	memset (cmd, 0x00, sizeof(cmd));
	sprintf (cmd, "echo out > %s/gpio%d/direction && sync\n",
				GPIO_CTRL_PATH, LED_POWER_GPIO);
	fprintf (fp, "%s", cmd);
	pclose(fp);

	// confirm /sys/class/leds/blue:heartbeat/brightness
	sprintf (led[LED_ALIVE].filename, "%s/%s",
				LED_ALIVE_PATH, LED_ALIVE_CTRL);
	if (access (led[LED_ALIVE].filename, R_OK) == 0)
		led[LED_ALIVE].is_file = true;

	// confirm /sys/class/gpio502/value
	sprintf (led[LED_POWER].filename, "%s/gpio%d/%s",
				GPIO_CTRL_PATH, LED_POWER_GPIO, LED_POWER_CTRL);
	if (access (led[LED_POWER].filename, R_OK) == 0)
		led[LED_POWER].is_file = true;
}

//------------------------------------------------------------------------------
int led_write (char *filename, bool onoff)
{
	FILE 	*fp;
	char	rdata;

	// led control set
	if ((fp = fopen(filename, "w")) != NULL) {
		fputc (onoff ? '1' : '0', fp);
		fclose(fp);
	}
	
	// led control get
	if ((fp = fopen(filename, "r")) != NULL) {
		rdata = fgetc (fp);
		fclose(fp);
	}

	info ("rwdata = %c, ctrl_data = %d\n", rdata, onoff);
	
	return	(rdata != (onoff + '0')) ? 1 : 0;
}

//------------------------------------------------------------------------------
int led_test_func (char *msg, char *resp_msg)
{
	char *ptr, msg_clone[20], fname[128];
	int	ret, ctrl_led;

	info ("msg = %s\n", msg);
	memset (msg_clone, 0x00, sizeof(msg_clone));
	sprintf (msg_clone, "%s", msg);

	memset (fname, 0x00, sizeof(fname));
	if ((ptr = strtok (msg_clone, ",")) != NULL) {
		ptr = toupperstr(ptr);

		if			(!strncmp(ptr, "ALIVE_ON", sizeof("ALIVE_ON"))) {
			ctrl_led = LED_ALIVE;	led[ctrl_led].status = true;
		} else if	(!strncmp(ptr, "ALIVE_OFF", sizeof("ALIVE_OFF"))) {
			ctrl_led = LED_ALIVE;	led[ctrl_led].status = false;
		} else if	(!strncmp(ptr, "POWER_ON", sizeof("POWER_ON"))) {
			ctrl_led = LED_POWER;	led[ctrl_led].status = true;
		} else if	(!strncmp(ptr, "POWER_OFF", sizeof("POWER_OFF"))) {
			ctrl_led = LED_POWER;	led[ctrl_led].status = false;
		} else {
			info ("msg unknown");
			return -1;
		}
		if (!led[ctrl_led].is_file) {
			info ("Err file : %s not found.", led[ctrl_led].filename);
			return -1;
		}

		ret = led_write (led[ctrl_led].filename, led[ctrl_led].status);
		/* resp msg : [status, led status, adc2, ] */
		sprintf (resp_msg, "%d,%d", ret, led[ctrl_led].status);
		return	0;

	}
	info ("msg null pointer err!\n");
	return -1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void led_test_exit (void)
{
	info ("%s\n", __func__);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/* channel, play time */
#define TEST_MSG_COUNT	5
__s8 test_msg[TEST_MSG_COUNT][20] = {
	"alive_on",
	"alive_off",
	"power_on",
	"power_off",
	"unknown",
};

//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	int count = 0, ret = 0;
	__s8 resp_msg[20];

	memset (resp_msg, 0x00, sizeof(resp_msg));
	led_test_init();

	while (true) {
		if (!(ret = led_test_func(test_msg[count % TEST_MSG_COUNT], resp_msg))) {
			info ("count = %d, msg_len = %02d, resp msg = %s\n", count, (int)strlen(resp_msg), resp_msg);
			memset (resp_msg, 0x00, sizeof(resp_msg));
		} else {
			info ("led device error! %d\n", count);
		}

		count++;
		if (count > (TEST_MSG_COUNT - 1))
			count = 0;
		sleep(2);
	}

	led_test_exit ();
	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
