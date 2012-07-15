/*
 * block.c
 *
 * The block functions
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
#include <string.h>	/* for getenv() */
#include <stdio.h>	/* for getenv() */
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <ctype.h>
// Volume name library is designed by E.J.
#include "volume.h"
#include "hotplug.h"
#include "System2AP.h"

#define DVDPLAYER_LOCK "/var/lock/.DvdPlayer"
#define HOTPLUG_CONFIG_FILE "/var/lock/hotplug/config"
#define HOTPLUG_MOUNT_TMP "/var/lock/hotplug/mount_tmp"
#define HOTPLUG_CONVERT_TMP "/var/lock/hotplug/convert_tmp"
//#define HOTPLUG_RENAME_TMP "/var/lock/hotplug/rename_tmp"
#define BLOCK_ADD_SIGNAL_NUMBER 48
#define BLOCK_REMOVE_SIGNAL_NUMBER 49
#define COMMAND_TRY 4			// the number of mount try
#define UMOUNT_COMMAND_TRY 3		// the number of umount try

//#define NO_DEVICE_FILESYSTEM		// No use device filesystem and the device file path would be like this: /dev/sda1
#define CHECK_RT_SIGNATURE
//#define TRY_MOUNT_MAC_HFS_HFSPLUS

enum device_type {
	TYPE_USB_DEV = 0x0,
	TYPE_SATA_DEV,
	TYPE_NUM,
};

char *type_to_device_path(enum device_type type)
{
        switch (type) {
        case TYPE_USB_DEV:
                return "/tmp/usbmounts";
        case TYPE_SATA_DEV:
                return "/tmp/satamounts";

        default:
                return NULL;
        }
}

#ifndef RESCUE_LINUX
#if 0
// devpath = "/block/sda" or "/block/sda/sda1"
// in either case, we want to get "/sys/block/sda/device", 
// then get the link path of "/sys/block/sda/device".
// from the link path, we can get host_num
static int get_block_host_num(char *host)
{
	char *devpath;
	char *ptr, *ptr1;
	char path[256];
	int i = 0, ret;

	devpath = getenv ("DEVPATH");

	// path = "/sys/block/sda" or "/sys/block/sda/sda1"
	sprintf(path, "/sys/%s", devpath);
	
	if((ptr = strstr(path, "sd")))
	{
		if((ptr1 = strstr(ptr, "/"))) // it should be "/sys/block/sda/sda1"
			*ptr1 = '\0'; // set '\0' at the place of "/" which is after "sd"
		strcat(path, "/device"); // path = "/sys//block/sda/device"
	}
	else
	{
		host = NULL;
		return 1;
	}

	// path = "../../devices/platform/ehci_hcd/usb1/1-2/1-2:1.0/host2/target2:0:0/2:0:0:0"
	// or     "../../devices/SATA_DEV/host0/target0:0:0/0:0:0:0"
	ret = readlink(path, path, sizeof(path)/sizeof(char) - 1);
	if(ret == -1)
		return 1;
	else
		path[ret] = 0;

	// ptr = "host0/target0:0:0/0:0:0:0"
	ptr = strstr(path, "host");
	if(ptr == NULL)
		return 1;

	while(ptr[i] != '/')
	{
		host[i] = ptr[i];
		i++;
	}
	// host = "host0"
	host[i]='\0';
	
	return 0;
}
#endif

// devpath = "/devices/platform/ehci_hcd/usb1/1-1/1-1.1/1-1.1:1.0/host2/target2:0:0/2:0:0:0"
static int get_block_host_num_by_physdevpath(char *host)
{
	char *devpath;
	char *ptr;
	int i = 0;

	devpath = getenv ("PHYSDEVPATH");

	if(!devpath)
	{
		host = NULL;
		return 1;
	}

	// ptr = "host2/target2:0:0/2:0:0:0"
	ptr = strstr(devpath, "host");
	if(ptr == NULL)
		return 1;

	while(ptr[i] != '/')
	{
		host[i] = ptr[i];
		i++;
	}
	// host = "host2"
	host[i]='\0';
	
	return 0;
}
#endif

int block_ptp=0;	// If this is a ptp device, the value of block_ptp will be 1.

void do_create_empty_file(const char *name)
{
	int fd;
	if(name) {
		fd = creat(name, 0644);
		if(fd != -1)
			close(fd);
	}
}

/*
   Translate scsi block's device name.
   	Exp: hotplug will give us this: /block/sda/sda1
   	We would add "/sys" in the beginning, remove "sda1", and add "device" in the end: /sys/block/sda/device
   	Then the file links to an informative path: ../../devices/platform/ehci_hcd/usb1/1-1/1-1:1.0/host0/target0:0:0/0:0:0:0
   	Use the last 4 integers and sda'1' to translate into the device path of sda1: /dev/scsi/host0/bus0/target0/lun0/part1
   	Find the sub-string "1-1:1.0". 1-"1":1.0 tells us the device is located in root port. The last integer of 0:0:0:0 tells us the lun number is 0. Therefore, port name would be: 1-0
   	If the sub-string is "1-1.1:1.0". 1-"1.1":1.0 tells us the device is located in port one of a usb hub, and the usb hub is plugged into the root port. The last integer of 0:0:0:0 tells us the lun number is 0. There, port name would be: 1.1-0
 */
