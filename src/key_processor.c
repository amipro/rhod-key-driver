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
#include <linux/input.h>
#include <string.h>
#include <stdlib.h>

#include "input.h"
#include "utils.h"
#include "suspend.h"

typedef enum {KEY_STATUS_NONE, KEY_STATUS_PENDING, KEY_STATUS_STICK, KEY_STATUS_LOCK} t_key_status;
typedef enum {KEY_TYPE_NONE, KEY_TYPE_STICKY, KEY_TYPE_LOCK, KEY_TYPE_FULL} t_key_type;
struct key_mode{
	int code;
	t_key_type type;
	int code_replace;
	int code_fn;
	t_key_status status;
	int is_pressed;
	int is_fn;
};
struct key_mode sticky_keys[100];
int sticky_keys_count=0;
int power_key_code=-1;
int is_fn_pressed=0;

/*={
	{464,KEY_RIGHTCTRL},  //Ctrl
	{215,109},  // FN
	{127},  // Alt
	{42}	// Shift
};*/

int translate_key_code(char *s){
	if(*s==0)
		return -1;
	return atoi(s);
}

struct key_mode *find_create_key(int code){
	int i;
	for(i=0;i<sticky_keys_count;i++)
		if(sticky_keys[i].code==code)
			return &sticky_keys[i];
	if(i>99)
		return NULL;

	memset(&sticky_keys[i],0,sizeof(sticky_keys[i]));
	sticky_keys_count=i+1;
	sticky_keys[i].code=code;

	return &sticky_keys[i];
}

int conf_key_set_stickiness(char *key_code, char *cmd, char *arg){
	int ret=0;
	t_key_type stickiness=KEY_TYPE_NONE;

	if(*arg==0){
		conf_error("Argument is missing %s\n",cmd);
		return -1;
	}
	if(!strcmp(arg,"none"))
		stickiness=KEY_TYPE_NONE;
	else if(!strcmp(arg,"sticky"))
		stickiness=KEY_TYPE_STICKY;
	else if(!strcmp(arg,"lock"))
		stickiness=KEY_TYPE_LOCK;
	else if(!strcmp(arg,"full"))
		stickiness=KEY_TYPE_FULL;
	else{
		conf_error("Unknown argument %s\n",arg);
		return -1;
	}

	int code=translate_key_code(key_code);
	struct key_mode *key=find_create_key(code);
	if(!key){
		conf_error("Can't add key %s\n",key_code);
		return -1;
	}
	key->type=stickiness;
	
	return ret;
}

int conf_key_set_code(char *key_code, char *cmd, char *arg){
	int ret=0;

	if(*arg==0){
		conf_error("Argument is missing %s\n",cmd);
		return -1;
	}

	int code=translate_key_code(key_code);
	int new_code=translate_key_code(arg);
	struct key_mode *key=find_create_key(code);
	if(!key){
		conf_error("Can't add key %s\n",key_code);
		return -1;
	}
	key->code_replace=new_code;
	
	return ret;
}

int conf_key_set_fn_code(char *key_code, char *cmd, char *arg){
	int ret=0;

	if(*arg==0){
		conf_error("Argument is missing %s\n",cmd);
		return -1;
	}

	int code=translate_key_code(key_code);
	int new_code=translate_key_code(arg);
	struct key_mode *key=find_create_key(code);
	if(!key){
		conf_error("Can't add key %s\n",key_code);
		return -1;
	}
	key->code_fn=new_code;

	return ret;
}

int conf_key_set_fn_key(char *key_code, char *cmd, char *arg){
	int ret=0;

	if(*arg){
		conf_error("Unexpected extra argument %s\n",arg);
		return -1;
	}

	int code=translate_key_code(key_code);
	struct key_mode *key=find_create_key(code);
	if(!key){
		conf_error("Can't add key %s\n",key_code);
		return -1;
	}
	key->code_replace=-1; // don't pass function key code
	key->is_fn=1;

	return ret;
}


int conf_key_set_power_key(char *key_code, char *cmd, char *arg){
	int ret=0;

	if(*arg!=0){
		conf_error("Extra argument\n");
		return -1;
	}
	power_key_code=translate_key_code(key_code);
	
	return ret;
}

int vt_switch=0;

void reset_keys(){
	int i;
	vt_switch=0;
	for(i=0;i<sticky_keys_count;i++){
		int c=sticky_keys[i].code_replace?sticky_keys[i].code_replace:sticky_keys[i].code;
		if(is_fn_pressed && sticky_keys[i].code_fn)
			c=sticky_keys[i].code_fn;
		if(sticky_keys[i].status!=KEY_STATUS_NONE){
			send_key_uinput (c, 0);	
			sticky_keys[i].status=KEY_STATUS_NONE;
		}
	}
	is_fn_pressed=0;
}

