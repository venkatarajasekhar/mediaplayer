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
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>

#include <linux/fs.h>

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
#define UMOUNT_COMMAND_TRY 40		// the number of umount try

//#define NO_DEVICE_FILESYSTEM		// No use device filesystem and the device file path would be like this: /dev/sda1

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
	if(strlen(ptr5) > 3)
		strncat(devices, devpath, (unsigned int)ptr5-(unsigned int)devpath);
	else {
		strcat(devices, devpath);
		strcat(devices, "/");
	}
// Some usb disks have no partition. Therefore, sda may need to be mounted, too.
// If sda has partitions, for example, sda1, it will return 1. On the contrary, it will return 0 or -1.
	if(ptr5[strlen(ptr5)-1]<0x30 || ptr5[strlen(ptr5)-1]>0x39) {
		DIR *pDIR;
		struct dirent *pDirEnt;
		int i;
		// Delay 0.3s because the /sys/block/sda/sda1 sometimes would not be ready yet.
		usleep(500000);
		pDIR = opendir(devices);
		if(!pDIR)
			return -1;
		while(1) {
			pDirEnt = readdir( pDIR );
			if(!pDirEnt)
				break;
			if(!strncmp(ptr5, pDirEnt->d_name, strlen(ptr5))) {
				for(i=strlen(ptr5);;i++) {
					char character=pDirEnt->d_name[i];
	
					if(character == '\0') {
						ret2=1;
						break;
					}
					if(character<0x30 && character>0x39)
						break;
				}
			}
			if(ret2)
				break;
		}
		
		closedir( pDIR );
	}
	strcat(devices, "device");			// "/sys/block/sda/device" links to, for example, "../../devices/platform/ehci_hcd/usb1/1-1/1-1:1.0/host0/target0:0:0/0:0:0:0". When there are 4 hubs connected in a string, it will be a very long string.
	ret = readlink(devices, devices, len1-1);
	if(ret == -1)
		return -1;
	else
		devices[ret] = 0;

// We would like to know the port that the device is located in
// Now we only support SCSI. For HD, we will copy NULL string to "port".
	if(port && !strncmp(ptr5, "sd", 2)) {
// If the sub-string is "1-1.1:1.0", 1.1 is what we want.
		ptr1 = start = strchr(devices, ':');
		if(!ptr1)
			return -1;
		while(*start != '-') {
			start--;
			if(start < devices)
				return -1;
		}
		start++;
		strncpy(port, start, ptr1-start);
		port[ptr1-start]=0;
// After adding the lun number to the end, port is "1.1-0"
		start = strrchr(devices, ':');
		start++;
		strcat(port, "-");
		strcat(port, start);
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
		} else if(!strncmp(ptr5, "hd", 2)) {
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
		}
		else
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
		if(!WEXITSTATUS(ret))
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


int do_command2(const char *command, const char *source, const char *target, const char *filesystemtype, 
			unsigned long mountflags, const void *data, int count, char *lock_file)
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
		if ( !strcmp(command, "mount") ){   /* mount function */
 		      ret = mount(source, target, filesystemtype, mountflags, (char *)data);
		     //printf("CALL MOUNT()\n");
		}else{      /* unmount function */ 
			 ret = umount2(target, mountflags);
			 //printf("CALL UNMOUNT2()\n");
		}
		unlink(str_buf);
		if( !(ret) )
			break;
		usleep(500000);
	}
	
	printf("Hotplug: %s\tret: %d\n", command, ret);
	return ret;
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

     char del_filename[100], open_filename[100];
     int fd;
     char mnt_source[100], mnt_target[100];
     int rc_mnt;

	SYS2AP_MESSAGE sysmsg;

// The directories in /var/lock/hotplug should be ready or else it may disappear if hotplug comes at the same time with mkdir in rcS
	disk_name=disk_name2;
	wait_dir_ready(HOTPLUG_MOUNT_TMP, 100);
	wait_dir_ready(HOTPLUG_CONVERT_TMP, 100);
	wait_dir_ready("/sys/block", 100);

	devpath = getenv ("DEVPATH");
	if(!(ptr = strrchr(devpath, '/')))	// "/block/sda/sda1"
		return 1;
	ptr+=1;
	//printf("@@@ptr=%s\n", ptr);
	sprintf(lockfile, "%s/.lock_%s", HOTPLUG_MOUNT_TMP, ptr);
