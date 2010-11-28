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

#include <stdio.h>
#include <stdarg.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/vt.h>

#include "utils.h"

int conf_key_set_stickiness(char *key_code, char *cmd, char *arg);
int conf_key_set_code(char *key_code, char *cmd, char *arg);
int conf_key_set_power_key(char *key_code, char *cmd, char *arg);
int conf_key_set_fn_code(char *key_code, char *cmd, char *arg);
int conf_key_set_fn_key(char *key_code, char *cmd, char *arg);
int conf_key_exec_0(char *key_code, char *cmd, char *arg);
int conf_key_exec_1(char *key_code, char *cmd, char *arg);
int conf_key_exec_suspend(char *key_code, char *cmd, char *arg);
int conf_key_exec_resume(char *key_code, char *cmd, char *arg);

int debug_level=0;
void set_debug(int level){
	debug_level=level;
}
void debug(const char *s,...)
{
	if(!debug_level)
		return;
	
    va_list va;
	va_start(va,s);

    vfprintf(stdout,s,va);

    va_end(va);
}

void error(const char *s,...)
{
    va_list va;
	va_start(va,s);

    vfprintf(stderr,s,va);

    va_end(va);
}

void syserror(const char *s,...)
{
    va_list va;
	va_start(va,s);

    vfprintf(stderr,s,va);

    va_end(va);
}

// The signature of configuration callbacks
typedef void (*t_config_callback)(char *label,char *cmd, char *arg);
// List of available options for each key
struct conf_options{
	char *name;					// option name
	t_config_callback callback;
} options[]={
	{"stickiness", (t_config_callback)conf_key_set_stickiness},
	{"code", (t_config_callback)conf_key_set_code},
	{"fncode", (t_config_callback)conf_key_set_fn_code},
	{"fnkey", (t_config_callback)conf_key_set_fn_key },
	{"suspend", (t_config_callback)conf_key_set_power_key},
	{"exec_0", (t_config_callback)conf_key_exec_0},
	{"exec_1", (t_config_callback)conf_key_exec_1},
	{"exec_suspend", (t_config_callback)conf_key_exec_suspend},
	{"exec_resume", (t_config_callback)conf_key_exec_resume}
};

char conf_label[256];	// current key section name
char *conf_file;	// configuration file path
int lineno=0;		// current configuration line number
int load_configuration(char *path){
	char buf[256];
	char *name;
	char *s;
	char *arg;
	int err=0;

	conf_file=path;
	lineno=0;
	
	FILE *f=fopen(path,"r");
	if(f==NULL){
		error("Error opening file %s\n",path);
		return -1;
	}

	while(fgets(buf,256,f)!=NULL){
		// parse a configuration line
		lineno++;
		s=buf;
		name=NULL;
		arg=NULL;
		while(*s && isspace(*s))s++;
		if(*s=='#'  || *s==0)continue;

		name=s;
		while(*s && !isspace(*s) && *s!=':')s++;
		if(*s==':'){
			// we are in a label
			*s=0;
			s++;
			strcpy(conf_label,name);
			name=NULL;
			goto parse_done;
		}
		if(*s){
			*s=0;
			s++;
		}
		while(*s && isspace(*s))s++;

		// get argument
		if(*s!='#'){
			arg=s;
			if(*s=='"'){
				s++;
				arg++;
				while(*s && *s!='"')s++;
			}else{
				while(*s && !isspace(*s))s++;
			}
			if(*s){
				*s=0;
				s++;
			}
		}

parse_done:
		while(*s && (isspace(*s) || *s=='\n'))s++;
		if(*s && *s!='#'){
			conf_error("Unexpected token %s\n",s);
			err=-1;
			goto error;
		}
		if(name){
			debug("execute configuration %s: %s %s\n",conf_label,name,arg);
			int i=0;
			err=-1;
			// find and call the callback for the current option
			for(i=0;i<ARRAY_SIZE(options);i++){
				if(!strcmp(options[i].name,name)){
					options[i].callback(conf_label,name,arg);
					err=0;
					break;
				}
			}
			if(err){
				conf_error("Unknown option '%s'\n",name);
				goto error;
			}	
		}
	}
	
error:
	fclose(f);

	return err;
}

void setconsole(int console)
{
	int fd=open("/dev/tty1",O_RDWR);
	if(fd==-1){
		error("error open /dev/tty\n");
		return;
	}

	//sleep(1);
	if(ioctl(fd,VT_ACTIVATE,console)){
		error("error VT_ACTIVATE\n");
	}
	close(fd);
}
