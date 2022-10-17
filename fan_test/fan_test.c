//------------------------------------------------------------------------------
//
// 2022.09.21 ODROID-N2 Lite FAN test. (chalres-park)
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
// FAN Control File & Value
//------------------------------------------------------------------------------
#define FAN_CONTROL_FILE	"/sys/devices/platform/pwm-fan/hwmon/hwmon0/pwm1_enable"
#define	FAN_ENABLE			1
#define	FAN_DISABLE			0

struct fan_test_t {
	bool	is_file;
	bool	status;
	bool	enable;
};

static struct fan_test_t fan;

//------------------------------------------------------------------------------
int fan_control (bool enable)
{
	__s8	rdata;
	FILE 	*fp;

	// fan control set
	if ((fp = fopen(FAN_CONTROL_FILE, "w")) != NULL) {
		fan.enable = enable;
		fputc (fan.enable ? '1' : '0', fp);
		fclose(fp);
	}
	
	// fan control get
	if ((fp = fopen(FAN_CONTROL_FILE, "r")) != NULL) {
		rdata = fgetc (fp);
		fclose(fp);
	}

	info ("rdata = %c, fan.enable = %c\n", rdata, fan.enable + '0');
	
	return	(rdata != (fan.enable + '0')) ? false : true;
}

//------------------------------------------------------------------------------
void fan_test_init (void)
{
	memset (&fan, 0x00, sizeof(fan));
	if (access(FAN_CONTROL_FILE, R_OK) == 0)	fan.is_file = true;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int fan_test_func (char *msg, char *resp_msg)
{
	char 	*ptr, msg_clone[20];
	int		ret;

	memset (msg_clone, 0x00, sizeof(msg_clone));
	sprintf (msg_clone, "%s", msg);

	if (!fan.is_file) {
		info ("Err file : %s not found!\n", FAN_CONTROL_FILE);
		return -1;
	}

	if ((ptr = strtok (msg_clone, ",")) != NULL) {
		ptr = toupperstr(ptr);

		if      	(!strncmp(ptr, "ON", sizeof("ON"))) {
			fan.status = true;
			ret = fan_countrol (fan.status);
		} else if 	(!strncmp(ptr, "OFF", sizeof("OFF"))) {
			fan.status = false;
			ret = fan_countrol (fan.status);
		} else {
			info ("err : unknown msg! %s\n", ptr);
			return -1;
		}
		/* resp msg : [status, control status] */
		sprintf (resp_msg, "%d,%d", ret, fan.status);
		return	0;
	}
	info ("msg null pointer err!\n");
	return -1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void fan_test_exit (void)
{
	info ("%s\n", __func__);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define	TEST_MSG_COUNT	3
__s8 test_msg[TEST_MSG_COUNT][20] = {
	"on",
	"off",
	"unknown",
};

//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	int count = 0, ret = 0;
	__s8 resp_msg[20];

	memset (resp_msg, 0x00, sizeof(resp_msg));
	fan_test_init();

	while (true) {
		if (!(ret = fan_test_func(test_msg[count % TEST_MSG_COUNT], resp_msg))) {
			info ("count = %d, msg_len = %02d, resp msg = %s\n", count, (int)strlen(resp_msg), resp_msg);
			memset (resp_msg, 0x00, sizeof(resp_msg));
		} else {
			info ("fan device error! %d\n", count);
		}
		count++;
		if (count > TEST_MSG_COUNT-1)
			count = 0;
		sleep(2);
	}

	fan_test_exit ();
	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