// Some devices, like mtdblock, won't have "PHYSDEVPATH" environment variable. Therefore, we would like to ignore them as soon as possible.
	if(parse_config(HOTPLUG_CONFIG_FILE, "ignore", ptr, 0) > 0) {
		printf("ptr=%s RETURN\n", ptr);
		return 1;
	}
	if(block_ptp)
		write_lockdata(lockfile, getenv("DEVPATH"), strlen(getenv("DEVPATH")));
	else
		write_lockdata(lockfile, getenv("PHYSDEVPATH"), strlen(getenv("PHYSDEVPATH")));

// Delete all the tmp or lock files
        sprintf(del_filename, "%s/lock_%s", HOTPLUG_MOUNT_TMP, ptr);
        if ( !(open(del_filename, O_RDONLY)) )
              if ( remove(del_filename)==-1 )
     	          printf ("block_add(): remove %s Error, errno=%s\n", del_filename, strerror(errno));
     	
        sprintf(del_filename, "%s/.d_%s", HOTPLUG_MOUNT_TMP, ptr);
        if ( !(open(del_filename, O_RDONLY)) )
             if ( remove(del_filename)==-1 )
                  printf ("block_add(): remove %s Error, errno=%s\n", del_filename, strerror(errno));     	

        sprintf(del_filename, "%s/.%s", HOTPLUG_MOUNT_TMP, ptr);
        if ( !(open(del_filename, O_RDONLY)) )
             if ( rmdir(del_filename)==-1 )
                   printf ("block_add(): rmdir %s Error, errno=%s\n", del_filename, strerror(errno));     	          	
	
	if(block_ptp) {
		sprintf(name, "ptp:%s", devpath);
	} else {
// When there is no device filesystem, we still need to know "name2" to parse_config.
		ret = device_name_translating(name, 256, name2, 64);
		if(ret < 0)
			return ret;
#ifdef NO_DEVICE_FILESYSTEM
		sprintf(name, "/dev/%s", ptr);
#endif
		if ( !strncmp(ptr, "sr", 2) ){	//Ken(20081113): for usb CDROM or DVD
			sysmsg.m_type = SYS2AP_MSG_USBCDROM;
			sprintf(sysmsg.m_msg, "%s UP %s", ptr, name);
			printf("sysmsg.m_msg=%s\n", sysmsg.m_msg);
			RTK_SYS2AP_SendMsg(&sysmsg);
		}
	}

	while(parse_config(HOTPLUG_CONFIG_FILE, "delay", ptr, 0) > 0) sleep(1);
	if(!block_ptp && parse_config(HOTPLUG_CONFIG_FILE, "convert", ptr, 0) > 0) {
	  if(strlen(ptr)>3)
		sprintf(open_filename, "%s/%s", HOTPLUG_CONVERT_TMP, ptr+3);
	  else
		sprintf(open_filename, "%s/0", HOTPLUG_CONVERT_TMP);
          
          if ( (fd = open(open_filename, O_RDWR))==-1 ){
          	printf ("block_add(): OPEN %s Error, errno=%s\n", open_filename, strerror(errno));
                return -1;
          }

          if ( write(fd, name, strlen(name))!= strlen(name) ){
                printf ("block_add(): WRITE %s Error, errno=%s\n", open_filename, strerror(errno));
	        close(fd);
	        return -1;
          }	  
          close(fd);
          return 1;
	}
	
	if(!block_ptp && parse_config(HOTPLUG_CONFIG_FILE, "convert", name2, 0) == 1) {
          sprintf(open_filename, "%s/name", HOTPLUG_CONVERT_TMP);
          if ( (fd = open(open_filename, O_RDWR))==-1 ){
          	printf ("block_add(): OPEN %s Error, errno=%s\n", open_filename, strerror(errno));
                return -1;
          }
          if ( write(fd, ptr, strlen(ptr)) != strlen(ptr)){
               printf ("block_add(): WRITE %s Error, errno=%s\n", open_filename, strerror(errno));
	       close(fd);
	       return -1;
          }
          close(fd);
          
	  if(strlen(ptr)>3)
		sprintf(open_filename, "%s/%s", HOTPLUG_CONVERT_TMP, ptr+3);
	  else
		sprintf(open_filename, "%s/0", HOTPLUG_CONVERT_TMP);

          if ( (fd = open(open_filename, O_RDWR))==-1 ){
          	printf ("block_add(): OPEN %s Error, errno=%s\n", open_filename, strerror(errno));
               return -1;
          }
          if ( write(fd, name, strlen(name)) != strlen(name) ){
               printf ("block_add(): WRITE %s Error, errno=%s\n", open_filename, strerror(errno));
	       close(fd);
	       return -1;
          }			
	  close(fd);
	  return 1;	          
	}

	if(!block_ptp && parse_config(HOTPLUG_CONFIG_FILE, "rename", name2, disk_name) == 1) {
		if(strlen(ptr)>3) {
			strcat(disk_name, "_");
			strcat(disk_name, ptr+3);
		}
	} else {
//		strcpy(disk_name, ptr);
		disk_name = NULL;
	}
