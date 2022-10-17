//------------------------------------------------------------------------------
//
// 2022.09.23 ODROID-N2 Lite STORAGE test(READ Only). (chalres-park)
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
#define	STORAGE_EMMC_DEV	"/dev/mmcblk0"
#define	STORAGE_SD_DEV		"/dev/mmcblk1"

#define	STORAGE_EMMC		0
#define	STORAGE_SD			1

// default check speed
#define	STORAGE_EMMC_SPEED	140
#define	STORAGE_SD_SPEED	40

// Storage Read (16 Mbytes, 1 block count)
#define	STORAGE_TEST_FLAG	"dd of=/dev/null bs=16M count=1 iflag=nocache,dsync oflag=nocache,dsync if="

struct storage_test_t {
	bool	is_file;
	bool	status;
	int		speed;
};

struct storage_test_t	storage[2];
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void storage_test_init (void)
{
	FILE	*fp;
	char	cmd[256];
	char rdata[256], *ptr;

	memset (rdata, 0x00, sizeof(rdata));
	memset (storage, 0x00, sizeof(storage));

	if (access (STORAGE_EMMC_DEV, R_OK) == 0) {
		storage[STORAGE_EMMC].is_file = true;

		memset (cmd, 0x00, sizeof(cmd));
		sprintf (cmd, "%s%s 2<&1", STORAGE_TEST_FLAG, STORAGE_EMMC_DEV);
		if ((fp = popen(cmd, "r")) != NULL) {
			while (fgets (rdata, sizeof(rdata), fp) != NULL) {
				info ("fgets = %s\n", rdata);
				if ((ptr = strstr (rdata, " MB/s")) != NULL) {
					while (*ptr != ',')		ptr--;

					storage[STORAGE_EMMC].speed = atoi(ptr+1);
					info ("find emmc speed = %d MB/s\n",
									storage[STORAGE_EMMC].speed);
				}
			}
			pclose(fp);
		}
	}

	if (access (STORAGE_SD_DEV, R_OK) == 0) {
		storage[STORAGE_SD].is_file = true;

		memset (cmd, 0x00, sizeof(cmd));
		sprintf (cmd, "%s%s 2<&1", STORAGE_TEST_FLAG, STORAGE_SD_DEV);
		if ((fp = popen(cmd, "r")) != NULL) {
			while (fgets (rdata, sizeof(rdata), fp) != NULL) {
				info ("fgets = %s\n", rdata);
				if ((ptr = strstr (rdata, " MB/s")) != NULL) {
					while (*ptr != ',')		ptr--;

					storage[STORAGE_SD].speed = atoi(ptr+1);
					info ("find sd speed = %d MB/s\n",
									storage[STORAGE_SD].speed);
				}
			}
			pclose(fp);
		}
	}
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int storage_test_func (char *msg, char *resp_msg)
{
	char *ptr, msg_clone[20], item;
	int	ret;

	info ("msg = %s\n", msg);
	memset (msg_clone, 0x00, sizeof(msg_clone));
	sprintf (msg_clone, "%s", msg);

	if ((ptr = strtok (msg_clone, ",")) != NULL) {
		ptr = toupperstr(ptr);
		if			(!strncmp(ptr, "EMMC", sizeof("EMMC"))) {
			item = STORAGE_EMMC;
			storage[item].status = storage[item].speed > STORAGE_EMMC_SPEED ? 1 : 0;
		} else if	(!strncmp(ptr, "SD", sizeof("SD"))) {
			item = STORAGE_SD;
			storage[item].status = storage[item].speed > STORAGE_SD_SPEED ? 1 : 0;
		} else {
			info ("msg unknown");
			return -1;
		}
		sprintf (resp_msg, "%d,%d MB/s", storage[item].status, storage[item].speed); 
		return	0;
	} 
	info ("msg null pointer err!\n");
	return -1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void storage_test_exit (void)
{
	info ("%s\n", __func__);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define	TEST_MSG_COUNT	3
__s8 test_msg[TEST_MSG_COUNT][20] = {
	"sd",
	"emmc",
	"unknown",
};

//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	int count = 0, ret = 0;
	__s8 resp_msg[20];

	memset (resp_msg, 0x00, sizeof(resp_msg));
	storage_test_init();

	while (true) {
		if (!(ret = storage_test_func(test_msg[count % TEST_MSG_COUNT], resp_msg))) {
			info ("count = %d, msg_len = %02d, resp msg = %s\n", count, (int)strlen(resp_msg), resp_msg);
			memset (resp_msg, 0x00, sizeof(resp_msg));
		} else {
			info ("storage device error! %d\n", count);
		}
		count++;
		if (count > (TEST_MSG_COUNT -1))
			count = 0;
		sleep(2);
	}

	storage_test_exit ();
	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
