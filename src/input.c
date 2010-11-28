/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
	 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor Boston, MA 02110-1301,  USA
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/input.h>
#include <linux/uinput.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>

#include "utils.h"

#define UINPUT_DEV_PATH1 "/dev/uinput"
#define UINPUT_DEV_PATH2 "/dev/input/uinput"

static int uinput_fd=-1;

int evdev_count=0;
int evdev_fd_list[10];
void clear_evdev();

// Search for keyboards in /dev/input/
int setup_evdev(){
	int err=0;
	DIR *dp;
	struct dirent *ep;
	char evtype_b[EV_MAX/8];
	int fd;
	char buf[21];

	// loop for input devices in /dev/input
	dp = opendir ("/dev/input");
	if (dp != NULL){
		while ((ep=readdir(dp))){
			debug("Looping for input device %s\n",ep->d_name);
			if(!strncmp("event",ep->d_name,5)){
				// test device has keys but not a pointer
				sprintf(buf,"/dev/input/%s",ep->d_name);
				fd=open(buf,O_RDONLY|O_NDELAY);
				if(fd<0){
					error("Open error of evdev[%s]: %s\n",ep->d_name, strerror(errno));
					continue;
				}
				memset(evtype_b, 0, sizeof(evtype_b));
				if (ioctl(fd, EVIOCGBIT(0, EV_MAX), evtype_b) < 0) {
					error("evdev ioctl error of evdev[%s]: %s\n",ep->d_name, strerror(errno));
					close(fd);
					continue;
				}
				if (!TEST_BIT(EV_KEY, evtype_b) || TEST_BIT(EV_REL, evtype_b) || TEST_BIT(EV_ABS, evtype_b)){
					debug("Input device %s was ignored\n",ep->d_name);
					close(fd);
					continue;
				}
				evdev_fd_list[evdev_count++]=fd;
				debug("Input device %s was added\n",ep->d_name);
				if (ioctl(fd, EVIOCGRAB, 2) < 0) {
					error("evdev ioctl GRAB error of evdev[%s]: %s\n",ep->d_name, strerror(errno));
				}
			}
		}
		closedir (dp);
	}
	else{
		error ("Couldn't open the directory\n");
		err=-1;
	}

	if(!evdev_count){
		error ("Can't find valid input device\n");
		err=-1;
	}

	// clearup incase of error
	if(err){
		clear_evdev();
	}

	return err;
}

void clear_evdev(){
	for(;evdev_count>=0;--evdev_count)
		close(evdev_fd_list[evdev_count]);
}

int send_key_uinput(int code, int value, int type){
	struct input_event event;

	debug("send_key_uinput: code=%d, value=%d, type=%d\n", code, value, type);
	gettimeofday(&event.time, NULL);
	event.type = type;
	event.code = code;
	event.value = value;

	if(write(uinput_fd, &event, sizeof(event))<0){
		error("Error send key: %s\n", strerror(errno));
		return -1;
	}
	event.type = EV_SYN;
	event.code = 0;
	event.value = 0;

	if(write(uinput_fd, &event, sizeof(event))<0){
		error("Error send SYN\n");
		return -1;
	}

	return 0;
}

int setup_uinput(void){
	struct uinput_user_dev uinp;
	uinput_fd=open(UINPUT_DEV_PATH1, O_WRONLY|O_NDELAY);
	if(uinput_fd<0){
		uinput_fd=open(UINPUT_DEV_PATH2, O_WRONLY|O_NDELAY);
		if(uinput_fd<0){
			goto error;
		}
	}

	// create the device
	memset(&uinp,0,sizeof(uinp)); // Intialize the uInput device to NULL
	strncpy(uinp.name, "RhodKeyDriver", UINPUT_MAX_NAME_SIZE);
	uinp.id.version = 4;
	uinp.id.bustype = BUS_USB;

	write(uinput_fd, &uinp, sizeof(uinp));
	if (ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY))
	{
		goto error;
	}
	if (ioctl(uinput_fd, UI_SET_EVBIT, EV_SYN))
	{
		goto error;
	}
	if (ioctl(uinput_fd, UI_SET_EVBIT, EV_SW))
	{
		goto error;
	}

	int i;
	for(i = 1; i<0x100; i++)
		ioctl(uinput_fd, UI_SET_KEYBIT, i);
	for(i = 0; i<SW_CNT; i++)
		ioctl(uinput_fd, UI_SET_SWBIT, i);

	if (ioctl(uinput_fd, UI_DEV_CREATE))
	{
		goto error;
	}

	return 0;
	error:
		error("Initialization error of uinput: %s\n", strerror(errno));
		return -1;
}

void clear_uinput(void){
	if(uinput_fd>=0){
		ioctl(uinput_fd, UI_DEV_DESTROY);
		close(uinput_fd);
	}
}

int setup_input(){
	int ret;

	ret=setup_evdev();
	if(!ret){
		ret=setup_uinput();
		if(ret)
			clear_evdev();
	}

	return ret;
}

void clear_input(){
	clear_evdev();
	clear_uinput();
}

void input_loop(void (processor)(int type, int code, int value)){
	int i;
	int maxfd=0;
	struct input_event e;
	fd_set read_fds;
	struct timeval tval;
	time_t t;

	t=-1;

	while(1){
		FD_ZERO(&read_fds);

		for(i=0;i<evdev_count;i++){
			FD_SET(evdev_fd_list[i], &read_fds);
			maxfd = MAX(evdev_fd_list[i], maxfd);
		}

		tval.tv_sec=3;
		tval.tv_usec=0;
		if(select(maxfd+1,&read_fds,NULL,NULL,&tval)==-1){
			error("input_loop: select faild: %s\n", strerror(errno));
			break;
		}

		for(i=0;i<evdev_count;i++){
			if(FD_ISSET(evdev_fd_list[i], &read_fds)){
				if(read(evdev_fd_list[i], &e, sizeof(e))==sizeof(e)){
					debug("Input event received type=%d code=%d value=%d\n",e.type,e.code,e.value);
					time(&t);		// time of last action
					processor(e.type,e.code,e.value);
				}
			}
		}
		if(t!=-1 && time(NULL)-t>=3){
			reset_keys();
			t=-1;
		}
	}
}