static int device_name_translating(char *dev, int len1, char *port, int len2)
{
	char *devpath;
//	char num[4];
	char *start, *brk_str, *ptr1=NULL, *ptr2=NULL, *ptr3=NULL, *ptr4=NULL, *ptr5;
	int ret, ret2=0;
	char devices[256];

	if(len1<200 || len2<32)
		return -1;			// Too dangerous
	devpath = getenv ("DEVPATH");
	
	if(strlen(devpath)>256-10)		// The devpath is too long
		return -1;
	if(!(ptr5 = strrchr(devpath, '/')))
		return -1;
	ptr5++;
	strcpy(devices, "/sys");
	if(!strncmp(ptr5, "mmc", 3)) {
		if(strstr(ptr5, "p")) {	// ex. mmcblk0p1
			strncat(devices, devpath, (unsigned int)ptr5-(unsigned int)devpath);
		} else {		// ex. mmcblk0
			strcat(devices, devpath);
			strcat(devices, "/");
		}
	} else if(strlen(ptr5) > 3) {
		strncat(devices, devpath, (unsigned int)ptr5-(unsigned int)devpath);
	} else {
		strcat(devices, devpath);
		strcat(devices, "/");
	}
	//printf("[%d] devpath=%s ptr5=%s, devices=%s, dev=%s, port=%s\n", 
			//__LINE__, devpath, ptr5, devices, dev, port);
// Some usb disks have no partition. Therefore, sda may need to be mounted, too.
// If sda has partitions, for example, sda1, it will return 1. On the contrary, it will return 0 or -1.
	if(ptr5[strlen(ptr5)-1]<0x30 || ptr5[strlen(ptr5)-1]>0x39 || !(!strncmp(ptr5, "mmc", 3) && strstr(ptr5, "p"))) {
		char partnum_filename[32], read_buffer[2];
		int partnum_fd=0;
		int out_loop=0;
#define WAIT_PART_NUM_READY	200
		
		sprintf(partnum_filename, "/sys/block/%s/part_num", ptr5);
		while(1) {
			usleep(100000);
			if((partnum_fd=open(partnum_filename, O_RDONLY)) == -1) {
				printf("#######[cfyeh-debug] %s(%d) open %s fail!\n", __func__, __LINE__, partnum_filename);
				break;
			}
			else if(read(partnum_fd, read_buffer, 1) != 1) {
				close(partnum_fd);
				continue;
			} else if(read_buffer[0] == 0x30) {
				printf("Hotplug: \"%s\" has no partition.\n", ptr5);
				ret2 = 0;
				close(partnum_fd);
				break;
			} else if(read_buffer[0]>0x30 && read_buffer[0]<=0x39) {
				printf("Hotplug: \"%s\" has partitions.\n", ptr5);
				ret2 = 1;
				close(partnum_fd);
				break;
			}
			close(partnum_fd);
			out_loop++;
			//printf("Hotplug: out_loop %d\n", out_loop);
			if(out_loop >= WAIT_PART_NUM_READY) {
				printf("Hotplug: wait too long. out_loop %d\n", out_loop);
				break;
			}
		}
	}
	strcat(devices, "device");			// "/sys/block/sda/device" links to, for example, "../../devices/platform/ehci_hcd/usb1/1-1/1-1:1.0/host0/target0:0:0/0:0:0:0". When there are 4 hubs connected in a string, it will be a very long string.
	ret = readlink(devices, devices, len1-1);
	if(ret == -1)
		return -1;
	else
		devices[ret] = 0;
	//printf("[%d] devpath=%s ptr5=%s, devices=%s, dev=%s, port=%s\n", 
			//__LINE__, devpath, ptr5, devices, dev, port);
			
// We would like to know the port that the device is located in
// Now we only support SCSI. For HD, we will copy NULL string to "port".
	if(port && !strncmp(ptr5, "sd", 2)) {
// If the sub-string is "1-1.1:1.0", 1.1 is what we want.
		ptr1 = start = strchr(devices, ':');
		if(!ptr1)
			return -1;
		ret = 0;
		while(*start != '-') {
			start--;
			if(start < devices)
			{
				ret = -1;
				strcpy(port, "");
				break;
			}
		}
		if(!ret)
		{
			start++;
			strncpy(port, start, ptr1-start);
			port[ptr1-start]=0;
// After adding the lun number to the end, port is "1.1-0"
			start = strrchr(devices, ':');
			start++;
			strcat(port, "-");
			strcat(port, start);
		}
	} else if(port)
		strcpy(port, "");

// We would like to know the device path of the device
	if(dev) {
		if(!strncmp(ptr5, "sd", 2)) {
// The last 4 integers, for example, 0:0:0:0 represent host, bus, target, and lun respectively.
			start = strrchr(devices, '/');
			start++;
			if((ptr1 = strtok_r(start, ":", &brk_str)))
				if((ptr2 = strtok_r(NULL, ":", &brk_str)))
					if((ptr3 = strtok_r(NULL, ":", &brk_str)))
						ptr4 = strtok_r(NULL, ":", &brk_str);
			if(!ptr4)
				return -1;
		
			if(strlen(ptr5) > 3)			// ex. sda1
				sprintf(dev, "/dev/scsi/host%s/bus%s/target%s/lun%s/part%s", ptr1, ptr2, ptr3, ptr4, (ptr5+3));
			else					// ex. sda
				sprintf(dev, "/dev/scsi/host%s/bus%s/target%s/lun%s/disc", ptr1, ptr2, ptr3, ptr4);
		} else if(!strncmp(ptr5, "sr", 2)) {	//Ken(20081031): for usb CDROM or DVD
		// The last 4 integers, for example, 0:0:0:0 represent host, bus, target, and lun respectively.
			start = strrchr(devices, '/');
			start++;
			if((ptr1 = strtok_r(start, ":", &brk_str)))
				if((ptr2 = strtok_r(NULL, ":", &brk_str)))
					if((ptr3 = strtok_r(NULL, ":", &brk_str)))
						ptr4 = strtok_r(NULL, ":", &brk_str);
			if(!ptr4)
				return -1;
		
			sprintf(dev, "/dev/scsi/host%s/bus%s/target%s/lun%s/cd", ptr1, ptr2, ptr3, ptr4);
			//printf("[%s] dev=%s\n", __FUNCTION__, dev);
		}else if(!strncmp(ptr5, "hd", 2)) {
			start = strrchr(devices, '/');
			start++;
			if((ptr1 = strtok_r(start, ".", &brk_str)))
				if((ptr2 = strtok_r(NULL, ".", &brk_str)));
			if(!ptr2)
				return -1;
			if(strlen(ptr5) > 3)			// ex. hda1
				sprintf(dev, "/dev/ide/host%s/bus0/target%s/lun0/part%s", ptr1, ptr2, (ptr5+3));
			else					// ex. hda
				sprintf(dev, "/dev/ide/host%s/bus0/target%s/lun0/disc", ptr1, ptr2);
		}else if(!strncmp(ptr5, "mmc", 3)) {
			char tmp[64];
			strcpy(tmp, ptr5);
			if((ptr1 = strtok_r(tmp, "k", &brk_str)))
				if((ptr2 = strtok_r(NULL, "p", &brk_str)));
			if(strlen(brk_str))			// ex. mmcblk0p1
				sprintf(dev, "/dev/mmc/blk%s/part%s", ptr2, brk_str);
			else					// ex. mmcblk0
				sprintf(dev, "/dev/mmc/blk%s/disc", ptr2);
		}else
			strcpy(dev, "");
	}

	return ret2;
}