// The disc has partition. Therefore, we skip mounting, for example, sda.
	if(!block_ptp && ret == 1)
		return 1;
	
      sprintf(open_filename, "%s/.%s", HOTPLUG_MOUNT_TMP, ptr);
	 if ( (mkdir(open_filename, 0755))==-1 ){
			printf ("block_add(): mkdir %s Error, errno=%s\n", open_filename, strerror(errno));
			printf("Hotplug: Mkdir \"%s/.%s\" fail.\n", HOTPLUG_MOUNT_TMP, ptr);
		     add_partition(ptr, name, disk_name);
			return 1;
	 }

	if(block_ptp) {
		//sprintf(command, "mount -t ptpfs %s %s/.%s", ptr, HOTPLUG_MOUNT_TMP, ptr);
		sprintf(mnt_source, "%s", ptr);
		sprintf(mnt_target, "%s/.%s", HOTPLUG_MOUNT_TMP, ptr);	
		if((ret=do_command2("mount", mnt_source, mnt_target, "ptpfs", 0, NULL, COMMAND_TRY, ptr)) == -1) {
			printf("Hotplug: Mount \"%s\" fail.\n", ptr);
			add_partition(ptr, name, disk_name);
			return 1;
		}
	} else {
		sprintf(mnt_source, "%s", name);
		sprintf(mnt_target, "%s/.%s", HOTPLUG_MOUNT_TMP, ptr);
		if ( !strncmp(ptr, "sr", 2) ){
#if 0
			if((ret=do_command2("mount", mnt_source, mnt_target, "udf", MS_RDONLY, NULL, COMMAND_TRY, ptr)) == 0)	
				printf("Hotplug: Mount \"%s\" udf successfully.\n", ptr);
			else if((ret=do_command2("mount", mnt_source, mnt_target, "iso9660", MS_RDONLY, NULL, COMMAND_TRY, ptr)) == 0)
				printf("Hotplug: Mount \"%s\" iso9660 successfully.\n", ptr);
			else if((ret=do_command2("mount", mnt_source, mnt_target, "vcd", MS_RDONLY, NULL, COMMAND_TRY, ptr)) == 0)
				printf("Hotplug: Mount \"%s\" vcd successfully.\n", ptr);		
			else{
				add_partition(ptr, name, disk_name);	
				return 1;
			}	
#endif
		}else{
			if((ret=do_command2("mount", mnt_source, mnt_target, "vfat", MS_MGC_VAL, "shortname=winnt,utf8", COMMAND_TRY, ptr)) == 0)
				printf("Hotplug: Mount \"%s\" vfat successfully.\n", ptr);
// Why we still need to retry this command? Because I found that sometimes the first "mount ntfs" will fail alought there is already delay when mounting VFAT.
// I think this is because when "mount sda" and "mount sda1" happen at the same time, it will have problem.
			else if((ret=do_command2("mount", mnt_source, mnt_target, "ntfs", MS_RDONLY, "nls=utf8", COMMAND_TRY, ptr)) == 0)
				printf("Hotplug: Mount \"%s\" ntfs successfully.\n", ptr);
			else if((ret=do_command2("mount", mnt_source, mnt_target, "ufsd", MS_RDONLY, "nls=utf8", COMMAND_TRY, ptr)) == 0)
				printf("Hotplug: Mount \"%s\" ufsd successfully.\n", ptr);
			else if((ret=do_command2("mount", mnt_source, mnt_target, "ext3", MS_RDONLY, NULL, COMMAND_TRY, ptr)) == 0)
				printf("Hotplug: Mount \"%s\" ext3 successfully.\n", ptr);
			else{
				add_partition(ptr, name, disk_name);	
				return 1;
			}	
		}
	}

	sprintf(name2, "%s/.d_%s", HOTPLUG_MOUNT_TMP, ptr);
	do_create_empty_file(name2);
      sprintf(open_filename, "/mnt/usbmounts/%s", ptr);
	 if ( (mkdir(open_filename, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))==-1 ){
			printf ("block_add(): mkdir %s Error, errno=%s\n", open_filename, strerror(errno));
			printf("Hotplug: Mkdir \"/mnt/usbmounts/%s\" fail.\n", ptr);
		     add_partition(ptr, name, disk_name);
			return 1;
	 }

	sprintf(mnt_source, "%s/.%s", HOTPLUG_MOUNT_TMP, ptr);
	sprintf(mnt_target, "/mnt/usbmounts/%s", ptr);	
	//printf("@@@mnt_source=%s, mnt_target=%s\n", mnt_source, mnt_target);

	if(block_ptp){
          rc_mnt = mount(mnt_source, mnt_target, "ptpfs", MS_MOVE, NULL);
		//sprintf(command, "mount -t ptpfs --move %s/.%s /mnt/usbmounts/%s", HOTPLUG_MOUNT_TMP, ptr, ptr);
	}else{
		rc_mnt = mount(mnt_source, mnt_target, NULL, MS_MOVE, NULL);
		//sprintf(command, "mount --move %s/.%s /mnt/usbmounts/%s", HOTPLUG_MOUNT_TMP, ptr, ptr);
	}
	
	//printf("rc_mnt=%d\n", rc_mnt);
	if(rc_mnt==-1){
		sprintf(open_filename, "/mnt/usbmounts/%s", ptr);
		if ( rmdir(open_filename) == -1 )
			 printf ("block_add(): rmdir %s Error, errno=%s\n", open_filename, strerror(errno));	
		if ( remove(name2) == -1 )
			 printf ("block_add(): remove %s Error, errno=%s\n", name2, strerror(errno));
		printf("Hotplug: Mount --move \"/mnt/usbmounts/%s\" fail.\n", ptr);
		add_partition(ptr, name, disk_name);
		return 1;
	}

	if ( remove(name2) == -1 )                                 // This lock file may be left for a long time.
		 printf ("block_add(): remove %s Error, errno=%s\n", open_filename, strerror(errno));

	if(block_ptp)
		add_partition(ptr, name, disk_name);
	else
		add_partition(ptr, NULL, disk_name);

	printf("Hotplug: Mount \"%s\" successfully.\n", ptr);

	signal_pid(DVDPLAYER_LOCK, BLOCK_ADD_SIGNAL_NUMBER);
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

	char open_filename[100], mnt_target[100];
	SYS2AP_MESSAGE sysmsg;
	
	devpath = getenv ("DEVPATH");
	if(!(ptr = strrchr(devpath, '/')))	// "/block/sda/sda1"
		return 1;
	
	ptr+=1;
	//printf("###ptr=%s\n", ptr);
	sprintf(lockfile, "%s/.lock_%s", HOTPLUG_MOUNT_TMP, ptr);

	//Ken(20081113): for usb CDROM or DVD
	if ( !strncmp(ptr, "sr", 2) ){	
		sysmsg.m_type = SYS2AP_MSG_USBCDROM;
		sprintf(sysmsg.m_msg, "%s DOWN", ptr);
		printf("sysmsg.m_msg=%s\n", sysmsg.m_msg);
		RTK_SYS2AP_SendMsg(&sysmsg);
	}
	
