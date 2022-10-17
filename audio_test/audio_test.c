//------------------------------------------------------------------------------
//
// 2022.09.19 ODROID-N2 Lite Audio test. (chalres-park)
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
#define AUDIO_LEFT_FILENAME			"/root/audio_test/1Khz_Left.wav"
#define AUDIO_RIGHT_FILENAME		"/root/audio_test/1Khz_Right.wav"
#define	AUDIO_PLAY_TIME				2

//------------------------------------------------------------------------------
struct audio_test_t {
	bool			enable;
	bool			is_busy;
	__s16			play_time;
	__s8			filename[128];
	pthread_t		pthread;
	pthread_mutex_t	mutex;
};

static struct audio_test_t audio;
//------------------------------------------------------------------------------
void *audio_test_thread (void *arg)
{
	FILE *fp;
	struct audio_test_t *paudio = (struct audio_test_t *)arg;
	__s8 cmd[256];

	while (true) {
		if (!paudio->is_busy && paudio->enable) {
			pthread_mutex_lock(&paudio->mutex);
			paudio->is_busy = true;
			pthread_mutex_unlock(&paudio->mutex);

			memset (cmd, 0x00, sizeof(cmd));
			sprintf (cmd, "aplay -Dhw:0,1 %s -d %d && sync", paudio->filename, paudio->play_time);
			if (NULL == (fp = popen(cmd, "r")))	{
				err("popen() error!\n");
				exit(1);
			}
			pclose(fp);

			pthread_mutex_lock(&paudio->mutex);
			paudio->enable = false;		paudio->is_busy = false;
			pthread_mutex_unlock(&paudio->mutex);
		}
		usleep(50);
	}
}

//------------------------------------------------------------------------------
// msg desc : channel(left, right, stop), play time (sec)
// test cmd = "aplay -Dhw:0,1 {audio_filename} -d {play_time}"
// return : -1 -> unknown file & status, 0 -> success, 1 -> device busy
//------------------------------------------------------------------------------
int audio_test_func (char *msg, char *resp_msg)
{
	if (!audio.is_busy) {
		char *ptr, msg_clone[20];

		info ("msg = %s\n", msg);
		memset (msg_clone, 0x00, sizeof(msg_clone));
		sprintf (msg_clone, "%s", msg);
		
		if ((ptr = strtok (msg_clone, ",")) != NULL) {
			info ("msg = %s, %s\n", msg, ptr);
			ptr = toupperstr(ptr);

			memset (audio.filename, 0x00, sizeof (audio.filename));
			if			(!strncmp(ptr, "L_CH", sizeof("L_CH"))) {
				if (access (AUDIO_LEFT_FILENAME, R_OK)) {
					info ("Err file : %s not found.", AUDIO_LEFT_FILENAME);
					return -1;
				}
				strncpy(audio.filename, AUDIO_LEFT_FILENAME, sizeof(AUDIO_LEFT_FILENAME));
			} else if 	(!strncmp(ptr, "R_CH", sizeof("R_CH"))) {
				if (access (AUDIO_RIGHT_FILENAME, R_OK)) {
					info ("Err file : %s not found.", AUDIO_RIGHT_FILENAME);
					return -1;
				}
				strncpy(audio.filename, AUDIO_RIGHT_FILENAME, sizeof(AUDIO_RIGHT_FILENAME));
			}	else {
				info ("unknown msg! msg = %s\n", msg);
				return -1;
			}

			// Default Play time setup (2 sec)
			audio.play_time = AUDIO_PLAY_TIME;
			
			pthread_mutex_lock(&audio.mutex);
			if (!audio.enable)
				audio.enable = true;
			pthread_mutex_unlock(&audio.mutex);
			usleep(100);
			sprintf(resp_msg, "%d,%d", audio.enable, audio.play_time);
			return 0;
		}
		info ("null pointer msg\n");
		return -1;
	}
	// device busy now
	return	1;
}

//------------------------------------------------------------------------------
int audio_test_init (void)
{
	memset (&audio, 0x00, sizeof(audio));
	pthread_create (&audio.pthread, NULL, audio_test_thread, &audio);
}
//------------------------------------------------------------------------------
void audio_test_exit (void)
{
	info ("rx thread detach = %d\n", pthread_detach(audio.pthread));
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
#define	TEST_MSG_COUNT	3
__s8 test_msg[3][20] = {
	"l_ch",
	"r_ch",
	"unknown",
};

//------------------------------------------------------------------------------
int main(int argc, char **argv)
{
	int count = 0, ret = 0;
	__s8 resp_msg[20];

	memset (resp_msg, 0x00, sizeof(resp_msg));
	audio_test_init();

	while (true) {
		if (!(ret = audio_test_func(test_msg[count % TEST_MSG_COUNT], resp_msg))) {
			info ("count = %d, msg_len = %02d, resp msg = %s\n", count, (int)strlen(resp_msg), resp_msg);
			memset (resp_msg, 0x00, sizeof(resp_msg));
		} else {
			if (ret > 0)
				info ("audio device busy! %d\n", count);
			else
				info ("audio device error! %d\n", count);
		}
		if (!(ret > 0)) {
			count++;
			if (count > (TEST_MSG_COUNT - 1))
				count = 0;
		}
		sleep(2);
	}

	audio_test_exit ();
	return 0;
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
