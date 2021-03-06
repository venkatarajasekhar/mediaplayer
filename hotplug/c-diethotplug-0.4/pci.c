/*
 * pci.c
 *
 * The pci functions
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
#include <string.h>	/* for strtoul() */
#include "hotplug.h"

#include "pci_modules.h"

#define PCI_ANY		0xffffffff

static int match_vendor (unsigned int vendor, unsigned int device,
			 unsigned int subvendor, unsigned int subdevice,
			 unsigned int pci_class)
{
	int i;
	int retval;
	unsigned int class_temp;

	dbg ("vendor = %x, device = %x, subvendor = %x, subdevice = %x",
	     vendor, device, subvendor, subdevice);

	for (i = 0; pci_module_map[i].module_name != NULL; ++i) {
		dbg ("looking at %s", pci_module_map[i].module_name);
		if ((pci_module_map[i].vendor != PCI_ANY) &&
		    (pci_module_map[i].vendor != vendor)) {
			dbg ("vendor check failed %x != %x",
			     pci_module_map[i].vendor, vendor);
			continue;
		}
		if ((pci_module_map[i].device != PCI_ANY) &&
		    (pci_module_map[i].device != device)) {
			dbg ("device check failed %x != %x",
			     pci_module_map[i].device, device);
			continue;
		}
		if ((pci_module_map[i].subvendor != PCI_ANY) &&
		    (pci_module_map[i].subvendor != subvendor)) {
			dbg ("subvendor check failed %x != %x",
			     pci_module_map[i].subvendor, subvendor);
			continue;
		}
		if ((pci_module_map[i].subdevice != PCI_ANY) &&
		    (pci_module_map[i].subdevice != subdevice)) {
			dbg ("subdevice check failed %x != %x",
			     pci_module_map[i].subdevice, subdevice);
			continue;
		}

		/* check that the class matches */
		class_temp = pci_module_map[i].class_mask & pci_class;
		if (pci_module_map[i].class != class_temp) {
			dbg ("class mask check failed %x != %x",
			     pci_module_map[i].class, class_temp);
			continue;
		}

		/* found one! */
		dbg ("loading %s", pci_module_map[i].module_name);
		retval = load_module (pci_module_map[i].module_name);
		if (retval)
			return retval;
	}

	return 0;
}
	
static int pci_add (void)
{
	char *class_env;
	char *id_env;
	char *subsys_env;
	int error;
	unsigned int vendor;
	unsigned int device;
	unsigned int subvendor;
	unsigned int subdevice;
	unsigned int class;
	
	id_env = getenv("PCI_ID");
	subsys_env = getenv("PCI_SUBSYS_ID");
	class_env = getenv("PCI_CLASS");
	if ((id_env == NULL) ||
	    (subsys_env == NULL) ||
	    (class_env == NULL)) {
		dbg ("missing an environment variable, aborting.");
		return 1;
	}
	
	error = split_2values (id_env, 16, &vendor, &device);
	if (error)
		return error;
	error = split_2values (subsys_env, 16, &subvendor, &subdevice);
	if (error)
		return error;
	class = strtoul (class_env, NULL, 16);

#ifdef WAIT_MODULE_DIR
	wait_dir_ready("/lib/modules/2.6.12.6-VENUS/kernel", 100);
	usleep(300000);
#endif
	error = match_vendor (vendor, device, subvendor, subdevice, class);

	return error;
}


static int pci_remove (void)
{
	/* right now we don't do anything here :) */
	return 0;
}

static struct subsystem pci_subsystem[] = {
	{ ADD_STRING, pci_add },
	{ REMOVE_STRING, pci_remove },
	{ NULL, NULL }
};

int pci_handler (void)
{
	char * action;
	
	action = getenv ("ACTION");
	dbg ("action = %s", action);
	if (action == NULL) {
		dbg ("missing ACTION environment variable, aborting.");
		return 1;
	}

	return call_subsystem (action, pci_subsystem);
}