// We cannot use this function at this time because the sys directory of the device may disappear immediately when it is unplugged.
// However, we can use this on card reader which always has sys directory.
//	ret = device_name_translating(name, 256, name2, 32);
// HOTPLUG_RENAME_TMP directory stores the files that have been renamed. Use them instead of "device_name_translating"
/*	sprintf(name, "%s/%s", HOTPLUG_RENAME_TMP, ptr);
	if(!stat(name, &st) && S_ISREG(st.st_mode)) {
		fd = fopen(name, "rt");
		if(!fd) {
			unlink(name);
			del_partition(ptr);
			printf("Hotplug: Open file \"%s\" fail.\n", name);
			return 1;
		}
		if(!fgets(disk_name, 16, fd)) {
			fclose(fd);
			unlink(name);
			del_partition(ptr);
			printf("Hotplug: Read file \"%s\" fail.\n", name);
			return 1;
		}
		fclose(fd);
		unlink(name);
	} else
		strcpy(disk_name, ptr);
*/
	if(parse_config(HOTPLUG_CONFIG_FILE, "ignore", ptr, 0) > 0) {
//		if(block_ptp)
//			testclear_lockdata(lockfile, getenv("DEVPATH"), strlen(getenv("DEVPATH")));
//		else
//			testclear_lockdata(lockfile, getenv("PHYSDEVPATH"), strlen(getenv("PHYSDEVPATH")));
		return 1;
	}
	if(!block_ptp && parse_config(HOTPLUG_CONFIG_FILE, "convert", ptr, 0) > 0) {
		if(strlen(ptr)>3) {
			sprintf(open_filename, "%s/%s", HOTPLUG_CONVERT_TMP, ptr+3);
			printf("Hotplug: Delete convert file \"%s/%s\".\n", HOTPLUG_CONVERT_TMP, ptr+3);
		} else {
			sprintf(open_filename, "%s/0", HOTPLUG_CONVERT_TMP);
			printf("Hotplug: Delete convert file \"%s/0\".\n", HOTPLUG_CONVERT_TMP);
		}
		if ( remove(open_filename) == -1 )
			 printf ("block_remove(): remove %s Error, errno=%s\n", open_filename, strerror(errno));			 
			 
		if(block_ptp)
			testclear_lockdata(lockfile, getenv("DEVPATH"), strlen(getenv("DEVPATH")));
		else
			testclear_lockdata(lockfile, getenv("PHYSDEVPATH"), strlen(getenv("PHYSDEVPATH")));
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
	sprintf(name, "/mnt/usbmounts/%s", ptr);
	//printf("name=%s, stat(name, &st)=%d, S_ISDIR(st.st_mode)=%d\n", name, stat(name, &st), S_ISDIR(st.st_mode));
	if(stat(name, &st)<0 || !S_ISDIR(st.st_mode)) {
		printf("Hotplug: Cannot find \"%s\".\n", name);
		del_partition(ptr);
		if(block_ptp)
			testclear_lockdata(lockfile, getenv("DEVPATH"), strlen(getenv("DEVPATH")));
		else
			testclear_lockdata(lockfile, getenv("PHYSDEVPATH"), strlen(getenv("PHYSDEVPATH")));
          
          if ( remove(lockfile)==-1 )
                 printf ("block_remove(): remove %s Error, errno=%s\n", lockfile, strerror(errno));   
		
		return 1;
	}

	// Lock file to notify that the corresponding directory cannot be accessed
	sprintf(name, "%s/.d_%s", HOTPLUG_MOUNT_TMP, ptr);
	do_create_empty_file(name);

	del_partition(ptr);
	signal_pid(DVDPLAYER_LOCK, BLOCK_REMOVE_SIGNAL_NUMBER);

	//sprintf(command, "umount -f -l /mnt/usbmounts/%s", ptr);
	sprintf(mnt_target, "/mnt/usbmounts/%s", ptr);
	//if(do_command(command, UMOUNT_COMMAND_TRY, ptr))
	/*MNT_FORCE=0x00000001, MNT_DETACH=0x00000002*/
	if((ret=do_command2("umount2", NULL, mnt_target, NULL, 0x00000001|0x00000002, NULL, COMMAND_TRY, ptr)) == -1){
		ret = 1;
	}else {
		sprintf(open_filename, "/mnt/usbmounts/%s", ptr);
		if ( rmdir(open_filename) == -1 )
			 printf ("block_remove(): rmdir %s Error, errno=%s\n", open_filename, strerror(errno));	
		if ( remove(name) == -1 )
			 printf ("block_remove(): remove %s Error, errno=%s\n", open_filename, strerror(errno));			 

	     sprintf(open_filename, "%s/.%s", HOTPLUG_MOUNT_TMP, ptr);
          if ( rmdir(open_filename)==-1 )
                printf ("block_remove(): rmdir %s Error, errno=%s\n", open_filename, strerror(errno));     	          	
	}
	
                  	
	printf("Hotplug: Umount %s successfully.\n", ptr);

	if(block_ptp)
		testclear_lockdata(lockfile, getenv("DEVPATH"), strlen(getenv("DEVPATH")));
	else
		testclear_lockdata(lockfile, getenv("PHYSDEVPATH"), strlen(getenv("PHYSDEVPATH")));
	
	
     if ( remove(lockfile)==-1 )
          printf ("block_remove(): remove %s Error, errno=%s\n", lockfile, strerror(errno));     	
	
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


