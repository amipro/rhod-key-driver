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
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "utils.h"

int power_on=1;
int backlight_fd=-1;
int power_fd=-1;
char *suspend_cmd=NULL;
char *resume_cmd=NULL;
char bl_save[10]="144";

int setup_power(void){
	power_fd=open("/sys/power/state",O_WRONLY);
	if(power_fd<0){
		goto error;
	}
	backlight_fd=open("/sys/class/leds/lcd-backlight/brightness",O_RDWR);
	return 0;
error:
	error("Initialization error of power: %s\n", strerror(errno));
	return -1;
}

void clear_power(void){
	if(power_fd>=0)
		close(power_fd);
	if(backlight_fd>=0)
		close(backlight_fd);
	if(suspend_cmd)
		free(suspend_cmd);
	if(resume_cmd)
		free(resume_cmd);
}

void flip_power(void){
	if(power_on){
		debug("power off\n");
		if(suspend_cmd){
			debug("Execute external command '%s'\n", suspend_cmd);
			if(system(suspend_cmd)==-1)
				error("Execute of command `%s`\n",suspend_cmd);
		}

		if(backlight_fd>=0){
			// Save old value
			lseek(backlight_fd, 0, SEEK_SET);
			int ret;
			ret=read(backlight_fd, bl_save, 9);
			if(ret>0){
				bl_save[ret]=0;
			}else{
				error("flip_power: failed to save old bl value\n");
			}
			// Switch off backlight
			write(backlight_fd,"0",1);
		}
		write(power_fd,"mem",3);
		power_on=0;
	}
	else{
		debug("power on\n");
		write(power_fd,"on",2);
		if(resume_cmd){
			debug("Execute external command '%s'\n", resume_cmd);
			if(system(resume_cmd)==-1)
				error("Execute of command `%s`\n",resume_cmd);
		}
		// Restore old bl value
		if(backlight_fd>=0){
			debug("restore olf bl value %s\n", bl_save);
			write(backlight_fd,bl_save,3);
		}
		power_on=1;
	}
}

int conf_key_exec_resume(char *key_code, char *cmd, char *arg){
	int ret=0;

	if(*arg==0){
		conf_error("Argument is missing %s\n", cmd);
		return -1;
	}

	resume_cmd=malloc(strlen(arg)+1);
	if(resume_cmd){
		strcpy(resume_cmd, arg);
	}else{
		conf_error("malloc failed\n");
		return -1;
	}
	
	return ret;
}

int conf_key_exec_suspend(char *key_code, char *cmd, char *arg){
	int ret=0;

	if(*arg==0){
		conf_error("Argument is missing %s\n", cmd);
		return -1;
	}

	suspend_cmd=malloc(strlen(arg)+1);
	if(suspend_cmd){
		strcpy(suspend_cmd, arg);
	}else{
		conf_error("malloc failed\n");
		return -1;
	}
	
	return ret;
}
