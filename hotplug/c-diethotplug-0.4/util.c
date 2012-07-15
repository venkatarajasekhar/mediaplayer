/*
 * util.c
 *
 * Simple utility functions needed by some of the subsystems.
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
#include <string.h>
#include <stdlib.h>	/* for exit() */
#include <unistd.h>
#include <stdio.h>
#include "hotplug.h"
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/**
 * split_2values
 *
 * takes a string of format "xxxx:yyyy" and figures out the
 * values for xxxx and yyyy
 * 
 */
int split_2values (const char *string, int base, unsigned int *value1, unsigned int *value2)
{
	char buffer[200];
	char *temp1;
	const char *temp2;
	
	dbg("string = %s", string);

	if (string == NULL)
		return -1;
	/* dietLibc doesn't have strnlen yet :( */
	/* if (strnlen (string, sizeof(buffer)) >= sizeof(buffer)) */
	if (strlen (string) >= sizeof(buffer))
		return -1;

	/* pick up the first number */
	temp1 = &buffer[0];
	temp2 = string;
	while (1) {
		if (*temp2 == 0x00)
			break;
		if (*temp2 == ':')
			break;
		*temp1 = *temp2;
		++temp1;
		++temp2;
	}
	*temp1 = 0x00;
	*value1 = strtoul (buffer, NULL, base);
	dbg ("buffer = %s", &buffer[0]);
	dbg ("value1 = %d", *value1);

	if (*temp2 == 0x00) {
		/* string is ended, not good */
		return -1;
	}

	/* get the second number */
	temp1 = &buffer[0];
	++temp2;
	while (1) {
		if (*temp2 == 0x00)
			break;
		*temp1 = *temp2;
		++temp1;
		++temp2;
	}
	*temp1 = 0x00;
	*value2 = strtoul (buffer, NULL, base);
	dbg ("buffer = %s", &buffer[0]);
	dbg ("value2 = %d", *value2);

	return 0;
}


/**
 * split_3values
 *
 * takes a string of format "xxxx/yyyy/zzzz" and figures out the
 * values for xxxx, yyyy, and zzzz
 * 
 */
int split_3values (const char *string, int base, unsigned int * value1, unsigned int * value2, unsigned int * value3)
{
	char buffer[200];
	char *temp1;
	const char *temp2;
	
	dbg("string = %s", string);

	if (string == NULL)
		return -1;
	/* dietLibc doesn't have strnlen yet :( */
	/* if (strnlen (string, sizeof(buffer)) >= sizeof(buffer)) */
	if (strlen (string) >= sizeof(buffer))
		return -1;

	/* pick up the first number */
	temp1 = &buffer[0];
	temp2 = string;
	while (1) {
		if (*temp2 == 0x00)
			break;
		if (*temp2 == '/')
			break;
		*temp1 = *temp2;
		++temp1;
		++temp2;
	}
	*temp1 = 0x00;
	*value1 = strtoul (buffer, NULL, base);
	dbg ("buffer = %s", &buffer[0]);
	dbg ("value1 = %d", *value1);

	if (*temp2 == 0x00) {
		/* string is ended, not good */
		return -1;
	}

	/* get the second number */
	temp1 = &buffer[0];
	++temp2;
	while (1) {
		if (*temp2 == 0x00)
			break;
		if (*temp2 == '/')
			break;
		*temp1 = *temp2;
		++temp1;
		++temp2;
	}
	*temp1 = 0x00;
	*value2 = strtoul (buffer, NULL, base);
	dbg ("buffer = %s", &buffer[0]);
	dbg ("value2 = %d", *value2);

	if (*temp2 == 0x00) {
		/* string is ended, not good */
		return -1;
	}

	/* get the third number */
	temp1 = &buffer[0];
	++temp2;
	while (1) {
		if (*temp2 == 0x00)
			break;
		*temp1 = *temp2;
		++temp1;
		++temp2;
	}
	*temp1 = 0x00;
	*value3 = strtoul (buffer, NULL, base);
	dbg ("buffer = %s", &buffer[0]);
	dbg ("value3 = %d", *value3);

	return 0;
}


