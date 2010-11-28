/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * main.c
 * Copyright (C) Ahmed Abdel-Hamid 2010 <ahmedam@mail.usa.com>
 * 
 * rhod-key-driver is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * rhod-key-driver is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <unistd.h>
#include <config.h>
#include "suspend.h"
#include "input.h"
#include "key_processor.h"
#include "utils.h"

int main(int argc,char **argv)
{
	int i;
	int c;
	
	while ((c = getopt (argc, argv, "hv")) != -1)
		switch (c){
			case '?':
			case 'h':
				error(PACKAGE_STRING ": Keyboard Manager for Rhodium\n"
				      "Usage: %s [hv] [c <configuration file>]\n"
				      "h\tDisplay this help\n"
				      "v\tEnable debug\n"
				      "c\tconfiguration file path\n"
				      "\nHome page: http://github.com/amipro/rhod-key-driver\n", argv[0]);
				return 0;
			case 'v':
				set_debug(1);
				break;
       }

	for(i=optind;i<argc;i++){
		debug("Load configuration file %s\n",argv[i]);
		if(load_configuration(argv[i]))
		    goto error;
	}
	
	if(setup_input())
		goto error;
	if(setup_power())
		goto error;
	
	input_loop(process_evdev);
	
error:
	clear_power();
	clear_input();
	
	return 0;
}
