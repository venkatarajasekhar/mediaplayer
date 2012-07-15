/*
 * hotplug.c
 *
 * A version of /sbin/hotplug that is not a script.
 *
 * Why?  Think initrd :)
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

#include <stdlib.h>
#include "hotplug.h"
#include "hotplug_version.h"
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/times.h>

#define HOTPLUG_TIME 0

int cpu_handler (void)
{
	return 0;
}

//int net_handler (void)
//{
//	return 0;
//}

int dock_handler (void)
{
	return 0;
}

static struct subsystem main_subsystem[] = {
//	{ "pci", pci_handler },
	{ "usb", usb_handler },
	{ "scsi", scsi_handler },
	{ "block", block_handler },
	{ "firmware", firmware_handler },
//	{ "ieee1394", ieee1394_handler },
//	{ "cpu", cpu_handler },
	{ "net", net_handler },
//	{ "dock", dock_handler },
	{ NULL, NULL }
};
	

int main(int argc, char *argv[])
{
	int fd;
#if HOTPLUG_TIME
	/* use to record process run time */
	struct tms T_start, T_end;
	clock_t start, end;
	static long clktck=0;
     if ( clktck==0 )
     	if ( (clktck=sysconf(_SC_CLK_TCK)) < 0 )
     		printf("sysconf error!!");

	start = times(&T_start);
#endif	
	
#if 0
	char *devpath, *action;
	
	devpath = getenv ("DEVPATH");
	action = getenv ("ACTION");
#endif

	close(1);
	fd = open("/dev/console", O_WRONLY);
	if(fd >= 0)
		dup2(fd, 1);

	if (argc != 2) {
		dbg ("unknown number of arguments");
		return 1;
	}
#if 0
	printf("===Hotplug event: TARGET:%s\tACTION:%s\tDEVPATH:%s =====\n", argv[1], action, devpath);
#endif
	fsync(1);
	/* 
	 * We pass off control to the subsystem that is specified by argv[1].
	 */
	setpriority(PRIO_PROCESS, 0, 5);
	call_subsystem (argv[1], main_subsystem);
#if HOTPLUG_TIME	
	end = times(&T_end);
	printf("/sbin/hotplug %s spends %7.2f sec\n", argv[1], (end-start)/(double)clktck );
#endif	
	return 1;
}