int call_subsystem (const char *string, struct subsystem *subsystem)
{
	int i;

	for (i = 0; subsystem[i].name != NULL; ++i) {
		if (strncmp (string, subsystem[i].name, strlen(string)) == 0)
			return subsystem[i].handler();
	}
	return 1;
}


int load_module (const char *module_name)
{
	char *argv[3];

	argv[0] = "/sbin/modprobe";
	argv[1] = (char *)module_name;
	argv[2] = NULL;
	dbg ("loading module %s", module_name);
	switch (fork()) {
		case 0:
			/* we are the child, so lets run the program */
			execv ("/sbin/modprobe", argv);
			exit(0);
			break;
		case (-1):
			dbg ("fork failed.");
			break;
		default:
			break;
	}
	return 0;
}

int signal_pid(const char *file, int signal_num)
{
	FILE *f;
	char *ptr;
	char num[20];
	
	f = fopen(file, "rt");
	if(!f)
		return 1;
	ptr = fgets(num, 20, f);
	if(!ptr) {
		fclose(f);
		return 1;
	}
	if(kill(atoi(num), signal_num)) {
		fclose(f);
		return 1;
	}
	fclose(f);
	return 0;
}

/* To parse the lock or config file.
   For example, parse_config("file", "delay", "hda1", char **value) will:
   	1. return 2 if match with '*'. Ex: "delay:*"
   	2. return 1 if match the whole token. Ex: "delay:hda1"
   	3. return 3 if match partial token. Ex: "delay:hda"
   	4. return 0 if not match
   	5. return -1 if the format is wrong
   The maximum length of value is 64.
   	
   */
int parse_config(const char *file, const char *name, const char *target, char *value)
{
	FILE *f;
	char *ptr1, *ptr2, *ptr3=NULL, *ptr4=NULL, *brk_str;
	char line[100]; 
	char *sep=": \t\n";

	if(!file || !name || !target) {
		return -1;
	}

	f = fopen(file, "rt");
	if(!f) {
		return 0;
	}
	while(fgets(line, 100, f)) {
		if((ptr1 = strtok_r(line, sep, &brk_str))) {
			if(ptr1[0] == '#')
				continue;
			if((ptr2 = strtok_r(NULL, sep, &brk_str)))
				if((ptr3 = strtok_r(NULL, sep, &brk_str)))
					ptr4 = strtok_r(NULL, sep, &brk_str);
			if(ptr4)
				return -1;
			if(!strcasecmp(ptr1, name) && ptr2) {
// Decide if the value is necessary. For example, the value of "redirect:sda1:rw,/tmp/aaa" is "rw,/tmp/aaa"
				if((value && !ptr3) || (!value && ptr3))
					return -1;
				if(ptr3) {
// The maximum length of value is 64.
					if(strlen(ptr3) > 63)
						return -1;
					strcpy(value, ptr3);
				}
// Match the whole token
// Exp: parse_config("file", "delay", "hda1", char *value) found "delay:hda1"
				if(!strcasecmp(ptr2, target)) {
					fclose(f);
					return 1;
				}
// Match the whole token with '*'
// Exp: parse_config("file", "delay", "hda1", char *value) found "delay:*"
				if(!strcasecmp(ptr2, "*")) {
					fclose(f);
					return 2;
				}
// Match partial token
// Exp: parse_config("file", "delay", "hda1", char *value) found "delay:hda"
				if(!strncasecmp(ptr2, target, strlen(ptr2))) {
					fclose(f);
					return 3;
				}
			}
		}
	}

	fclose(f);
	return 0;
}

void wait_dir_ready(const char *name, int times)
{
	struct stat st;
	int i=0;
	char mesg[128];

	while(1) {
		if(!stat(name, &st))
			if(S_ISDIR(st.st_mode))
				break;
		i++;
		usleep(300000);
		if(i > times) {
			sprintf(mesg, "Dir %s is not ready for %d*0.3 seconds.", name, times);
//			dbg (mesg);
			break;
		}
	}
}

/*
	Write data to a lock file. Writing will be blocked if the file is not empty.
	Lock file will be created if it doesn't exist.
		filename: lock file name
		data: written data
		len: the length of the written data
 */
