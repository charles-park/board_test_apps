//------------------------------------------------------------------------------
//
// 2022.09.23 ODROID-N2 Lite USB Mess-storage test. (chalres-park)
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
// USB Connector & Position name
//------------------------------------------------------------------------------
/*
	+---------------+
	| USB 3 | USB 4 |	USB3 : LEFT_UP,	USB4 : RIGHT_UP
	+-------+-------+
	| USB 1 | USB 2 |	USB1 : LEFT_DN,	USB2 : RIGHT_DN
	+-------+-------+ 

	ODROID-N2L (BACK View)
	                 +----------------+
	                 | USB 3 (USB3.0) |	USB3 : LEFT_UP (USB30/USB20)
	+-----+ +------+ +----------------+
	| PWR | | HDMI | | USB 1 (USB2.0) |	USB1 : LEFT_DN (USB20)
	+-----+ +------+ +----------------+
*/
//------------------------------------------------------------------------------
/* USB spec, CON pos, Detect Filename : 5000, 480, 12 */
//------------------------------------------------------------------------------
// Storage Read (16 Mbytes, 1 block count)
//------------------------------------------------------------------------------
#define	STORAGE_TEST_FLAG			"dd of=/dev/null bs=16M count=1 iflag=nocache,dsync oflag=nocache,dsync if=/dev/"

#define USB20_LEFT_DN_FILENAME		"/sys/bus/usb/devices/1-2"
#define USB30_LEFT_DN_FILENAME		""

#define USB20_LEFT_UP_FILENAME		"/sys/bus/usb/devices/1-1"
#define USB30_LEFT_UP_FILENAME		"/sys/bus/usb/devices/2-1"

//------------------------------------------------------------------------------
#define	USB_CON_L_DN	0
#define	USB_CON_R_DN	1
#define	USB_CON_L_UP	2
#define	USB_CON_R_UP	3

//------------------------------------------------------------------------------
struct usb_test_t {
	bool	status;
	int		speed;
	bool	is_mass;
	char	mass_node[8];
	int		r_speed;
};

static struct usb_test_t usb[4];

//------------------------------------------------------------------------------
bool get_usb_speed (struct usb_test_t *usb, __s8 *filename, __s32 check_speed)
{
	if (access(filename, R_OK) == 0) {
		FILE *fp;
		char rdata[256], *ptr;

		memset (rdata, 0x00, sizeof(rdata));
		sprintf (rdata, "%s/speed", filename);

		if ((fp = fopen(rdata, "r")) != NULL) {
			memset (rdata, 0x00, sizeof(rdata));
			fgets (rdata, sizeof(rdata), fp);
			fclose(fp);

			usb->speed 	= atoi(rdata);
			usb->status	= (check_speed != usb->speed) ? false : true;

			memset (rdata, 0x00, sizeof(rdata));
			sprintf (rdata, "find %s/ -name sd* 2<&1", filename);
			// mass-storage check
			if ((fp = popen(rdata, "r")) != NULL) {
				memset (rdata, 0x00, sizeof(rdata));
				fgets (rdata, sizeof(rdata), fp);
				pclose(fp);
				if ((ptr = strstr (rdata, "sd")) != NULL) {
					usb->is_mass = true;
					strncpy (usb->mass_node, ptr, strlen(ptr)-1);
					memset (rdata, 0x00, sizeof(rdata));
					sprintf (rdata, "%s%s 2<&1", STORAGE_TEST_FLAG, usb->mass_node);
					if ((fp = popen(rdata, "r")) != NULL) {
						memset (rdata, 0x00, sizeof(rdata));
						while (fgets (rdata, sizeof(rdata), fp) != NULL) {
							if ((ptr = strstr (rdata, " MB/s")) != NULL) {
								while (*ptr != ',')		ptr--;

								usb->r_speed = atoi(ptr+1);
								info ("find mass-storage node = /dev/%s, read speed = %d MB/s\n",
											usb->mass_node, usb->r_speed);
							}
						}
						pclose(fp);
					}
				}
			}
			info ("status = %d, usb speed = %d, fname = %s, is_mass = %d\n",
								usb->status, usb->speed, filename, usb->is_mass);
		}
	}
	return	usb->status;
}

