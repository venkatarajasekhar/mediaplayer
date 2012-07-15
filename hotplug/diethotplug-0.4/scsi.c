/*
 * scsi.c
 *
 * The scsi functions
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
#include <stdio.h>	/* for getenv() */
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "hotplug.h"
#include "System2AP.h"

#ifndef RESCUE_LINUX
// devpath = "/devices/SATA_DEV/host0/target0:0:0/0:0:0:0"
// we just need to check SATA_DEV's plug-in and plug-put
// USB's plug-in and plug-out are done in usb.c
static int get_sata_host_port(char *host, char* port, int wait_time)
{
	char port_struct[32] = {0};
	char path[256];
	int fd;
	char *devpath;
	char *ptr, *ptr1;
	int i = 0;

	//devpath = /devices/SATA_DEV/host0/target0:0:0/0:0:0:0
	devpath = getenv ("DEVPATH");

	// ptr = "SATA_DEV/host0/target0:0:0/0:0:0:0"
	ptr = strstr(devpath, "SATA_DEV");
	if(ptr == NULL)
		return 1;

	// ptr1 = "host0/target0:0:0/0:0:0:0"
	ptr1 = strstr(ptr, "host");
	if(ptr1 == NULL) // it may be a usb device
		return 1;

	while(ptr1[i] != '/')
	{
		host[i] = ptr1[i];
		i++;
	}

	// host = "host0"
	host[i]='\0';

	if(port == NULL)
		return 0;	

	// path = "/sys//devices/SATA_DEV/host0/target0:0:0/0:0:0:0/port_structure"
	sprintf(path, "/sys/%s/port_structure", devpath);

	// read the port structure
	i = 0;
	while(((fd = open(path, O_RDONLY)) < 0) && (i < wait_time))
	{
		i++;
		printf("[scsi] Error in open %s port_structure... wait %d sec\n", host, i);
		sleep(1);
	}

	if(fd < 0)
	{
		printf("[scsi] Error in open %s port_structure...\n", host);
		return 1;
	}
	
	if (read(fd, port_struct, sizeof(port_struct)) < 0) {
		printf("[scsi] Error in read %s port_structure...\n", host);
		close(fd);
		return 1;
	}
	close(fd);

	memcpy(port, port_struct, sizeof(port_struct));
	port[strlen(port) - 1] = '\0';

	return 0;
}
#endif

static int scsi_add (void)
{
#ifndef RESCUE_LINUX
	char host[8], port[8];
	SYS2AP_MESSAGE sysmsg;

	if(get_sata_host_port(host, port, 3) == 0)
	{
		printf("Hotplug got one SATA Hotplug of \"Add\" from \"%s\" port \"%s\".\n", host, port);
		sysmsg.m_type = SYS2AP_MSG_SATAPLUG_STATUS;
		sprintf(sysmsg.m_msg, "%s %s %s", host, SYS2AP_MSG_UP, port);
		RTK_SYS2AP_SendMsg(&sysmsg);
	}
#endif

#ifdef WAIT_MODULE_DIR
	wait_dir_ready("/lib/modules/2.6.12.6-VENUS/kernel", 100);
	usleep(300000);
#endif
	load_module("sd_mod");
	return 0;
}


static int scsi_remove (void)
{
#ifndef RESCUE_LINUX
	char host[8];
	SYS2AP_MESSAGE sysmsg;

	if(get_sata_host_port(host, NULL, 0) == 0)
	{
		printf("Hotplug got one SATA Hotplug of \"Remove\" from \"%s\".\n", host);
		sysmsg.m_type = SYS2AP_MSG_SATAPLUG_STATUS;
		sprintf(sysmsg.m_msg, "%s %s", host, SYS2AP_MSG_DOWN);
		RTK_SYS2AP_SendMsg(&sysmsg);
	}
#endif

	return 0;
}


static struct subsystem scsi_subsystem[] = {
	{ ADD_STRING, scsi_add },
	{ REMOVE_STRING, scsi_remove },
	{ NULL, NULL }
};


int scsi_handler (void)
{
	char * action;
	
	action = getenv ("ACTION");
	dbg ("action = %s", action);
	if (action == NULL) {
		dbg ("missing ACTION environment variable, aborting.");
		return 1;
	}

	return call_subsystem (action, scsi_subsystem);
}