int write_lockdata(const char *filename, const char *data, int len)
{
	int fd;
	struct stat st;

	if(!filename || !data || len<0) {
		puts("write_lockdata parameter error.");
		return -1;
	}
	fd = open(filename, O_CREAT | O_WRONLY, S_IRWXU | S_IRGRP | S_IROTH);
	if(fd<0) {
		puts("write_lockdata open lock file error.");
		return -1;
	}

	while(1) {
		if(lseek(fd, 0, SEEK_SET)!=0) goto WRITE_LOCKDATA_ERR;
		if(lockf(fd, F_LOCK, 0)<0) goto WRITE_LOCKDATA_ERR;
		if(fstat(fd, &st)<0) goto WRITE_LOCKDATA_ERR;
		if(!st.st_size)
			break;
		if(lseek(fd, 0, SEEK_SET)!=0) goto WRITE_LOCKDATA_ERR;
		if(lockf(fd, F_ULOCK, 0)<0) goto WRITE_LOCKDATA_ERR;
		usleep(10);
	}
	if(write(fd, data, len)<0) goto WRITE_LOCKDATA_ERR;
	if(lseek(fd, 0, SEEK_SET)!=0) goto WRITE_LOCKDATA_ERR;
	if(lockf(fd, F_ULOCK, 0)<0) goto WRITE_LOCKDATA_ERR;
	close(fd);

	return 0;

WRITE_LOCKDATA_ERR:
	puts("write_lockdata error.");
	close(fd);
	return -1;
}

/*
	Read data from a lock file.
		filename: lock file name
		data: buffer
		len: the length of the buffer
 */
int read_lockdata(const char *filename, char *data, int *len)
{
	int fd;

	if(!filename || !data || *len<0) {
		puts("read_lockdata parameter error.");
		return -1;
	}
	fd = open(filename, O_RDWR);	// Cannot use O_RDONLY if using lockf
	if(fd<0) {
		puts("read_lockdata open lock file error.");
		*len = 0;
		return -1;
	}
	if(lseek(fd, 0, SEEK_SET)!=0) goto READ_LOCKDATA_ERR;
	if(lockf(fd, F_LOCK, 0)<0) goto READ_LOCKDATA_ERR;
	if((*len = read(fd, data, *len))<0) goto READ_LOCKDATA_ERR;
	if(lseek(fd, 0, SEEK_SET)!=0) goto READ_LOCKDATA_ERR;
	if(lockf(fd, F_ULOCK, 0)<0) goto READ_LOCKDATA_ERR;
	close(fd);

	return 0;

READ_LOCKDATA_ERR:
	puts("read_lockdata error.");
	close(fd);
	return -1;
}


/*
	Clear the content of the lockfile "if" the content equals the given key
		filename: lock file name
		data: the key to be compared
		len: the length of the key
 */
int testclear_lockdata(const char *filename, const char *data, int len)
{
	int fd;
	char *buf;
	int ret;

	if(!filename || !data || len<0) {
		puts("testclear_lockdata parameter error.");
		return -1;
	}
	fd = open(filename, O_RDWR);	// Cannot use O_RDONLY if using lockf
	if(fd<0) {
		puts("testclear_lockdata open lock file error.");
		return -1;
	}
	buf = malloc(len);
	while(1) {
		if(lseek(fd, 0, SEEK_SET)!=0) goto TESTCLEAR_LOCKDATA_ERR;
		if(lockf(fd, F_LOCK, 0)<0) goto TESTCLEAR_LOCKDATA_ERR;
		if((ret = read(fd, buf, len))<0) goto TESTCLEAR_LOCKDATA_ERR;
		if(lseek(fd, 0, SEEK_SET)!=0) goto TESTCLEAR_LOCKDATA_ERR;
		if(ret == len && !strncmp(data, buf, len)) {
			if(ftruncate(fd, 0)<0) goto TESTCLEAR_LOCKDATA_ERR;
			if(lockf(fd, F_ULOCK, 0)<0) goto TESTCLEAR_LOCKDATA_ERR;
			break;
		}
		if(lockf(fd, F_ULOCK, 0)<0) goto TESTCLEAR_LOCKDATA_ERR;
		usleep(10);
	}
	close(fd);
	free(buf);

	return 0;

TESTCLEAR_LOCKDATA_ERR:
	puts("testclear_lockdata error.");
	free(buf);
	close(fd);
	return -1;
}