int do_command(char *command, int count, char *lock_file)
{
	int i, ret=1;
	char str_buf[512];
	struct stat st;

	sprintf(str_buf, "%s/lock_%s", HOTPLUG_MOUNT_TMP, lock_file);
	for(i=0;i<count;) {
		if(stat(str_buf, &st) != -1) {
			usleep(200000);
			continue;
		}
		do_create_empty_file(str_buf);
		i++;
		ret=system(command);
		unlink(str_buf);
		if(!WEXITSTATUS(ret) || i>=count)	// For i>=count, this is to jump out as soon as possible
			break;
		usleep(500000);
	}

	printf("Hotplug: %s\tret: %d\n", command, WEXITSTATUS(ret));
//	sprintf(str_buf, "echo \"Command: %s\tret: %d\" >> /tmp/hotplug_log", command, WEXITSTATUS(ret));
//	system(str_buf);
	
	if(ret == -1)			// "ret == -1" means "system" command errors, e.g., fork problem
		return ret;
	else
		return WEXITSTATUS(ret);
}

static void check_ready(char* ptr, SYS2AP_MESSAGE sysmsg, int is_all_mounted, char* host_num, enum device_type type, int is_block_ptp)
{
#ifndef RESCUE_LINUX
	if ( strncmp(ptr, "sr", 2) ){	
		if(parse_config(HOTPLUG_CONFIG_FILE, "ignore", "Ready_Clear", NULL) > 0)
		{
			printf("#######[cfyeh-debug] %s(%d) find ignore:Ready_Clear at config file.\n", __func__, __LINE__);
			return;
		}
		if(is_all_mounted == 1)
		{
			printf("Hotplug got one BLOCK Hotplug of \"Ready\" from \"%s\"\n", ptr);
			sysmsg.m_type = SYS2AP_MSG_BLOCK;
			if (getenv("PORT_STRUCT"))
				sprintf(sysmsg.m_msg, "%s %s %s %s %s", ptr, SYS2AP_MSG_READY,
						(is_block_ptp) ? "ptp" : host_num,
						(type == TYPE_SATA_DEV) ? "SATA" : "USB", getenv("PORT_STRUCT"));
			else
				sprintf(sysmsg.m_msg, "%s %s %s %s %s", ptr, SYS2AP_MSG_READY,
						(is_block_ptp) ? "ptp" : host_num,
						(type == TYPE_SATA_DEV) ? "SATA" : "USB", ptr);
			RTK_SYS2AP_SendMsg(&sysmsg);
		}
	}
#endif
}

static void check_clear(char* ptr, SYS2AP_MESSAGE sysmsg, int is_all_umounted, char* host_num, enum device_type type, int is_block_ptp)
{
#ifndef RESCUE_LINUX
	if ( strncmp(ptr, "sr", 2) ){	
		if(parse_config(HOTPLUG_CONFIG_FILE, "ignore", "Ready_Clear", NULL) > 0)
		{
			printf("#######[cfyeh-debug] %s(%d) find ignore:Ready_Clear at config file.\n", __func__, __LINE__);
			return;
		}
		if(is_all_umounted == 1)
		{
			printf("Hotplug got one BLOCK Hotplug of \"Clear\" from \"%s\"\n", ptr);
			sysmsg.m_type = SYS2AP_MSG_BLOCK;
			if (getenv("PORT_STRUCT"))
				sprintf(sysmsg.m_msg, "%s %s %s %s %s", ptr, SYS2AP_MSG_CLEAR,
						(is_block_ptp) ? "ptp" : host_num,
						(type == TYPE_SATA_DEV) ? "SATA" : "USB", getenv("PORT_STRUCT"));
			else
				sprintf(sysmsg.m_msg, "%s %s %s %s %s", ptr, SYS2AP_MSG_CLEAR,
						(is_block_ptp) ? "ptp" : host_num,
						(type == TYPE_SATA_DEV) ? "SATA" : "USB", ptr);
			RTK_SYS2AP_SendMsg(&sysmsg);
		}
	}
#endif
}

inline static void check_ptp_write_lockdata(int is_block_ptp, int fd) {
#ifndef RESCUE_LINUX // we do not mount sata at rescue linux
	if(is_block_ptp)
		write_lockdata_fd(fd, getenv("DEVPATH"), strlen(getenv("DEVPATH")));
	else
		write_lockdata_fd(fd, getenv("PHYSDEVPATH"), strlen(getenv("PHYSDEVPATH")));
#endif
}

inline static void check_ptp_testwait_lockdata(int is_block_ptp, const char *filename) {
#ifndef RESCUE_LINUX // we do not mount sata at rescue linux
	if(is_block_ptp)
		testwait_lockdata(filename, getenv("DEVPATH"), strlen(getenv("DEVPATH")));
	else
		testwait_lockdata(filename, getenv("PHYSDEVPATH"), strlen(getenv("PHYSDEVPATH")));
#endif
}

inline static void check_ptp_testclear_lockdata(int is_block_ptp, const char *filename) {
#ifndef RESCUE_LINUX // we do not mount sata at rescue linux
	if(is_block_ptp)
		testclear_lockdata(filename, getenv("DEVPATH"), strlen(getenv("DEVPATH")));
	else
		testclear_lockdata(filename, getenv("PHYSDEVPATH"), strlen(getenv("PHYSDEVPATH")));
#endif
}

