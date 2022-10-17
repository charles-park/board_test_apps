//------------------------------------------------------------------------------
//
// 2022.09.20 ODROID-N2 Lite ADC test. (chalres-park)
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
/* adc voltage cal = 1.8V / 4096 * raw value */
//------------------------------------------------------------------------------
// HEADER 20x2 (40), 441mV(FIX)
#define ADC_40_FILENAME		"/sys/bus/iio/devices/iio:device0/in_voltage2_raw"
#define	ADC_40_IN_MAX		500
#define	ADC_40_IN_MIN		400

// HEADER 20x2 (37), 1350mV(FIX)
#define ADC_37_FILENAME		"/sys/bus/iio/devices/iio:device0/in_voltage3_raw"
#define	ADC_37_IN_MAX		1400
#define	ADC_37_IN_MIN		1300

#define	ADC_37	0
#define	ADC_40	1

struct adc_test_t {
	bool	status;
	int		value;
	int		voltage;
};

struct adc_test_t adc[2];
//------------------------------------------------------------------------------
int adc_read (char *filename)
{
	__s8	rdata[16];
	FILE 	*fp;

	if (access (filename, R_OK) != 0)
		return -1;

	// adc raw value get
	if ((fp = fopen(filename, "r")) != NULL) {
		fgets (rdata, sizeof(rdata), fp);
		fclose(fp);
	}
	info ("filename = %s, rdata = %s\n",filename, rdata);
	
	return	(atoi(rdata));
}

//------------------------------------------------------------------------------
#define	RETRY_COUNT		5
//------------------------------------------------------------------------------
void adc_test_init (void)
{
	int retry, i;

	memset (adc, 0x00, sizeof(adc));
	info ("------------------------------------------------------------\n");
	if (access (ADC_37_FILENAME, R_OK) == 0) {
		for (i = 0, retry = RETRY_COUNT; i < retry; i++) {
			adc[ADC_37].value = adc_read (ADC_37_FILENAME);
			adc[ADC_37].voltage = 1800 * adc[ADC_37].value / 4096;
			if ((adc[ADC_37].voltage > ADC_37_IN_MIN) && 
				(adc[ADC_37].voltage < ADC_37_IN_MAX) ) {
				adc[ADC_37].status = true;
				break;
			}
		} 
	}	

	if (access (ADC_40_FILENAME, R_OK) == 0) {
		for (i = 0, retry = RETRY_COUNT; i < retry; i++) {
			adc[ADC_40].value = adc_read (ADC_40_FILENAME);
			adc[ADC_40].voltage = 1800 * adc[ADC_40].value / 4096;
			if ((adc[ADC_40].voltage > ADC_40_IN_MIN) && 
				(adc[ADC_40].voltage < ADC_40_IN_MAX) ) {
				adc[ADC_40].status = true;
				break;
			}
		}
	}	
	info ("------------------------------------------------------------\n");
}

//------------------------------------------------------------------------------
int adc_test_func (char *msg, char *resp_msg)
{
	char *ptr, msg_clone[20], item, item_is_mv;

	info ("msg = %s\n", msg);
	memset (msg_clone, 0x00, sizeof(msg_clone));
	sprintf (msg_clone, "%s", msg);

	if ((ptr = strtok (msg_clone, ",")) != NULL) {
		ptr = toupperstr(ptr);

		if      (!strncmp(ptr, "ADC_37", sizeof("ADC_37"))) {
			item = ADC_37;	item_is_mv = false;
		}
		else if (!strncmp(ptr, "ADC_40", sizeof("ADC_40"))) {
			item = ADC_40;	item_is_mv = false;
		}
		else if (!strncmp(ptr, "ADC_37_MV", sizeof("ADC_37_MV"))) {
			item = ADC_37;	item_is_mv = true;
		}
		else if (!strncmp(ptr, "ADC_40_MV", sizeof("ADC_40_MV"))) {
			item = ADC_40;	item_is_mv = true;
		} else {
			info ("err : unknown msg! %s\n", ptr);
			return -1;
		}

		/* resp msg : [status, get_speed-read mb/s */
		if (item_is_mv)
			sprintf (resp_msg, "%d,%04d mV",
				adc[item].status, adc[item].voltage);
		else
			sprintf (resp_msg, "%d,%s",
				adc[item].status, adc[item].status ? "PASS" : "FAIL");
		return	0;
	}
	info ("msg null pointer err!\n");
	return -1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void adc_test_exit (void)
{
	info ("%s\n", __func__);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define	TEST_MSG_COUNT	5
__s8 test_msg[TEST_MSG_COUNT][20] = {
	"adc_37",
	"adc_40",
	"adc_37_mv",
	"adc_40_mv",
	"unknown",
};

//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	int count = 0, ret = 0;
	__s8 resp_msg[20];

	memset (resp_msg, 0x00, sizeof(resp_msg));
	adc_test_init();

	while (true) {
		if (!(ret = adc_test_func(test_msg[count % TEST_MSG_COUNT], resp_msg))) {
			info ("count = %d, msg_len = %02d, resp msg = %s\n", count, (int)strlen(resp_msg), resp_msg);
			memset (resp_msg, 0x00, sizeof(resp_msg));
		} else {
			info ("adc device error! %d\n", count);
		}
		count++;
		if (count > TEST_MSG_COUNT-1)
			count = 0;
		sleep(2);
	}

	adc_test_exit ();
	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
