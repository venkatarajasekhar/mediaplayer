/*
 * usb.c
 *
 * The usb functions
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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "hotplug.h"
#include "System2AP.h"

#include "usb_modules.h"

/* bitmap values taken from include/linux/usb.h */
#define USB_DEVICE_ID_MATCH_VENDOR		0x0001
#define USB_DEVICE_ID_MATCH_PRODUCT		0x0002
#define USB_DEVICE_ID_MATCH_DEV_LO		0x0004
#define USB_DEVICE_ID_MATCH_DEV_HI		0x0008
#define USB_DEVICE_ID_MATCH_DEV_CLASS		0x0010
#define USB_DEVICE_ID_MATCH_DEV_SUBCLASS	0x0020
#define USB_DEVICE_ID_MATCH_DEV_PROTOCOL	0x0040
#define USB_DEVICE_ID_MATCH_INT_CLASS		0x0080
#define USB_DEVICE_ID_MATCH_INT_SUBCLASS	0x0100
#define USB_DEVICE_ID_MATCH_INT_PROTOCOL	0x0200
#define USB_DEVICE_ID_MATCH_DEVICE		(USB_DEVICE_ID_MATCH_VENDOR | USB_DEVICE_ID_MATCH_PRODUCT)
#define USB_DEVICE_ID_MATCH_DEV_RANGE		(USB_DEVICE_ID_MATCH_DEV_LO | USB_DEVICE_ID_MATCH_DEV_HI)
#define USB_DEVICE_ID_MATCH_DEVICE_AND_VERSION	(USB_DEVICE_ID_MATCH_DEVICE | USB_DEVICE_ID_MATCH_DEV_RANGE)
#define USB_DEVICE_ID_MATCH_DEV_INFO		(USB_DEVICE_ID_MATCH_DEV_CLASS | USB_DEVICE_ID_MATCH_DEV_SUBCLASS | USB_DEVICE_ID_MATCH_DEV_PROTOCOL)
#define USB_DEVICE_ID_MATCH_INT_INFO		(USB_DEVICE_ID_MATCH_INT_CLASS | USB_DEVICE_ID_MATCH_INT_SUBCLASS | USB_DEVICE_ID_MATCH_INT_PROTOCOL)
	
#define DVDPLAYER_LOCK "/var/lock/.DvdPlayer"
#define HOTPLUG_ADD_SIGNAL_NUMBER 46
#define HOTPLUG_REMOVE_SIGNAL_NUMBER 47

extern int block_ptp;

