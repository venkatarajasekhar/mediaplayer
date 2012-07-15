/*
 * firmware.c
 *
 * The firmware functions
 *
 * Copyright (C) 2001 Greg Kroah-Hartman <greg@kroah.com>
 *
 *	This program is free software; you can redistribute it and/or modify it
 *	under the terms of the GNU General Public License as published by the
 *	Free Software Foundation version 2 of the License.
 * 
 *	This program is distributed in the hope that it will be useful, but
 *	WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *	General Public License for more details.
 * 
 *	You should have received a copy of the GNU General Public License along
 *	with this program; if not, write to the Free Software Foundation, Inc.,
 *	675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stddef.h>	/* for NULL */
#include <stdlib.h>	/* for getenv() */
#include <errno.h>
#include "hotplug.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

#define SYSFS "/sys"
#define FIRMWARE_DIR "/var/lib/hotplug/firmware"

static int firmware_add (void)
{
	char * devpath, *firmware;
	struct stat sysfile_stat;
	char filename[128];
	char buffer[2048];
	int false=0;
	unsigned int count;
	FILE *F_sysfile1, *F_sysfile2, *F_firmwarefile;
	
	devpath = getenv ("DEVPATH");
	firmware = getenv ("FIRMWARE");

	sprintf(filename, "%s/%s/loading", SYSFS, devpath);
	if(stat(filename, &sysfile_stat))
		sleep(1);
	F_sysfile1 = fopen(filename, "r+t");
	sprintf(filename, "%s/%s/data", SYSFS, devpath);
	F_sysfile2 = fopen(filename, "r+b");
	sprintf(filename, "%s/%s", FIRMWARE_DIR, firmware);
	F_firmwarefile = fopen(filename, "rb");

	if(F_sysfile1 && F_sysfile2 && F_firmwarefile) {
		fputc('1', F_sysfile1);
		fflush(F_sysfile1);
		while((count = fread(buffer, 1, 1024, F_firmwarefile)))
			if(count != fwrite(buffer, 1, count, F_sysfile2)) {
				false = 1;
			}
		if(false)
			fputs("-1", F_sysfile1);
		else
			fputc('0', F_sysfile1);
// We need to flush F_sysfile2 first because I found a very strange situation.
// fwrite will call firmware_data_write in drivers/base/firmware_class.c.
// If you flush F_sysfile1 first, firmware_loading_store will be called before the last firmware_data_write.
// Is the last firmware_data_write cached in someplace? I donot know.
		fflush(F_sysfile2);
		fflush(F_sysfile1);
	} else if(F_sysfile1 && !F_firmwarefile) {
		false = 1;
		fputs("-1", F_sysfile1);
		fflush(F_sysfile1);
	} else
		false = 1;

	if(F_sysfile1)
		fclose(F_sysfile1);
	if(F_sysfile2)
		fclose(F_sysfile2);
	if(F_firmwarefile)
		fclose(F_firmwarefile);

	if(false)
		return 1;
	else
		return 0;
}


static int firmware_remove (void)
{
	/* right now we don't do anything here :) */
	return 0;
}


static struct subsystem firmware_subsystem[] = {
	{ ADD_STRING, firmware_add },
	{ REMOVE_STRING, firmware_remove },
	{ NULL, NULL }
};


int firmware_handler (void)
{
	char * action;
	
	action = getenv ("ACTION");
	dbg ("action = %s", action);
	if (action == NULL) {
		dbg ("missing ACTION environment variable, aborting.");
		return 1;
	}

	return call_subsystem (action, firmware_subsystem);
}