/*
   An example of hotplug event of usb block device:
      /sbin/hotplug usb
        Environment: PHYSDEVPATH=/devices/platform/ehci_hcd/usb1/1-1/1-1.4/1-1.4:1.0/host7/target7:0:0/7:0:0:0 HOME=/ PATH=/sbin:/bin:/usr/sbin:/usr/bin ACTION=add DEVPATH=/block/sda SUBSYSTEM=usb .....
 */
static int block_add (void)
{
	char *disk_name, disk_name2[32], name[256], name2[64], command[280];
	int ret = 0;
	char *devpath;
	char *ptr;
	char lockfile[256];
	enum device_type type = TYPE_NUM;
	SYS2AP_MESSAGE sysmsg;
	char host_num[8], path[256];
	int label_num = 0;
	char label_path[256];
	int is_all_mounted;
	int device_type = 0;
	int fd = -1;
#ifdef CHECK_RT_SIGNATURE
	int signature = 0;
	int is_ignore_signature = 0;
#endif /* CHECK_RT_SIGNATURE */

	if(getenv("BUS_TYPE")) {
		if(parse_config(HOTPLUG_CONFIG_FILE, "ignore", getenv("BUS_TYPE"), getenv("PORT_STRUCT")) > 0)
			return 1;
		if(strstr(getenv("BUS_TYPE"), "usb"))
			type = TYPE_USB_DEV;
		else if(strstr(getenv("BUS_TYPE"), "sata"))
			type = TYPE_SATA_DEV;
	}

#ifdef RESCUE_LINUX // we do not mount sata at rescue linux
	if(type == TYPE_SATA_DEV)
		return 0;
#endif

#ifdef CHECK_RT_SIGNATURE
	if(getenv("RT_SIGNATURE"))
	if(strstr(getenv("RT_SIGNATURE"), "1")) {
		if(parse_config(HOTPLUG_CONFIG_FILE, "ignore", "signature", getenv("PORT_STRUCT")) > 0) {
			is_ignore_signature = 1;
			signature = 0;
		}
		else
			signature = 1;
	}
#endif /* CHECK_RT_SIGNATURE */

	if(getenv("DISC_TYPE"))
	if(strstr(getenv("DISC_TYPE"), "1"))
		device_type = 1;

// The directories in /var/lock/hotplug should be ready or else it may disappear if hotplug comes at the same time with mkdir in rcS
	disk_name=disk_name2;
	wait_dir_ready(HOTPLUG_MOUNT_TMP, 100);
	wait_dir_ready(HOTPLUG_CONVERT_TMP, 100);
	wait_dir_ready("/sys/block", 100);

	devpath = getenv ("DEVPATH");
	if(!(ptr = strrchr(devpath, '/')))	// "/block/sda/sda1"
		return 1;

#ifndef RESCUE_LINUX
	get_block_host_num_by_physdevpath(host_num); // host_num = "host*"
#endif
	ptr+=1;

	// Only SCSI and PTP block devices will be mounted by block module of hotplug.
	// The names of SCSI block devices will be like sda and sdb1.
	// The names of PTP block devices will be like 1-1:1.0, 1-1.1:1.0, 1-1.1.2:1.0...
	// Here we check for these 2 kinds of block device by comparing the names.
	if(strncmp(ptr, "sd", 2) && strncmp(ptr, "sr", 2) && strncmp(ptr, "mmc", 3)) {
		char *ptr1;
		char special_char[4]=":.";
		int special_char_ptr=0, hasnum=0, pattern_ok=1;
		
		if(!strncmp(ptr, "1-", 2) || !strncmp(ptr, "2-", 2)) {
			for(ptr1=ptr+2;(*ptr1)!=0;ptr1++) {
				if(isdigit(*ptr1)) {
					hasnum=1;
					continue;
				}
				if(hasnum) {
					if(!special_char_ptr && (*ptr1)=='.') {
						hasnum=0;
						continue;
					} else if((*ptr1)==special_char[special_char_ptr]) {
						special_char_ptr++;
						hasnum=0;
						continue;
					}
				}
				pattern_ok=0;
				break;
			}
		}
		if(special_char_ptr != 2 || !hasnum)
			pattern_ok=0;
		if(!pattern_ok) {
			printf("Hotplug: device \"%s\" is skipped.\n", ptr);
			return 1;
		}
	}

	sprintf(lockfile, "%s/.lock_%s", HOTPLUG_MOUNT_TMP, ptr);
// Some devices, like mtdblock, won't have "PHYSDEVPATH" environment variable. Therefore, we would like to ignore them as soon as possible.
	if(parse_config(HOTPLUG_CONFIG_FILE, "ignore", ptr, 0) > 0)
		return 1;
#ifndef RESCUE_LINUX // we do not mount sata at rescue linux
	// create lock data
	while((fd = create_lockdata(lockfile)) < 0)
		usleep(10);
#endif

// Delete all the tmp or lock files
	sprintf(command, "rm -rf %s/lock_%s", HOTPLUG_MOUNT_TMP, ptr);
	system(command);
	sprintf(command, "rm -rf %s/.d_%s", HOTPLUG_MOUNT_TMP, ptr);
	system(command);
	sprintf(command, "rmdir %s/.%s", HOTPLUG_MOUNT_TMP, ptr);
	system(command);
	if(block_ptp) {
		sprintf(name, "ptp:%s", devpath);
		setenv("PORT_STRUCT", ptr, 0);
	} else {
// When there is no device filesystem, we still need to know "name2" to parse_config.
		ret = device_name_translating(name, 256, name2, 64);
		if(ret < 0) {
			check_ptp_write_lockdata(block_ptp, fd);
			return ret;
		}
		// for no partition SATA HDD, sleep(1)
		if((ret == 0) && (type == TYPE_SATA_DEV))
			sleep(1);
#ifdef NO_DEVICE_FILESYSTEM
		sprintf(name, "/dev/%s", ptr);
#endif

#ifndef RESCUE_LINUX
#ifdef CHECK_RT_SIGNATURE
		// if sata hdd signatured, send message when device_type = 1, else return 0
		if((type == TYPE_SATA_DEV) && (signature == 1))
		{
			//if(device_type == 1) // because it is late for disc to get signature status
			if(isdigit(ptr[strlen(ptr)-1]) == 0) // for sdx only
			{
				printf("Hotplug got one BLOCK Hotplug of \"BLOCK_SATA_SIGNATURE\" from \"%s\"\n", ptr);
				sysmsg.m_type = SYS2AP_MSG_BLOCK_SATA_SIGNATURE;
				sprintf(sysmsg.m_msg, "%s %s", ptr, name);
				RTK_SYS2AP_SendMsg(&sysmsg);
				printf("Hotplug got one BLOCK Hotplug of \"Ready\" from \"%s\"\n", ptr);
				sysmsg.m_type = SYS2AP_MSG_BLOCK;
				if (getenv("PORT_STRUCT"))
					sprintf(sysmsg.m_msg, "%s %s %s %s %s", ptr, SYS2AP_MSG_READY,
							(block_ptp) ? "ptp" : host_num,
							(type == TYPE_SATA_DEV) ? "SATA" : "USB", getenv("PORT_STRUCT"));
				else
					sprintf(sysmsg.m_msg, "%s %s %s %s %s", ptr, SYS2AP_MSG_READY,
							(block_ptp) ? "ptp" : host_num,
							(type == TYPE_SATA_DEV) ? "SATA" : "USB", ptr);
				RTK_SYS2AP_SendMsg(&sysmsg);
			}
			check_ptp_write_lockdata(block_ptp, fd);
			return 0;
		}
#endif /* CHECK_RT_SIGNATURE */
#endif

#ifndef RESCUE_LINUX
		if ( !strncmp(ptr, "sr", 2) ){	//Ken(20081113): for usb CDROM or DVD
			printf("Hotplug got one USB CDROM Hotplug of \"Add\" from \"%s\"\n", ptr);
			sysmsg.m_type = SYS2AP_MSG_USBCDROM;
			sprintf(sysmsg.m_msg, "%s UP %s", ptr, name);
			//printf("sysmsg.m_msg=%s\n", sysmsg.m_msg);
			RTK_SYS2AP_SendMsg(&sysmsg);
		}
#endif
	}
	while(parse_config(HOTPLUG_CONFIG_FILE, "delay", ptr, 0) > 0) sleep(1);
	if(!block_ptp && parse_config(HOTPLUG_CONFIG_FILE, "convert", ptr, 0) > 0) {
		if(strlen(ptr)>3)
			sprintf(command, "echo %s >%s/%s", name, HOTPLUG_CONVERT_TMP, ptr+3);
		else
			sprintf(command, "echo %s >%s/0", name, HOTPLUG_CONVERT_TMP);
		system(command);
		check_ptp_write_lockdata(block_ptp, fd);
		return 1;
	}
	if(!block_ptp && parse_config(HOTPLUG_CONFIG_FILE, "convert", name2, 0) == 1) {
		sprintf(command, "echo %s >%s/name", ptr, HOTPLUG_CONVERT_TMP);
		system(command);
		if(strlen(ptr)>3)
			sprintf(command, "echo %s >%s/%s", name, HOTPLUG_CONVERT_TMP, ptr+3);
		else
			sprintf(command, "echo %s >%s/0", name, HOTPLUG_CONVERT_TMP);
		system(command);
		check_ptp_write_lockdata(block_ptp, fd);
		return 1;
	}
	if(!block_ptp && parse_config(HOTPLUG_CONFIG_FILE, "rename", name2, disk_name) == 1) {
		if(strlen(ptr)>3) {
			strcat(disk_name, "_");
			strcat(disk_name, ptr+3);
		}
//		sprintf(command, "echo -n %s >%s/%s", disk_name, HOTPLUG_RENAME_TMP, ptr);
//		system(command);
	} else {
//		strcpy(disk_name, ptr);
		disk_name = NULL;
	}
// The disc has partition. Therefore, we skip mounting, for example, sda.
	if(!block_ptp && ret == 1) {
		check_ptp_write_lockdata(block_ptp, fd);
		return 1;
	}
	sprintf(command, "mkdir -p %s/.%s", HOTPLUG_MOUNT_TMP, ptr);
	if(system(command) == -1) {
		printf("Hotplug: Mkdir \"%s/.%s\" fail.\n", HOTPLUG_MOUNT_TMP, ptr);
		label_num = add_partition(ptr, name, disk_name, 0);
		check_ptp_write_lockdata(block_ptp, fd);
		return 1;
	}
	if(block_ptp) {
		sprintf(command, "mount -t ptpfs %s %s/.%s", ptr, HOTPLUG_MOUNT_TMP, ptr);
		if((ret=do_command(command, COMMAND_TRY, ptr)) == -1) {
			printf("Hotplug: Mount \"%s\" fail.\n", ptr);
			label_num = add_partition(ptr, name, disk_name, 0);
			check_ptp_write_lockdata(block_ptp, fd);
			return 1;
		}
	} else {
		//printf("[%d]name=%s \n", __LINE__, name);
		if ( !strncmp(ptr, "sr", 2) ){	//Ken(20081031): for usb CDROM or DVD
#if 0		
			sprintf(command, "mount -t udf -o ro %s %s/.%s", name, HOTPLUG_MOUNT_TMP, ptr);
			if((ret=do_command(command, COMMAND_TRY, ptr)) == -1) {
				printf("[%d]Hotplug: Mount \"%s\" fail.\n", __LINE__, ptr);
				label_num = add_partition(ptr, name, disk_name, 0);
				check_ptp_write_lockdata(block_ptp, fd);
				return 1;
			}else if(ret) {
			 	sprintf(command, "mount -t iso9660 -o ro %s %s/.%s", name, HOTPLUG_MOUNT_TMP, ptr);
				if((ret=do_command(command, 1, ptr)) == -1) {
					printf("[%d]Hotplug: Mount \"%s\" fail.\n", __LINE__, ptr);
					label_num = add_partition(ptr, name, disk_name, 0);
					check_ptp_write_lockdata(block_ptp, fd);
					return 1;
				}else if(ret) {
			    		sprintf(command, "mount -t vcd -o ro %s %s/.%s", name, HOTPLUG_MOUNT_TMP, ptr);
					if((ret=do_command(command, 1, ptr)) == -1) {
						printf("[%d]Hotplug: Mount \"%s\" fail.\n", __LINE__, ptr);
						label_num = add_partition(ptr, name, disk_name, 0);
					check_ptp_write_lockdata(block_ptp, fd);
					return 1;
					}
				}
			}
#endif
		}else{
			sprintf(command, "mount -t ufsd -o ro -o nls=utf8 -o umask=0 %s %s/.%s", name, HOTPLUG_MOUNT_TMP, ptr);
			if((ret=do_command(command, 1, ptr)) == -1) {
				printf("[%d]Hotplug: Mount \"%s\" fail.\n", __LINE__, ptr);
				label_num = add_partition(ptr, name, disk_name, 0);
				check_ptp_write_lockdata(block_ptp, fd);
				return 1;
			} else if(ret) {
				sprintf(command, "mount -t ntfs -o ro -o nls=utf8 -o umask=0 %s %s/.%s", name, HOTPLUG_MOUNT_TMP, ptr);
// Why we still need to retry this command? Because I found that sometimes the first "mount ntfs" will fail alought there is already delay when mounting VFAT.
// I think this is because when "mount sda" and "mount sda1" happen at the same time, it will have problem.
				if((ret=do_command(command, 1, ptr)) == -1) {
					printf("[%d]Hotplug: Mount \"%s\" fail.\n", __LINE__, ptr);
					label_num = add_partition(ptr, name, disk_name, 0);
					check_ptp_write_lockdata(block_ptp, fd);
					return 1;
				}else if(ret) {
					sprintf(command, "mount -t vfat -o ro,shortname=winnt -o utf8 -o umask=0 %s %s/.%s", name, HOTPLUG_MOUNT_TMP, ptr);
					if((ret=do_command(command, 1, ptr)) == -1) {
						printf("[%d]Hotplug: Mount \"%s\" fail.\n", __LINE__, ptr);
						label_num = add_partition(ptr, name, disk_name, 0);
						check_ptp_write_lockdata(block_ptp, fd);
						return 1;
#if 0
					// if ((type == TYPE_SATA_DEV)), we don't try to mount ext3 filesystem
					}else if(ret && (type == TYPE_SATA_DEV)) {
#ifndef RESCUE_LINUX
						printf("Hotplug got one BLOCK Hotplug of \"NO_MOUNT\" from \"%s\"\n", ptr);
						sysmsg.m_type = SYS2AP_MSG_BLOCK_SATA_NO_MOUNT;
						sprintf(sysmsg.m_msg, "%s %s", ptr, name);
						RTK_SYS2AP_SendMsg(&sysmsg);
#endif
#endif
					}else if(ret) {
						sprintf(command, "mount -t ext3 -o ro %s %s/.%s", name, HOTPLUG_MOUNT_TMP, ptr);
						if(!is_ignore_signature && !signature && ((ret=do_command(command, 1, ptr)) == -1)) {
							printf("[%d]Hotplug: Mount \"%s\" fail.\n", __LINE__, ptr);
							label_num = add_partition(ptr, name, disk_name, 0);
							check_ptp_write_lockdata(block_ptp, fd);
							return 1;
						}
#ifdef TRY_MOUNT_MAC_HFS_HFSPLUS
						else if(ret) { // try mac filesystem
							sprintf(command, "mount -t hfs %s %s/.%s", name, HOTPLUG_MOUNT_TMP, ptr);
							if((ret=do_command(command, 1, ptr)) == -1) {
								printf("[%d]Hotplug: Mount \"%s\" fail.\n", __LINE__, ptr);
								label_num = add_partition(ptr, name, disk_name, 0);
								check_ptp_write_lockdata(block_ptp, fd);
								return 1;
							} else if(ret) {
								sprintf(command, "mount -t hfsplus %s %s/.%s", name, HOTPLUG_MOUNT_TMP, ptr);
								if((ret=do_command(command, 1, ptr)) == -1) {
									printf("[%d]Hotplug: Mount \"%s\" fail.\n", __LINE__, ptr);
									label_num = add_partition(ptr, name, disk_name, 0);
									check_ptp_write_lockdata(block_ptp, fd);
									return 1;
								}
							}
						}
#endif /* TRY_MOUNT_MAC_HFS_HFSPLUS */
					}
				}
			}
		}
	}

//	if(false == 1) {
//		sprintf(command, "rmdir %s/.%s", HOTPLUG_MOUNT_TMP, ptr);
//		system(command);
//		check_ptp_write_lockdata(block_ptp, fd);
//		return 1;
//	}

	sprintf(name2, "%s/.d_%s", HOTPLUG_MOUNT_TMP, ptr);
	do_create_empty_file(name2);
	sprintf(path, "%s/%s", type_to_device_path(TYPE_USB_DEV), ptr);

	sprintf(command, "mkdir -p %s", path);
	if(system(command) == -1) {
		printf("Hotplug: Mkdir \"%s\" fail.\n", path);
		label_num = add_partition(ptr, name, disk_name, 0);
		check_ptp_write_lockdata(block_ptp, fd);
		return 1;
	}
	if(block_ptp)
		sprintf(command, "mount -t ptpfs --move %s/.%s %s", HOTPLUG_MOUNT_TMP, ptr, path);
	else
		sprintf(command, "mount --move %s/.%s %s", HOTPLUG_MOUNT_TMP, ptr, path);
	if(system(command)) {
		sprintf(command, "rmdir %s", path);
		system(command);
		sprintf(command, "rm -f %s", name2);
		system(command);
//		sprintf(command, "rm -rf %s/%s", HOTPLUG_RENAME_TMP, ptr);
//		system(command);
		printf("Hotplug: Mount --move \"%s\" fail.\n", path);
		label_num = add_partition(ptr, name, disk_name, &is_all_mounted);
		check_ptp_write_lockdata(block_ptp, fd);
		check_ready(ptr, sysmsg, is_all_mounted, host_num, type, block_ptp);
		return 1;
	}

	if(getenv("EFI_SYSTEM"))
	if(strstr(getenv("EFI_SYSTEM"), "1")) {
		printf("#######[cfyeh-debug] %s(%d) a device w/ EFI_SYSTEM args!\n", __func__, __LINE__);
		sprintf(command, "umount %s", path);
		printf("command: %s\n", command);
		system(command);
		label_num = add_partition(ptr, name, disk_name, &is_all_mounted);
		check_ptp_write_lockdata(block_ptp, fd);
		check_ready(ptr, sysmsg, is_all_mounted, host_num, type, block_ptp);
		return 0;
	}

	sprintf(command, "rm -f %s", name2);		// This lock file may be left for a long time.
	system(command);
	if(block_ptp)
		label_num = add_partition(ptr, name, disk_name, &is_all_mounted);
	else
	{
		label_num = add_partition(ptr, NULL, disk_name, &is_all_mounted);
	}

	printf("Hotplug: Mount \"%s\" successfully.\n", ptr);

	signal_pid(DVDPLAYER_LOCK, BLOCK_ADD_SIGNAL_NUMBER);

#ifndef RESCUE_LINUX
	if ( strncmp(ptr, "sr", 2) ){	
		if(label_num > 0)
			get_mntpath(label_num, label_path);
		else
			label_path[0]='\0';

		printf("Hotplug got one BLOCK Hotplug of \"Add\" from \"%s\"\n", ptr);
		sysmsg.m_type = SYS2AP_MSG_BLOCK;
		sprintf(sysmsg.m_msg, "%s %s %s %s", ptr, SYS2AP_MSG_UP, path, label_path);
		RTK_SYS2AP_SendMsg(&sysmsg);
	}
#endif

	check_ptp_write_lockdata(block_ptp, fd);
	check_ready(ptr, sysmsg, is_all_mounted, host_num, type, block_ptp);
	return 0;
}

