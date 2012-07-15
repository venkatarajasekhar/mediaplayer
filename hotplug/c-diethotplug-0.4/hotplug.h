/*
 * hotplug.h
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

#ifndef HOTPLUG_H
#define HOTPLUG_H

// If we define this, hotplug will wait "/lib/modules/2.6.12.6-VENUS/kernel" to be ready.
#define WAIT_MODULE_DIR

#define ADD_STRING	"add"
#define REMOVE_STRING	"remove"
#define LINKUP_STRING	"linkup"
#define LINKDOWN_STRING	"linkdown"

struct subsystem {
	const char * name;
	int (* handler) (void);
};

#define DEBUG

#ifdef DEBUG
#include <syslog.h>
	#define dbg(format, arg...) do { log_message (LOG_DEBUG, "__FUNCTION__: " format, ## arg); } while (0)
#else
	#define dbg(format, arg...) do { } while (0)
#endif

extern int log_message (int level, const char *fmt, ...)  __attribute__ ((__format__(__printf__, 2, 3)));


extern int split_3values (const char *string, int base, unsigned int * value1, unsigned int * value2, unsigned int * value3);
extern int split_2values (const char *string, int base, unsigned int * value1, unsigned int * value2);
extern int call_subsystem (const char *string, struct subsystem *subsystem);
extern int load_module (const char *module_name);
extern int signal_pid(const char *file, int signal);
extern int parse_config(const char *file, const char *name, const char *target, char *value);
extern void wait_dir_ready(const char *name, int times);
extern int write_lockdata(const char *filename, const char *data, int len);
extern int read_lockdata(const char *filename, char *data, int *len);
extern int testclear_lockdata(const char *filename, const char *data, int len);

extern int usb_handler (void);
extern int scsi_handler (void);
extern int block_handler (void);
extern int firmware_handler (void);
extern int net_handler (void);
//extern int pci_handler (void);
//extern int ieee1394_handler (void);

#endif