//------------------------------------------------------------------------------
void usb_test_init (void)
{
	memset (usb, 0x00, sizeof(usb));

	info ("------------------------------------------------------------\n");
	get_usb_speed (&usb[USB_CON_L_DN], USB20_LEFT_DN_FILENAME, 480);
	if (!get_usb_speed (&usb[USB_CON_L_UP], USB30_LEFT_UP_FILENAME, 5000))
		 get_usb_speed (&usb[USB_CON_L_UP], USB20_LEFT_UP_FILENAME, 5000);
	info ("------------------------------------------------------------\n");
}

//------------------------------------------------------------------------------
// msg desc : CON hpos(up, dn), CON vpos(left, right)
// return : 0 -> sucess, -1 -> msg error
//------------------------------------------------------------------------------
int usb_test_func (char *msg, char *resp_msg)
{
	char	*ptr, msg_clone[20], item, item_is_port;

	memset (msg_clone, 0x00, sizeof(msg_clone));
	sprintf (msg_clone, "%s", msg);

	if ((ptr = strtok (msg_clone, ",")) != NULL) {
		ptr = toupperstr(ptr);

		if 			(!strncmp(ptr, "L_DN_PORT", sizeof("L_DN_PORT"))) {
			item = USB_CON_L_DN;	item_is_port = true;
		} else  if	(!strncmp(ptr, "L_UP_PORT", sizeof("L_UP_PORT"))) {
			item = USB_CON_L_UP;	item_is_port = true;
		} else  if	(!strncmp(ptr, "R_DN_PORT", sizeof("R_DN_PORT"))) {
			item = USB_CON_R_DN;	item_is_port = true;
		} else  if	(!strncmp(ptr, "R_UP_PORT", sizeof("R_UP_PORT"))) {
			item = USB_CON_R_UP;	item_is_port = true;
		} else  if	(!strncmp(ptr, "L_DN_READ", sizeof("L_DN_READ"))) {
			item = USB_CON_L_DN;	item_is_port = false;
		} else  if	(!strncmp(ptr, "L_UP_READ", sizeof("L_UP_READ"))) {
			item = USB_CON_L_UP;	item_is_port = false;
		} else  if	(!strncmp(ptr, "R_DN_READ", sizeof("R_DN_READ"))) {
			item = USB_CON_R_DN;	item_is_port = false;
		} else  if	(!strncmp(ptr, "R_UP_READ", sizeof("R_UP_READ"))) {
			item = USB_CON_R_UP;	item_is_port = false;
		} else {
			info ("err : unknown msg! %s\n", ptr);
			return -1;
		}
		/* resp msg : [status, get_speed-read mb/s */
		if (item_is_port)
			sprintf (resp_msg, "%d,%d",	usb[item].status, usb[item].speed);
		else
			sprintf (resp_msg, "%d,%d MB/s",
				usb[item].status, usb[item].is_mass ? usb[item].r_speed : 0);

		return	0;
	}
	info ("msg null pointer err!\n");
	return -1;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
void usb_test_exit (void)
{
	info ("%s\n", __func__);
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define	TEST_MSG_COUNT	9
__s8 test_msg[TEST_MSG_COUNT][20] = {
	"l_dn_port",
	"l_up_port",
	"r_dn_port",
	"r_up_port",
	"l_dn_read",
	"l_up_read",
	"r_dn_read",
	"r_up_read",
	"unknown",
};

//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	int count = 0, ret = 0;
	__s8 resp_msg[20];

	memset (resp_msg, 0x00, sizeof(resp_msg));
	usb_test_init();

	while (true) {
		if (!(ret = usb_test_func(test_msg[count % TEST_MSG_COUNT], resp_msg))) {
			info ("count = %d, msg_len = %02d, resp msg = %s\n", count, (int)strlen(resp_msg), resp_msg);
			memset (resp_msg, 0x00, sizeof(resp_msg));
		} else {
			info ("usb device error! %d\n", count);
		}
		count++;
		if (count > TEST_MSG_COUNT-1)
			count = 0;
		sleep(2);
	}

	usb_test_exit ();
	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
