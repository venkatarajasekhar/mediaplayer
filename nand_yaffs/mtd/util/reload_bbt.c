/******************************************************************************
 * $Id: reload_bbt.c 161767 2009-01-23 07:25:17Z ken.yu $
 * Overview: reload BBT
 * Copyright (c) 2008 Realtek Semiconductor Corp. All Rights Reserved.
 * Modification History:
 *    #000 2009-02-10 Ken-Yu   create file
 *
 *******************************************************************************/
#define _GNU_SOURCE
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <getopt.h>
#include <errno.h>

#include <asm/types.h>
#include "mtd/mtd-user.h"

//char 	*mtd_device = "/dev/mtd/0";

/*
 * Main program
 */
int main(int argc, char **argv)
{
	int fd;

	if(argc!=2)
	{
	printf("Error!!\nCommand note:reload_bbt /dev/mtd/#\n");
	return 1;
	}
	/* Open the device */
	printf("reload_bbt on %s\n",argv[1]);
	if ((fd = open(argv[1], O_RDWR)) == -1) {
		perror("open flash");
		exit(1);
	}

	if (ioctl(fd, MEMRELOADBBT, 0) != 0) {
		perror("MEMRELOADBBT");
		close(fd);
		exit(1);
	}
	
	if ( fd > 0 )
		close(fd);
	/* Return happy */
	return 0;
}