void process_evdev(int type, int _code, int value){
	int done=0;
	int i;

	if(value==1 && type==1){
		if(vt_switch==0 && _code==464){ //ctrl
			vt_switch=1;
		}
		else if(vt_switch==1 && _code==127){ //ctrl
			vt_switch=2;
		}
		else if(vt_switch==2 && _code>=2 && _code<=10){ //1-9
			int console=_code-1;
			// reset all keys
			reset_keys();
			setconsole(console);
			debug("switch to console %d\n",console);
			return;
		}
		else{
			vt_switch=0;
		}
	}
	//bypass key release for virtual console switch
	if(vt_switch==2 && value==0 && type==1 && _code>=2 && _code<=10){ //1-9
		vt_switch=0;
		return;
	}
	
	if(_code==power_key_code && value==1){   // power key
		reset_keys();
		flip_power();
	} else {								// stickiness and key mapping
		int keytype=KEY_TYPE_NONE;
		for(i=0;i<sticky_keys_count;i++){
			int c=sticky_keys[i].code_replace?sticky_keys[i].code_replace:_code;
			if(is_fn_pressed && sticky_keys[i].code_fn)
				c=sticky_keys[i].code_fn;

			if(type==1 && _code==sticky_keys[i].code && value==1){
				keytype=sticky_keys[i].type;
				debug("fn=%d c=%d\n",is_fn_pressed,c);
			
				sticky_keys[i].is_pressed=1;
				if(sticky_keys[i].is_fn){
					debug("fn pressed\n");
					is_fn_pressed=1;
				}
				if(keytype==KEY_TYPE_NONE){		// just key mapping
					if(c>0)
						send_key_uinput (c, 1);
				}else if(sticky_keys[i].status==KEY_STATUS_NONE){	// initiate stickyness state machine
					if(c>0)
						send_key_uinput (c, 1);
					sticky_keys[i].status=KEY_STATUS_PENDING;
				}
				done=1;
			}
			else if(type==1 && _code==sticky_keys[i].code && value==0){
				keytype=sticky_keys[i].type;
				sticky_keys[i].is_pressed=0;
				if(keytype==KEY_TYPE_NONE){
					if(c>0)
						send_key_uinput (c, 0);
					if(sticky_keys[i].is_fn)
						is_fn_pressed=0;
				} else {
					if(sticky_keys[i].status==KEY_STATUS_NONE){
						if(c>0)
							send_key_uinput (c, 0); 
						if(sticky_keys[i].is_fn)
							is_fn_pressed=0;
					}
					else if(sticky_keys[i].status==KEY_STATUS_PENDING){
						if(keytype==KEY_TYPE_LOCK){
							sticky_keys[i].status=KEY_STATUS_LOCK;
							debug("%d KEY_STATUS_LOCK\n",sticky_keys[i].code);
						} else {
							sticky_keys[i].status=KEY_STATUS_STICK;
							debug("%d KEY_STATUS_STICK\n",sticky_keys[i].code);
						}
					}
					else if(sticky_keys[i].status==KEY_STATUS_STICK){
						if(keytype==KEY_TYPE_STICKY){
							sticky_keys[i].status=KEY_STATUS_NONE;
							if(c>0)
									send_key_uinput (c, 0);
							if(sticky_keys[i].is_fn)
								is_fn_pressed=0;
							debug("%d KEY_STATUS_NONE\n",sticky_keys[i].code);
						}else{
							sticky_keys[i].status=KEY_STATUS_LOCK;
							debug("%d KEY_STATUS_LOCK\n",sticky_keys[i].code);
						}
					}
					else if(sticky_keys[i].status==KEY_STATUS_LOCK){
						if(c>0)
							send_key_uinput (c, 0);
						if(sticky_keys[i].is_fn)
							is_fn_pressed=0;
						sticky_keys[i].status=KEY_STATUS_NONE;
						debug("%d KEY_STATUS_NONE\n",sticky_keys[i].code);
					}
				}
				done=1;
			}
		}

		if(!done && type==1)
			send_key_uinput (_code, value);
		
		if(keytype==KEY_TYPE_NONE && type==1 && value==0){  // remove stick keys
			for(i=0;i<ARRAY_SIZE(sticky_keys);i++){
				int c=sticky_keys[i].code_replace?sticky_keys[i].code_replace:sticky_keys[i].code;
				if(is_fn_pressed && sticky_keys[i].code_fn)
					c=sticky_keys[i].code_fn;
				if(sticky_keys[i].status==KEY_STATUS_PENDING){
					sticky_keys[i].status=KEY_STATUS_NONE;
					debug("%d KEY_STATUS_NONE\n",sticky_keys[i].code);
				}
				else if(sticky_keys[i].status==KEY_STATUS_STICK && !sticky_keys[i].is_pressed){
					sticky_keys[i].status=KEY_STATUS_NONE;
					if(c>0)
						send_key_uinput (c, 0);
					if(sticky_keys[i].is_fn)
						is_fn_pressed=0;
					debug("%d KEY_STATUS_NONE\n",sticky_keys[i].code);
				}
			}
		}
	}

}
