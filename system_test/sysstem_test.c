//------------------------------------------------------------------------------
//
// 2022.09.22 ODROID-N2 Lite SYSTEM Info test. (chalres-park)
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

#include <sys/sysinfo.h>

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
#define	SYSTEM_DISPLAY_SIZE	"/sys/class/graphics/fb0/virtual_size"

// Client HDMI connect to VU5 (800x480)
#define	SYSTEM_LCD_SIZE_X	800
#define	SYSTEM_LCD_SIZE_Y	480

struct system_test_t {
	int		mem_size;	// GB size
	int		disp_x, disp_y;
};

struct system_test_t client_system;
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void system_test_init (void)
{
	FILE	*fp;
	struct 	sysinfo sinfo;
	__u32	mem_size;

	memset (&client_system, 0x00, sizeof(client_system));

	if (access (SYSTEM_DISPLAY_SIZE, R_OK) == 0) {

		if ((fp = fopen(SYSTEM_DISPLAY_SIZE, "r")) != NULL) {
			char rdata[16], *ptr;
			memset (rdata, 0x00, sizeof(rdata));
			if (fgets (rdata, sizeof(rdata), fp) != NULL) {
				if ((ptr = strtok (rdata, ",")) != NULL)
					client_system.disp_x = atoi(ptr);
				if ((ptr = strtok (NULL, ",")) != NULL)
					client_system.disp_y = atoi(ptr) / 2;
			}
			fclose(fp);
		}
	}

	if (!sysinfo (&sinfo)) {
		if (sinfo.totalram) {
			mem_size = sinfo.totalram / 1024 / 1024;

			if		((mem_size > 4096) && (mem_size < 8192))
				client_system.mem_size = 8;
			else if	((mem_size > 2048) && (mem_size < 4096))
				client_system.mem_size = 4;
			else if ((mem_size > 1024) && (mem_size < 2048))
				client_system.mem_size = 2;
			else
				client_system.mem_size = 0;
		}
	}
	info ("lcd_x = %d, lcd_y = %d, mem = %d GB\n",
		client_system.disp_x, client_system.disp_y, client_system.mem_size);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int system_test_func (char *msg, char *resp_msg)
{
	char *ptr, msg_clone[20];

	info ("msg = %s\n", msg);
	memset (msg_clone, 0x00, sizeof(msg_clone));
	sprintf (msg_clone, "%s", msg);

	if ((ptr = strtok (msg_clone, ",")) != NULL) {
		ptr = toupperstr(ptr);
		if			(!strncmp(ptr, "MEM", sizeof("MEM"))) {
			sprintf (resp_msg, "%d,%d GB", 1, client_system.mem_size); 
		} else if	(!strncmp(ptr, "LCD", sizeof("LCD"))) {
			bool status;
			if ((client_system.disp_x != SYSTEM_LCD_SIZE_X) ||
				(client_system.disp_y != SYSTEM_LCD_SIZE_Y))
				status = 0;
			else
				status = 1;
			sprintf (resp_msg, "%d,%dx%d", status, client_system.disp_x, client_system.disp_y); 
		} else {
			info ("err : unknown msg! %s\n", ptr);
			return -1;
		}
		return 0;
	}
	info ("msg null pointer err! %s\n", ptr);
	return -1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void system_test_exit (void)
{
	info ("%s\n", __func__);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define	TEST_MSG_COUNT	3
__s8 test_msg[TEST_MSG_COUNT][20] = {
	"mem",
	"lcd",
	"unknown",
};

//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	int count = 0, ret = 0;
	__s8 resp_msg[20];

	memset (resp_msg, 0x00, sizeof(resp_msg));
	system_test_init();

	while (true) {
		if (!(ret = system_test_func(test_msg[count % TEST_MSG_COUNT], resp_msg))) {
			info ("count = %d, msg_len = %02d, resp msg = %s\n", count, (int)strlen(resp_msg), resp_msg);
			memset (resp_msg, 0x00, sizeof(resp_msg));
		} else {
			info ("system device error! %d\n", count);
		}
		count++;
		if (count > (TEST_MSG_COUNT -1))
			count = 0;
		sleep(2);
	}

	system_test_exit ();
	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
