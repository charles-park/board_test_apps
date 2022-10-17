//------------------------------------------------------------------------------
//
// 2022.09.20 ODROID-N2 Lite USB test. (chalres-park)
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
	-----------------
	| USB 1 | USB 2 |	USB1 : LEFT_UP,	USB2 : RIGHT_UP
	-----------------
	| USB 3 | USB 4 |	USB3 : LEFT_DN,	USB4 : RIGHT_DN
	-----------------

	ODROID-N2 Lite
	------------------
	| USB 1 (USB3.0) |	USB1 : USB30_LEFT_UP, USB20_LEFT_UP
	------------------
	| USB 3 (USB2.0) |	USB3 : USB20_LEFT_DN
	------------------
*/
//------------------------------------------------------------------------------
/* USB spec, CON pos, Detect Filename : 5000, 480, 12 */
//------------------------------------------------------------------------------
#define USB20_LEFT_DN_FILENAME		"/sys/bus/usb/devices/1-2/speed"
#define USB30_LEFT_DN_FILENAME		""

#define USB20_LEFT_UP_FILENAME		"/sys/bus/usb/devices/1-1/speed"
#define USB30_LEFT_UP_FILENAME		"/sys/bus/usb/devices/2-1/speed"

struct usb_test_t {
	bool	is_file;
	bool	speed_err;
	__s32	speed;
	__s32	read;
	__s32	write;
	__s32	idx;
};

#define	USB_CON_UP		0
#define	USB_CON_DN		1
#define	USB_CON_LEFT	0
#define	USB_CON_RIGHT	1

static struct usb_test_t usb[2][2];

//------------------------------------------------------------------------------
bool get_usb_speed (struct usb_test_t *usb, __s8 *filename, __s32 check_speed)
{
	if (access(filename, R_OK) == 0) {
		FILE *fp;
		char rdata[10];

		if ((fp = fopen(filename, "r")) != NULL) {
			fgets (rdata, sizeof(rdata)-1, fp);
			fclose(fp);
			usb->is_file = true;
		}
		else
			usb->is_file = false;

		usb->speed    	= atoi(rdata);
		usb->speed_err	= (check_speed != usb->speed) ? true : false;
		info ("speed_err = %d, usb speed = %d, fname = %s, idx = %d\n",
							usb->speed_err, usb->speed, filename, usb->idx);
	}
	else {
		usb->is_file = false;
	}

	return	usb->is_file ? false : true;
}

//------------------------------------------------------------------------------
void usb_test_init (void)
{
	memset (&usb[0][0], 0x00, sizeof(struct usb_test_t));

	if (get_usb_speed (&usb[USB_CON_LEFT][USB_CON_UP], USB30_LEFT_UP_FILENAME, 5000))
		get_usb_speed (&usb[USB_CON_LEFT][USB_CON_UP], USB20_LEFT_UP_FILENAME, 5000);
	usb[USB_CON_LEFT][USB_CON_UP].idx = 1;

	get_usb_speed (&usb[USB_CON_LEFT][USB_CON_DN], USB20_LEFT_DN_FILENAME, 480);
	usb[USB_CON_LEFT][USB_CON_DN].idx = 2;
}

//------------------------------------------------------------------------------
// msg desc : CON hpos(up, dn), CON vpos(left, right)
// return : 0 -> sucess, -1 -> msg error
//------------------------------------------------------------------------------
int usb_test_func (char *msg, char *resp_msg)
{
	char *ptr, msg_clone[20];
	bool con_hpos = false, con_vpos = false;

	info ("msg = %s\n", msg);
	memset (msg_clone, 0x00, sizeof(msg_clone));
	sprintf (msg_clone, "%s", msg);

	if ((ptr = strtok (msg_clone, ",")) != NULL) {
		ptr = toupperstr(ptr);

		if (!strncmp(ptr, "LEFT", sizeof("LEFT")))
			con_hpos = USB_CON_LEFT;

		if (!strncmp(ptr, "RIGHT", sizeof("RIGHT")))
			con_hpos = USB_CON_RIGHT;
	} else {
		goto err;
	}

	if ((ptr = strtok(NULL, ",")) != NULL) {
		ptr = toupperstr(ptr);

		info ("ptr = %s\n", ptr);
		if (!strncmp(ptr, "DOWN", sizeof("DOWN")))
			con_vpos = USB_CON_DN;

		if (!strncmp(ptr, "UP", sizeof("UP")))
			con_vpos = USB_CON_UP;
	} else {
		goto err;
	}

	info ("con_hpos = %d, con_vpos = %d\n", con_hpos, con_vpos);
	/* resp msg : [err, get_speed, read mb/s, write mb/s, usb index] */
	sprintf (resp_msg, "%d,%d,%d,%d,%d",
		usb[con_hpos][con_vpos].speed_err,
		usb[con_hpos][con_vpos].speed,
		usb[con_hpos][con_vpos].read,
		usb[con_hpos][con_vpos].write,
		usb[con_hpos][con_vpos].idx);

	info ("resp_msg len = %ld\n", strlen(resp_msg));
	return	0;
err:
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
/* channel, play time */
__s8 usb_test_msg[4][20] = {
	"left, up",
	"left, down",
	"right, up",	
	"right, down",	
};

//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	int count = 0, ret = 0;
	__s8 resp_msg[20];

	memset (resp_msg, 0x00, sizeof(resp_msg));
	usb_test_init();

	while (true) {
		if (!(ret = usb_test_func(usb_test_msg[count%4], resp_msg))) {
			info ("usb resp msg = %s, count = %d\n", resp_msg, count);
			memset (resp_msg, 0x00, sizeof(resp_msg));
		} else {
			info ("usb device error! %d\n", count);
		}
		count++;
		if (count > 3)
			count = 0;
		sleep(2);
	}

	usb_test_exit ();
	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
