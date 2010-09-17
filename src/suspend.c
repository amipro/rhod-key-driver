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

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include "utils.h"

int power_on=1;
int backlight_fd=-1;
int power_fd=-1;
int setup_power(void){
	power_fd=open("/sys/power/state",O_WRONLY);
	if(power_fd<0){
		goto error;
	}
	backlight_fd=open("/sys/class/leds/lcd-backlight/brightness",O_WRONLY);
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
}

void flip_power(void){
	if(power_on){
		debug("power off\n");
		write(power_fd,"mem",3);
		power_on=0;
		if(backlight_fd>=0)write(backlight_fd,"0",1);
	}
	else{
		debug("power on\n");
		write(power_fd,"on",2);
		power_on=1;
		if(backlight_fd>=0)write(backlight_fd,"144",3);
	}
}
