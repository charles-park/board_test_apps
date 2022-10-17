//------------------------------------------------------------------------------
//
// 2022.09.22 ODROID-N2 Lite HDMI test. (chalres-park)
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
#define	HDMI_CTRL_PATH			"/sys/class/amhdmitx/amhdmitx0"
#define HDMI_HPD_STATUS			"hpd_state"
#define	HDMI_EDID_STRING		"rawedid"
#define	HDMI_EDID_PASS			"00ffffffffffff00"

struct hdmi_test_t {
	bool	status;
};

#define	HDMI_HPD	0
#define	HDMI_EDID	1

struct hdmi_test_t hdmi[2];
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void hdmi_test_init (void)
{
	FILE	*fp;
	char	filename[256];

	memset (hdmi, 0x00, sizeof(hdmi));

	memset (filename, 0x00, sizeof(filename));
	sprintf(filename, "%s/%s", HDMI_CTRL_PATH, HDMI_HPD_STATUS);
	info ("%s\n\n", filename );
	if (access (filename, R_OK) == 0) {
		if ((fp = fopen(filename, "r")) != NULL) {
			hdmi[HDMI_HPD].status = (fgetc(fp) == '1') ? 1 : 0;
			fclose(fp);
		}
	}

	memset (filename, 0x00, sizeof(filename));
	sprintf(filename, "%s/%s", HDMI_CTRL_PATH, HDMI_EDID_STRING);
	info ("%s\n\n", filename );
	if (access (filename, R_OK) == 0) {
		if ((fp = fopen(filename, "r")) != NULL) {
			char	rdata[16];

			memset (rdata, 0x00, sizeof(rdata));
			fgets (rdata, sizeof(rdata), fp);
			// HDMI_EDID_PASS string is edid pass.
			if (!strncmp(rdata, HDMI_EDID_PASS, sizeof(HDMI_EDID_PASS)))
				hdmi[HDMI_EDID].status = true;
			fclose(fp);
		}
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int hdmi_test_func (char *msg, char *resp_msg)
{
	char *ptr, msg_clone[20], item;

	memset (msg_clone, 0x00, sizeof(msg_clone));
	sprintf (msg_clone, "%s", msg);

	if ((ptr = strtok (msg_clone, ",")) != NULL) {
		ptr = toupperstr(ptr);

		if		(!strncmp(ptr, "HPD", sizeof("HPD")))
			item = HDMI_HPD;
		else if (!strncmp(ptr, "EDID", sizeof("EDID")))
			item = HDMI_EDID;
		else {
			info ("err : unknown msg! %s\n", ptr);
			return -1;
		}
		/* resp msg : [status,] */
		sprintf (resp_msg, "%d", hdmi[item].status);
		return	0;
	}
	info ("msg null pointer err!\n");
	return -1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void hdmi_test_exit (void)
{
	info ("%s\n", __func__);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define	TEST_MSG_COUNT	3
__s8 test_msg[TEST_MSG_COUNT][20] = {
	"edid",
	"hpd",
	"unknown",
};

//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	int count = 0, ret = 0;
	__s8 resp_msg[20];

	memset (resp_msg, 0x00, sizeof(resp_msg));
	hdmi_test_init();

	while (true) {
		if (!(ret = hdmi_test_func(test_msg[count % TEST_MSG_COUNT], resp_msg))) {
			info ("count = %d, msg_len = %02d, resp msg = %s\n", count, (int)strlen(resp_msg), resp_msg);
			memset (resp_msg, 0x00, sizeof(resp_msg));
		} else {
			info ("hdmi device error! %d\n", count);
		}
		count++;
		if (count > TEST_MSG_COUNT-1)
			count = 0;
		sleep(2);
	}

	hdmi_test_exit ();
	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