static int block_remove (void)
{
//	char name[256], name2[32], command[280];
	char name[256], command[280];
	char *devpath;
	char *ptr;
	struct stat st;
	int ret=0;
//	FILE *fd;
	char lockfile[256];
	enum device_type type = TYPE_NUM;
	SYS2AP_MESSAGE sysmsg;
	char path[256];
	int is_all_umounted;
	char host_num[8];
#ifdef CHECK_RT_SIGNATURE
	int signature = 0;
#endif /* CHECK_RT_SIGNATURE */

#ifndef RESCUE_LINUX
	get_block_host_num_by_physdevpath(host_num); // host_num = "host*"
#endif
	if(getenv("BUS_TYPE")) {
		if(parse_config(HOTPLUG_CONFIG_FILE, "ignore", getenv("BUS_TYPE"), getenv("PORT_STRUCT")) > 0)
			return 1;
		if(strstr(getenv("BUS_TYPE"), "usb"))
			type = TYPE_USB_DEV;
		else if(strstr(getenv("BUS_TYPE"), "sata"))
			type = TYPE_SATA_DEV;
	}

#ifdef CHECK_RT_SIGNATURE
	if(getenv("RT_SIGNATURE"))
	if(strstr(getenv("RT_SIGNATURE"), "1")) {
		if(parse_config(HOTPLUG_CONFIG_FILE, "ignore", "signature", getenv("PORT_STRUCT")) > 0) {
			signature = 0;
		}
		else
			signature = 1;
	}
#endif /* CHECK_RT_SIGNATURE */

#ifdef RESCUE_LINUX // we do not mount sata at rescue linux
	if(type == TYPE_SATA_DEV)
		return 0;
#endif

	devpath = getenv ("DEVPATH");
	if(!(ptr = strrchr(devpath, '/')))	// "/block/sda/sda1"
		return 1;
	
	ptr+=1;

	// Only SCSI and PTP block devices will be mounted by block module of hotplug.
	// The names of SCSI block devices will be like sda and sdb1.
	// The names of PTP block devices will be like 1-1:1.0, 1-1.1:1.0, 1-1.1.2:1.0...
	// Here we check for these 2 kinds of block device by comparing the names.
	if(strncmp(ptr, "sd", 2) && strncmp(ptr, "sr", 2) && strncmp(ptr, "mmc", 3)) {
		char *ptr1;
		char special_char[4]=":.";
		int special_char_ptr=0, hasnum=0, pattern_ok=1;
		
		if(!strncmp(ptr, "1-", 2) || !strncmp(ptr, "2-", 2)) {
			for(ptr1=ptr+2;(*ptr1)!=0;ptr1++) {
				if(isdigit(*ptr1)) {
					hasnum=1;
					continue;
				}
				if(hasnum) {
					if(!special_char_ptr && (*ptr1)=='.') {
						hasnum=0;
						continue;
					} else if((*ptr1)==special_char[special_char_ptr]) {
						special_char_ptr++;
						hasnum=0;
						continue;
					}
				}
				pattern_ok=0;
				break;
			}
		}
		if(special_char_ptr != 2 || !hasnum)
			pattern_ok=0;
		if(!pattern_ok) {
			printf("Hotplug: device \"%s\" is skipped.\n", ptr);
			return 1;
		}
	}

	sprintf(lockfile, "%s/.lock_%s", HOTPLUG_MOUNT_TMP, ptr);
	if(parse_config(HOTPLUG_CONFIG_FILE, "ignore", ptr, 0) > 0) {
		return 1;
	}
	// wait lock file ready
	check_ptp_testwait_lockdata(block_ptp, lockfile);

#ifndef RESCUE_LINUX
#ifdef CHECK_RT_SIGNATURE
	// if sata hdd signatured, send message when device_type = 1, else return 0
	if((type == TYPE_SATA_DEV) && (signature == 1))
	{
		//if(device_type == 1) // because it is late for disc to get signature status
		if(isdigit(ptr[strlen(ptr)-1]) == 0) // for sdx only
		{
			printf("Hotplug got one BLOCK Hotplug of \"Clear\" from \"%s\"\n", ptr);
			sysmsg.m_type = SYS2AP_MSG_BLOCK;
			if (getenv("PORT_STRUCT"))
				sprintf(sysmsg.m_msg, "%s %s %s %s %s", ptr, SYS2AP_MSG_CLEAR,
						(block_ptp) ? "ptp" : host_num,
						(type == TYPE_SATA_DEV) ? "SATA" : "USB", getenv("PORT_STRUCT"));
			else
				sprintf(sysmsg.m_msg, "%s %s %s %s %s", ptr, SYS2AP_MSG_CLEAR,
						(block_ptp) ? "ptp" : host_num,
						(type == TYPE_SATA_DEV) ? "SATA" : "USB", ptr);
			RTK_SYS2AP_SendMsg(&sysmsg);
		}
		check_ptp_testclear_lockdata(block_ptp, lockfile);
		return 0;
	}
#endif /* CHECK_RT_SIGNATURE */
#endif

#ifndef RESCUE_LINUX
	//Ken(20081113): for usb CDROM or DVD
	if ( !strncmp(ptr, "sr", 2) ){	
		printf("Hotplug got one USB CDROM Hotplug of \"Remove\" from \"%s\"\n", ptr);
		sysmsg.m_type = SYS2AP_MSG_USBCDROM;
		sprintf(sysmsg.m_msg, "%s DOWN", ptr);
		//printf("sysmsg.m_msg=%s\n", sysmsg.m_msg);
		RTK_SYS2AP_SendMsg(&sysmsg);
	}
#endif
	
// We cannot use this function at this time because the sys directory of the device may disappear immediately when it is unplugged.
// However, we can use this on card reader which always has sys directory.
//	ret = device_name_translating(name, 256, name2, 32);
// HOTPLUG_RENAME_TMP directory stores the files that have been renamed. Use them instead of "device_name_translating"
/*	sprintf(name, "%s/%s", HOTPLUG_RENAME_TMP, ptr);
	if(!stat(name, &st) && S_ISREG(st.st_mode)) {
		fd = fopen(name, "rt");
		if(!fd) {
			unlink(name);
			del_partition(ptr, 0);
			printf("Hotplug: Open file \"%s\" fail.\n", name);
			return 1;
		}
		if(!fgets(disk_name, 16, fd)) {
			fclose(fd);
			unlink(name);
			del_partition(ptr, 0);
			printf("Hotplug: Read file \"%s\" fail.\n", name);
			return 1;
		}
		fclose(fd);
		unlink(name);
	} else
		strcpy(disk_name, ptr);
*/
	if(!block_ptp && parse_config(HOTPLUG_CONFIG_FILE, "convert", ptr, 0) > 0) {
		if(strlen(ptr)>3) {
			sprintf(command, "rm -f %s/%s", HOTPLUG_CONVERT_TMP, ptr+3);
			printf("Hotplug: Delete convert file \"%s/%s\".\n", HOTPLUG_CONVERT_TMP, ptr+3);
		} else {
			sprintf(command, "rm -f %s/0", HOTPLUG_CONVERT_TMP);
			printf("Hotplug: Delete convert file \"%s/0\".\n", HOTPLUG_CONVERT_TMP);
		}
		system(command);
		check_ptp_testclear_lockdata(block_ptp, lockfile);
		return 1;
	}
//	if(rename) {
// When usb disk is unplugged, /sys/block/xxx always disappears first and the following actions may not be executed.
//		if(parse_config(HOTPLUG_CONFIG_FILE, "convert", name2, 0) == 1) {
//			if(strlen(ptr)>3)
//				sprintf(command, "rm -f %s/%s", HOTPLUG_CONVERT_TMP, ptr+3);
//			else
//				sprintf(command, "rm -f %s/0", HOTPLUG_CONVERT_TMP);
//			system(command);
//			return 1;
//		}
//		if(parse_config(HOTPLUG_CONFIG_FILE, "rename", name2, disk_name) == 1) {
//		} else {
//			strcpy(disk_name, ptr);
//		}
//	} else {
//		strcpy(disk_name, ptr);
//	}
	// Check if the directory exists.

	// test sata path to see if the device is from sata
	sprintf(path, "%s/%s", type_to_device_path(TYPE_USB_DEV), ptr);

	if(stat(path, &st) || !S_ISDIR(st.st_mode)) {
		printf("Hotplug: Cannot find \"%s\".\n", path);
		del_partition(ptr, &is_all_umounted);
		check_ptp_testclear_lockdata(block_ptp, lockfile);
		check_clear(ptr, sysmsg, is_all_umounted, host_num, type, block_ptp);
		return 1;
	}

	// Lock file to notify that the corresponding directory cannot be accessed
	sprintf(name, "%s/.d_%s", HOTPLUG_MOUNT_TMP, ptr);
	do_create_empty_file(name);

	del_partition(ptr, &is_all_umounted);
	signal_pid(DVDPLAYER_LOCK, BLOCK_REMOVE_SIGNAL_NUMBER);

#ifndef RESCUE_LINUX
	if ( strncmp(ptr, "sr", 2) ){	
		printf("Hotplug got one BLOCK Hotplug of \"Remove\" from \"%s\"\n", ptr);
		sysmsg.m_type = SYS2AP_MSG_BLOCK;
		sprintf(sysmsg.m_msg, "%s %s", ptr, SYS2AP_MSG_DOWN);
		RTK_SYS2AP_SendMsg(&sysmsg);
	}
#endif

	sprintf(command, "umount -f -l %s", path);

	if(getenv("EFI_SYSTEM"))
	if(strstr(getenv("EFI_SYSTEM"), "1"))
		goto out;

	if(do_command(command, UMOUNT_COMMAND_TRY, ptr))
		ret = 1;
	else {
		sprintf(command, "rmdir %s", path);
		system(command);
		sprintf(command, "rm -f %s", name);
		system(command);
	}
	printf("Hotplug: Umount %s successfully.\n", ptr);
out:
	check_ptp_testclear_lockdata(block_ptp, lockfile);
	check_clear(ptr, sysmsg, is_all_umounted, host_num, type, block_ptp);
	return ret;
}


static struct subsystem block_subsystem[] = {
	{ ADD_STRING, block_add },
	{ REMOVE_STRING, block_remove },
	{ NULL, NULL }
};


int block_handler (void)
{
	char * action;

	action = getenv ("ACTION");
	dbg ("action = %s", action);
	if (action == NULL) {
		dbg ("missing ACTION environment variable, aborting.");
		return 1;
	}

	return call_subsystem (action, block_subsystem);
}


