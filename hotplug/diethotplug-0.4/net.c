/*
 * net.c
 *
 * The net functions
 *
 * Copyright (C) 2007 Colin <colin@realtek.com.tw>
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
#include "hotplug.h"
#include "System2AP.h"

#define WLAN_STR "wlan"	// We only handle Realtek's 802.11 wireless cards, and their names would be wlan0, wlan1, ...
#define ETH_STR "eth"	// The prefix name of ethernet

static int net_add (void)
{
	int error=0;
	char *interface_env;
	SYS2AP_MESSAGE sysmsg;
	
	interface_env = getenv ("INTERFACE");
	if (!interface_env) {
		dbg ("missing \"INTERFACE\" environment variable, aborting.");
		return 1;
	}

	if(!strncmp(WLAN_STR, interface_env, strlen(WLAN_STR) && *(interface_env+strlen(WLAN_STR))>='0' && *(interface_env+strlen(WLAN_STR))<='9')) {
		printf("Hotplug got one Net Hotplug of \"Add\" from \"%s\". It may be Realtek's wireless device.\n", interface_env);
		sysmsg.m_type = SYS2AP_MSG_NETPLUG_STATUS;
		sprintf(sysmsg.m_msg, "%s UP", interface_env);
		RTK_SYS2AP_SendMsg(&sysmsg);
	} else {
		printf("Hotplug got one Net Hotplug of \"Add\" from unknow net device \"%s\".\n", interface_env);
	}

	return error;
}


static int net_remove (void)
{
	int error=0;
	char *interface_env;
	SYS2AP_MESSAGE sysmsg;
	
	interface_env = getenv ("INTERFACE");
	if (!interface_env) {
		dbg ("missing \"INTERFACE\" environment variable, aborting.");
		return 1;
	}

	if(!strncmp(WLAN_STR, interface_env, strlen(WLAN_STR) && *(interface_env+strlen(WLAN_STR))>='0' && *(interface_env+strlen(WLAN_STR))<='9')) {
		printf("Hotplug got one Net Hotplug of \"Remove\" from \"%s\". It may be Realtek's wireless device.\n", interface_env);
		sysmsg.m_type = SYS2AP_MSG_NETPLUG_STATUS;
		sprintf(sysmsg.m_msg, "%s DOWN", interface_env);
		RTK_SYS2AP_SendMsg(&sysmsg);
	} else {
		printf("Hotplug got one Net Hotplug of \"Remove\" from unknow net device \"%s\".\n", interface_env);
	}

	return error;
}

static int net_linkup (void)
{
	int error=0;
	char *dev_env;
	SYS2AP_MESSAGE sysmsg;
	
	dev_env = getenv ("INTERFACE");
	if (!dev_env) {
		dbg ("missing \"INTERFACE\" environment variable, aborting.");
		return 1;
	}

	printf("Hotplug got one Net Hotplug of \"LinkUP\" from \"%s\".\n", dev_env);
	sysmsg.m_type = SYS2AP_MSG_NETLINK_STATUS;
	sprintf(sysmsg.m_msg, "%s UP", dev_env);
	RTK_SYS2AP_SendMsg(&sysmsg);

	return error;
}

static int net_linkdown (void)
{
	int error=0;
	char *dev_env;
	SYS2AP_MESSAGE sysmsg;
	
	dev_env = getenv ("INTERFACE");
	if (!dev_env) {
		dbg ("missing \"INTERFACE\" environment variable, aborting.");
		return 1;
	}

	printf("Hotplug got one Net Hotplug of \"LinkDown\" from \"%s\".\n", dev_env);
	sysmsg.m_type = SYS2AP_MSG_NETLINK_STATUS;
	sprintf(sysmsg.m_msg, "%s DOWN", dev_env);
	RTK_SYS2AP_SendMsg(&sysmsg);

	return error;
}

static int net_pbc (void)
{
	int error=0;
	char *dev_env;
	SYS2AP_MESSAGE sysmsg;
	
	dev_env = getenv ("INTERFACE");
	if (!dev_env) {
		dbg ("missing \"INTERFACE\" environment variable, aborting.");
		return 1;
	}

	printf("Hotplug got one Net Hotplug of \"NetPBC\" from \"%s\".\n", dev_env);
	sysmsg.m_type = SYS2AP_MSG_NETLINK_STATUS;
	sprintf(sysmsg.m_msg, "%s PBC", dev_env);
	RTK_SYS2AP_SendMsg(&sysmsg);

	return error;
}

static struct subsystem net_subsystem[] = {
	{ ADD_STRING, net_add },
	{ REMOVE_STRING, net_remove },
	{ LINKUP_STRING, net_linkup },
	{ LINKDOWN_STRING, net_linkdown },
	{ NET_PBC_STRING, net_pbc },
	{ NULL, NULL }
};


int net_handler (void)
{
	char * action;
	
	action = getenv ("ACTION");
	dbg ("action = %s", action);
	if (action == NULL) {
		dbg ("missing ACTION environment variable, aborting.");
		return 1;
	}

	return call_subsystem (action, net_subsystem);
}