/* If module_action = 1, load module; if module_action = 2, unload module. */
static int match_usb_modules (unsigned short vendor, unsigned short product, unsigned short bcdDevice,
	unsigned char deviceclass, unsigned char devicesubclass, unsigned char deviceprotocol,
	unsigned char interfaceclass, unsigned char interfacesubclass, unsigned char interfaceprotocol, int module_action)
{
	int i;
	int retval=0;

	dbg ("vendor = %x, product = %x, bcdDevice = %x", vendor, product, bcdDevice);
	dbg ("deviceclass = %x, devicesubclass = %x, deviceprotocol = %x", deviceclass, devicesubclass, deviceprotocol);
	dbg ("interfaceclass = %x, interfacesubclass = %x, interfaceprotocol = %x", interfaceclass, interfacesubclass, interfaceprotocol);

	for (i = 0; usb_module_map[i].module_name != NULL; ++i) {
		dbg ("looking at %s, match_flags = %x", usb_module_map[i].module_name, usb_module_map[i].match_flags);
		if (usb_module_map[i].match_flags & (USB_DEVICE_ID_MATCH_DEVICE | USB_DEVICE_ID_MATCH_DEV_RANGE)) {
			if ((usb_module_map[i].match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
			    (usb_module_map[i].idVendor != vendor)) {
				dbg ("vendor check failed %x != %x", usb_module_map[i].idVendor, vendor);
				continue;
			}
			if ((usb_module_map[i].match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
			    (usb_module_map[i].idProduct != product)) {
				dbg ("product check failed %x != %x", usb_module_map[i].idProduct, product);
				continue;
			}
			if ((usb_module_map[i].match_flags & USB_DEVICE_ID_MATCH_DEV_LO) &&
			    (usb_module_map[i].bcdDevice_lo > bcdDevice)) {
				dbg ("bcdDevice_lo check failed %x > %x", usb_module_map[i].bcdDevice_lo, bcdDevice);
				continue;
			}
			if ((usb_module_map[i].match_flags & USB_DEVICE_ID_MATCH_DEV_HI) &&
			    (usb_module_map[i].bcdDevice_hi < bcdDevice)) {
				dbg ("bcdDevice_hi check failed %x < %x", usb_module_map[i].bcdDevice_hi, bcdDevice);
				continue;
			}
		}

		if (usb_module_map[i].match_flags & USB_DEVICE_ID_MATCH_DEV_INFO) {
			if ((usb_module_map[i].match_flags & USB_DEVICE_ID_MATCH_DEV_CLASS) &&
			    (usb_module_map[i].bDeviceClass != deviceclass)) {
				dbg ("class check failed %x != %x", usb_module_map[i].bDeviceClass, deviceclass);
				continue;
			}
			if ((usb_module_map[i].match_flags & USB_DEVICE_ID_MATCH_DEV_SUBCLASS) &&
			    (usb_module_map[i].bDeviceSubClass != devicesubclass)) {
				dbg ("subclass check failed %x != %x", usb_module_map[i].bDeviceSubClass, devicesubclass);
				continue;
			}
			if ((usb_module_map[i].match_flags & USB_DEVICE_ID_MATCH_DEV_PROTOCOL) &&
			    (usb_module_map[i].bDeviceProtocol != deviceprotocol)) {
				dbg ("protocol check failed %x != %x", usb_module_map[i].bDeviceProtocol, deviceprotocol);
				continue;
			}
		}

		if (usb_module_map[i].match_flags & USB_DEVICE_ID_MATCH_INT_INFO) {
			if ((usb_module_map[i].match_flags & USB_DEVICE_ID_MATCH_INT_CLASS) &&
			    (usb_module_map[i].bInterfaceClass != interfaceclass)) {
				dbg ("class check failed %x != %x", usb_module_map[i].bInterfaceClass, interfaceclass);
				continue;
			}
			if ((usb_module_map[i].match_flags & USB_DEVICE_ID_MATCH_INT_SUBCLASS) &&
			    (usb_module_map[i].bInterfaceSubClass != interfacesubclass)) {
				dbg ("subclass check failed %x != %x", usb_module_map[i].bInterfaceSubClass, interfacesubclass);
				continue;
			}
			if ((usb_module_map[i].match_flags & USB_DEVICE_ID_MATCH_INT_PROTOCOL) &&
			    (usb_module_map[i].bInterfaceProtocol != interfaceprotocol)) {
				dbg ("protocol check failed %x != %x", usb_module_map[i].bInterfaceProtocol, interfaceprotocol);
				continue;
			}
		}

		/* found one! */
		if(module_action == 1) {
			if(parse_config(HOTPLUG_CONFIG_FILE, "nomodule", "*", 0) == 1) {
				dbg ("Skip loading usb module.");
			} else {
				dbg ("loading %s", usb_module_map[i].module_name);
				retval = load_module (usb_module_map[i].module_name);
			}
		} else if(module_action == 2) {
			dbg ("unloading %s", usb_module_map[i].module_name);
			retval = unload_module (usb_module_map[i].module_name);
		}
		if (retval)
			return retval;
	}

	return -ENODEV;
}

#ifndef RESCUE_LINUX
// devpath = "/devices/platform/ehci_hcd/usb1/1-2/1-2.1"
// or        "/devices/platform/ehci_hcd/usb1/1-2/1-2.1/1-2.1:1.0"
// we just want to parse cases like devpath = "/devices/platform/ehci_hcd/usb1/1-2/1-2.1"
// if we parse two cases, it will send two message for a signal device
static int get_usb_locate(char *locate)
{
	char *devpath, *ptr, *ptr1;

	devpath = getenv ("DEVPATH");

	// ptr = "usb1/1-2/1-2.1"
	ptr = strstr(devpath, "usb");
	if(ptr == NULL) // it may be a SATA device
		return 1;
	
	// ptr1 should be NULL
	ptr1 = strstr(ptr, ":");
	if(ptr1) // we don't parse case two
		return 1;

	// prt = "1-2.1"
	while((ptr1 = strstr(ptr, "/")))
		ptr = ptr1 + 1;

	if(strstr(ptr, "usb"))
	{
		// locate = "usb1"
		//sprintf(locate, "%s", ptr);
		return 1;
	}
	else
	{
		 // locate = "2.1"
		sprintf(locate, "%s", ptr + 2);
	}
	
	return 0;
}
#endif

static int usb_add (void)
{
	char *product_env;
	char *type_env;
	char *interface_env;
	char ptr[128];
	SYS2AP_MESSAGE sysmsg;
	int error=0;
	unsigned int idVendor;
	unsigned int idProduct;
	unsigned int bcdDevice;
	unsigned int device_class;
	unsigned int device_subclass;
	unsigned int device_protocol;
	unsigned int interface_class = 0;
	unsigned int interface_subclass = 0;
	unsigned int interface_protocol = 0;
	
	signal_pid(DVDPLAYER_LOCK, HOTPLUG_ADD_SIGNAL_NUMBER);

#ifndef RESCUE_LINUX
	if(get_usb_locate(ptr) == 0)
	{
		printf("Hotplug got one USB Hotplug of \"Add\" from port \"%s\".\n", ptr);
		sysmsg.m_type = SYS2AP_MSG_USBPLUG_STATUS;
		sprintf(sysmsg.m_msg, "%s %s", ptr, SYS2AP_MSG_UP);
		RTK_SYS2AP_SendMsg(&sysmsg);
	}
#endif

	product_env = getenv ("PRODUCT");
	type_env = getenv ("TYPE");
	interface_env = getenv ("INTERFACE");
	if (!product_env) {
		dbg ("missing \"PRODUCT\" environment variable, aborting.");
		return 1;
	}
	error = split_3values (product_env, 16, &idVendor, &idProduct, &bcdDevice);
	if (error)
		return error;

	if(type_env)
		error = split_3values (type_env, 10, &device_class, &device_subclass, &device_protocol);
	else {
		device_class = 1000;
		device_subclass = 1000;
		device_protocol = 1000;
	}
	if (error)
		return error;

	if (interface_env)
		error = split_3values (interface_env, 10, &interface_class, &interface_subclass, &interface_protocol);
	else {
		interface_class = 1000;
		interface_subclass = 1000;
		interface_protocol = 1000;
	}
	if (error)
		return error;
#ifdef WAIT_MODULE_DIR
	wait_dir_ready("/lib/modules/2.6.12.6-VENUS/kernel", 100);
	usleep(300000);
#endif

	error = match_usb_modules ((unsigned short)idVendor, (unsigned short)idProduct, (unsigned short)bcdDevice,
		(unsigned char)device_class, (unsigned char)device_subclass, (unsigned char)device_protocol,
		(unsigned char)interface_class, (unsigned char)interface_subclass, (unsigned char)interface_protocol, 1);

	if(interface_class==6 && interface_subclass==1 && interface_protocol==1) {
		block_ptp = 1;
		block_handler();
	}
	return error;
}


static int usb_remove (void)
{
	char *product_env;
	char *type_env;
	char *interface_env;
	char ptr[128];
	SYS2AP_MESSAGE sysmsg;
	int error=0;
	unsigned int idVendor;
	unsigned int idProduct;
	unsigned int bcdDevice;
	unsigned int device_class;
	unsigned int device_subclass;
	unsigned int device_protocol;
	unsigned int interface_class = 0;
	unsigned int interface_subclass = 0;
	unsigned int interface_protocol = 0;

	signal_pid(DVDPLAYER_LOCK, HOTPLUG_REMOVE_SIGNAL_NUMBER);

#ifndef RESCUE_LINUX
	if(get_usb_locate(ptr) == 0)
	{
		printf("Hotplug got one USB Hotplug of \"Remove\" from port \"%s\".\n", ptr);
		sysmsg.m_type = SYS2AP_MSG_USBPLUG_STATUS;
		sprintf(sysmsg.m_msg, "%s %s", ptr, SYS2AP_MSG_DOWN);
		RTK_SYS2AP_SendMsg(&sysmsg);
	}
#endif

	product_env = getenv ("PRODUCT");
	type_env = getenv ("TYPE");
	interface_env = getenv ("INTERFACE");
	if (!product_env) {
		dbg ("missing \"PRODUCT\" environment variable, aborting.");
		return 1;
	}
	error = split_3values (product_env, 16, &idVendor, &idProduct, &bcdDevice);
	if (error)
		return error;

	if(type_env)
		error = split_3values (type_env, 10, &device_class, &device_subclass, &device_protocol);
	else {
		device_class = 1000;
		device_subclass = 1000;
		device_protocol = 1000;
	}
	if (error)
		return error;

	if (interface_env)
		error = split_3values (interface_env, 10, &interface_class, &interface_subclass, &interface_protocol);
	else {
		interface_class = 1000;
		interface_subclass = 1000;
		interface_protocol = 1000;
	}
	if (error)
		return error;
/* Bug: http://cadinfo1.realtek.com.tw:8080/~sub1/mantis/view.php?id=16005
   Temporarily no remove module to avoid Wifi bug.
 */
/*
#ifdef WAIT_MODULE_DIR
	wait_dir_ready("/lib/modules/2.6.12.6-VENUS/kernel", 100);
	usleep(300000);
#endif

	error = match_usb_modules ((unsigned short)idVendor, (unsigned short)idProduct, (unsigned short)bcdDevice,
		(unsigned char)device_class, (unsigned char)device_subclass, (unsigned char)device_protocol,
		(unsigned char)interface_class, (unsigned char)interface_subclass, (unsigned char)interface_protocol, 2);
*/

	if(interface_class==6 && interface_subclass==1 && interface_protocol==1) {
		block_ptp = 1;
		block_handler();
	}
	return 0;
}


static int usb_audio_in_up (void) {
	char ptr[128];
	SYS2AP_MESSAGE sysmsg;

#ifndef RESCUE_LINUX
	if(get_usb_locate(ptr) == 0)
	{
	printf("Hotplug got one USB Hotplug of \"Audio In UP\" from port \"%s\".\n", ptr);
	//sysmsg.m_type = SYS2AP_MSG_USBPLUG_STATUS;
	//sprintf(sysmsg.m_msg, "%s %s", ptr, SYS2AP_MSG_UP);
	//RTK_SYS2AP_SendMsg(&sysmsg);
	}
#endif
	return 0;
}


static int usb_audio_in_down (void) {
	char ptr[128];
	SYS2AP_MESSAGE sysmsg;

#ifndef RESCUE_LINUX
	if(get_usb_locate(ptr) == 0)
	{
	printf("Hotplug got one USB Hotplug of \"Audio In DOWN\" from port \"%s\".\n", ptr);
	//sysmsg.m_type = SYS2AP_MSG_USBPLUG_STATUS;
	//sprintf(sysmsg.m_msg, "%s %s", ptr, SYS2AP_MSG_DOWN);
	//RTK_SYS2AP_SendMsg(&sysmsg);
	}
#endif
	return 0;
}


static int usb_audio_out_up (void) {
	char ptr[128];
	SYS2AP_MESSAGE sysmsg;

#ifndef RESCUE_LINUX
	if(get_usb_locate(ptr) == 0)
	{
	printf("Hotplug got one USB Hotplug of \"Audio Out UP\" from port \"%s\".\n", ptr);
	//sysmsg.m_type = SYS2AP_MSG_USBPLUG_STATUS;
	//sprintf(sysmsg.m_msg, "%s %s", ptr, SYS2AP_MSG_UP);
	//RTK_SYS2AP_SendMsg(&sysmsg);
	}
#endif
	return 0;
}


static int usb_audio_out_down (void) {
	char ptr[128];
	SYS2AP_MESSAGE sysmsg;

#ifndef RESCUE_LINUX
	if(get_usb_locate(ptr) == 0)
	{
	printf("Hotplug got one USB Hotplug of \"Audio Out DOWN\" from port \"%s\".\n", ptr);
	//sysmsg.m_type = SYS2AP_MSG_USBPLUG_STATUS;
	//sprintf(sysmsg.m_msg, "%s %s", ptr, SYS2AP_MSG_DOWN);
	//RTK_SYS2AP_SendMsg(&sysmsg);
	}
#endif
	return 0;
}


static struct subsystem usb_subsystem[] = {
	{ ADD_STRING, usb_add },
	{ REMOVE_STRING, usb_remove },
	{ USB_AI_UP, usb_audio_in_up },
	{ USB_AI_DOWN, usb_audio_in_down },
	{ USB_AO_UP, usb_audio_out_up },
	{ USB_AO_DOWN, usb_audio_out_down },
	{ NULL, NULL }
};


int usb_handler (void)
{
	char * action;
	
	action = getenv ("ACTION");
	dbg ("action = %s", action);
	if (action == NULL) {
		dbg ("missing ACTION environment variable, aborting.");
		return 1;
	}

	return call_subsystem (action, usb_subsystem);
}


