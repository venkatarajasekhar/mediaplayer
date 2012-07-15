/*
 *  nanddump.c
 *
 *  Copyright (C) 2000 David Woodhouse (dwmw2@infradead.org)
 *                     Steven J. Hill (sjhill@realitydiluted.com)
 *
 * $Id: nanddump.c,v 1.28 2005/03/17 12:10:30 gleixner Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  Overview:
 *   This utility dumps the contents of raw NAND chips or NAND
 *   chips contained in DoC devices.
 
* Modification History:
*    #000 2008-10-06 Ken-Yu  add ioctl( , MEMREADDATAOOB, ) to do page read one time 
											instead of read OOB and DATA 2 times.
*    #001 2008-12-22 Ken-Yu   support 4K page size
*/

#define _GNU_SOURCE
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <asm/types.h>
#include <mtd/mtd-user.h>

#define PROGRAM "nanddump"
#define VERSION "$Revision: 1.28 $"

#define RTK_PU_DATA	512	//realtek nand process Unit 
#define RTK_PU_OOB	16	//realtek nand process Unit 

void display_help (void)
{
	printf("Usage: nanddump [OPTIONS] MTD-device\n"
	       "Dumps the contents of a nand mtd partition.\n"
	       "\n"
	       "           --help     	        display this help and exit\n"
	       "           --version  	        output version information and exit\n"
	       "-f file    --file=file          dump to file\n"
	       "-i         --ignoreerrors       ignore errors\n"
	       "-k         --packed             output packed file\n"
	       "-l length  --length=length      length\n"
	       "-o         --omitoob            omit oob data\n"
	       "-b         --omitbad            omit bad blocks from the dump\n"
	       "-p         --prettyprint        print nice (hexdump)\n"
	       "-r         --realtek        print realtek nand data layout\n"
	       "-t         --tune        tune performance for variable buffer size\n"	       
	       "-s addr    --startaddress=addr  start address\n");
	exit(0);
}

void display_version (void)
{
	printf(PROGRAM " " VERSION "\n"
	       "\n"
	       PROGRAM " comes with NO WARRANTY\n"
	       "to the extent permitted by law.\n"
	       "\n"
	       "You may redistribute copies of " PROGRAM "\n"
	       "under the terms of the GNU General Public Licence.\n"
	       "See the file `COPYING' for more information.\n");
	exit(0);
}

// Option variables

int 	ignoreerrors;		// ignore errors
int 	pretty_print;		// print nice in ascii
int 	omitoob;		// omit oob data
unsigned long	start_addr;	// start address
unsigned long	length;		// dump length
char    *mtddev;		// mtd device name
char    *dumpfile;		// dump file name
int	omitbad;
int	packed;
int	realtek;
int tune_buffer_data = 0;
int tune_buffer_oob = 0;

// oob layouts to pass into the kernel as default
struct nand_oobinfo rtk_oobinfo = { 
	.useecc = MTD_ECC_RTK_HW,
};

struct nand_oobinfo none_oobinfo = { 
	.useecc = MTD_ECC_NONE,
};

void process_options (int argc, char *argv[])
{
	int error = 0;

	for (;;) {
		int option_index = 0;
		static const char *short_options = "bs:f:il:opkrt:";
		static const struct option long_options[] = {
			{"help", no_argument, 0, 0},
			{"version", no_argument, 0, 0},
			{"ignoreerrors", no_argument, 0, 'i'},
			{"prettyprint", no_argument, 0, 'p'},
			{"omitoob", no_argument, 0, 'o'},
			{"omitbad", no_argument, 0, 'b'},
			{"startaddress", required_argument, 0, 's'},
			{"length", required_argument, 0, 'l'},
			{"packed", no_argument, 0, 'k'},
			{"realtek", no_argument, 0, 'r'},	//CMYu, 20090519
			{"tune_buffer_data", required_argument, 0, 't'},	//CMYu, 20091012
			{0, 0, 0, 0},
		};

		int c = getopt_long(argc, argv, short_options,
				    long_options, &option_index);
		if (c == EOF) {
			break;
		}

		switch (c) {
		case 0:
			switch (option_index) {
			case 0:
				display_help();
				break;
			case 1:
				display_version();
				break;
			}
			break;
		case 'b':
			omitbad = 1;
			break;
		case 's':
			start_addr = atol(optarg);
			break;
		case 'f':
			if (!(dumpfile = strdup(optarg))) {
				perror("stddup");
				exit(1);
			}
			break;
		case 'i':
			ignoreerrors = 1;
			break;
		case 'k':
			packed = 1;
			break;
		case 'l':
			length = atol(optarg);
			break;
		case 'o':
			omitoob = 1;
			break;
		case 'p':
			pretty_print = 1;
			break;
		case 'r':
			realtek = 1;
			break;
		case 't':
			tune_buffer_data = atol(optarg);
			break;			
		case '?':
			error = 1;
			break;
		}
	}
	
	if ((argc - optind) != 1 || error) 
		display_help ();
	
	mtddev = argv[optind];
}


//Ken, 20081222
unsigned int pagesize = 2048, oobsize = 64;

static void reverse_to_Yaffs2Tags(char *r_oobbuf)
{
	int k;
	for ( k=0; k<4; k++ ){
		r_oobbuf[k]  = r_oobbuf[1+k];
	}

	for ( k=0; k<4; k++ ){
		r_oobbuf[4+k] = r_oobbuf[16+k];
		r_oobbuf[8+k] = r_oobbuf[32+k];
		r_oobbuf[12+k] = r_oobbuf[48+k];
	}
}


/*
 * Main program
 */
int main(int argc, char **argv)
{
	unsigned long ofs, end_addr = 0;
	unsigned long long blockstart = 1;
	int i, j, fd, ofd, bs, badblock = 0;
	//struct mtd_oob_buf oob = {0, 16, oobbuf};
	mtd_info_t meminfo;
	char pretty_buf[80];

	struct mtd_data_oob DataOobLocal;

	int rtk_pcnt;
	int m;
	
	int index = 0;
	
	//Ken, 20081222	
/*
 * Buffers for reading data from flash
 */
	unsigned char *readbuf;
	unsigned char *oobbuf;
	unsigned char *oobpackedbuf;

	unsigned char *t_totalbuf;
	int process_cnt = 0;
	int process_cnt_mode = 0;
		
 	process_options(argc, argv);

	//CMYu, 20091013
	if ( tune_buffer_data){
		if (length%tune_buffer_data){
			printf("ERROR: length is not multiple of buffer size\n");
			exit (1);
		}
	}

	/* Open MTD device */
	if ((fd = open(mtddev, O_RDWR)) == -1) {
		perror("open flash");
		exit (1);
	}

	/* Fill in MTD device capability structure */   
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		perror("MEMGETINFO");
		close(fd);
		exit (1);
	}

	/* Ken, 20081222: Make sure device page sizes are valid */
	if ( meminfo.oobblock&(512-1) || meminfo.oobsize&(16-1) ){
		fprintf(stderr, "Unknown flash (not normal NAND)\n");
		close(fd);
		exit(1);	
	}

	//CMYu, 20090910
	if (ioctl (fd, MEMSETOOBSEL, &rtk_oobinfo) != 0) {
		perror ("MEMSETOOBSEL");
		close(fd);
		exit(1);
	} 
	
	//printf("[%s] rtk_oobinfo.useecc=%d\n", __FUNCTION__, rtk_oobinfo.useecc);
	
#if 0
	/* Make sure device page sizes are valid */	
	if (!(meminfo.oobsize == 128 && meminfo.oobblock == 4096) &&
		!(meminfo.oobsize == 64 && meminfo.oobblock == 2048) &&
	    !(meminfo.oobsize == 16 && meminfo.oobblock == 512) &&
	    !(meminfo.oobsize == 8 && meminfo.oobblock == 256)) {
		fprintf(stderr, "Unknown flash (not normal NAND)\n");
		close(fd);
		exit(1);
	}
#endif
	
	//Ken, 20081222	
	pagesize = meminfo.oobblock;
	oobsize = meminfo.oobsize;

	if (( readbuf = malloc(pagesize)) == NULL ){
		printf("allocate readbuf fails\n");
		exit(1);
	}

	if (( oobbuf = malloc(oobsize)) == NULL ){
		printf("allocate oobbuf fails\n");
		free(readbuf);
		exit(1);
	}
			
	if (( oobpackedbuf = malloc(oobsize)) == NULL ){
		printf("allocate oobpackedbuf fails\n");
		free(readbuf);
		free(oobbuf);
		exit(1);
	}

	if ( !realtek && tune_buffer_data){
		tune_buffer_oob = (tune_buffer_data/pagesize)*oobsize;
		if (( t_totalbuf = malloc(tune_buffer_data+tune_buffer_oob)) == NULL ){
			printf("allocate t_totalbuf fails\n");
			free(readbuf);
			free(oobbuf);
			exit(1);
		}
		process_cnt_mode = tune_buffer_data/pagesize;
		//printf("process_cnt_mode=%d\n", process_cnt_mode);
	}

	/* Read the real oob length */
	DataOobLocal.rtk_oob.length = meminfo.oobsize;

	/* Open output file for writing. If file name is "-", write to standard output. */
	if (!dumpfile) {
		ofd = STDOUT_FILENO;
	} else if ((ofd = open(dumpfile, O_WRONLY | O_TRUNC | O_CREAT, 0644)) == -1) {
		perror ("open outfile");
		free(readbuf);
		free(oobbuf);
		free(oobpackedbuf);
		if ( !realtek && tune_buffer_data)
			free(t_totalbuf);
		close(fd);
		exit(1);
	}

	/* Initialize start/end addresses and block size */
	if (length)
		end_addr = start_addr + length;
	if (!length || end_addr > meminfo.size)
 		end_addr = meminfo.size;

	bs = meminfo.oobblock;

	/* Print informative message */
	fprintf(stderr, "Block size %u, page size %u, OOB size %u\n", meminfo.erasesize, meminfo.oobblock, meminfo.oobsize);
	fprintf(stderr, "Dumping data starting at 0x%08x and ending at 0x%08x...\n",
	        (unsigned int) start_addr, (unsigned int) end_addr);

	/* Dump the flash contents */
	for (ofs = start_addr; ofs < end_addr ; ofs+=bs) {
		// new eraseblock , check for bad block
		if (blockstart != (ofs & (~meminfo.erasesize + 1))) {
			blockstart = ofs & (~meminfo.erasesize + 1);
			if ((badblock = ioctl(fd, MEMGETBADBLOCK, &blockstart)) < 0) {
				perror("ioctl(MEMGETBADBLOCK)");
				goto closeall;
			}
		}

		if (badblock) {
			if (omitbad)
				continue;
			memset (readbuf, 0xff, bs);
		} else {
			/* Read page data and exit on failure */
			/*
			if (pread(fd, readbuf, bs, ofs) != bs) {
				perror("pread");
				goto closeall;
			}
			*/
		}	

		//Ken, 20081005
		DataOobLocal.rtk_data.start = DataOobLocal.rtk_oob.start = ofs;
		DataOobLocal.rtk_data.length = meminfo.oobblock;

		if ( !realtek && tune_buffer_data){
			index = process_cnt % process_cnt_mode;
			DataOobLocal.rtk_data.ptr = t_totalbuf + index*(pagesize+oobsize);
			DataOobLocal.rtk_oob.ptr = t_totalbuf + index*(pagesize+oobsize)+pagesize;
		}else{
			DataOobLocal.rtk_data.ptr = readbuf;
			//DataOobLocal.rtk_oob.length = meminfo.oobsize;
			DataOobLocal.rtk_oob.ptr = oobbuf;		
		}
	
		if (ioctl(fd, MEMREADDATAOOB, &DataOobLocal) != 0) {
			perror("ioctl(MEMREADDATAOOB)");
			goto closeall;
		}

		if (packed && tune_buffer_data){
			reverse_to_Yaffs2Tags(t_totalbuf + index*(pagesize+oobsize)+pagesize);
		}
	
		if ( realtek ){
			rtk_pcnt = meminfo.oobblock/RTK_PU_DATA;
			
			for( i=0; i<rtk_pcnt; i++){
				/* Write out page data */
				if (pretty_print) {
					for (m = 0; m < RTK_PU_DATA; m += 16) {
						sprintf(pretty_buf,
							"0x%08x: %02x %02x %02x %02x %02x %02x %02x "
							"%02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
							(unsigned int) (ofs + i*RTK_PU_DATA+m),  readbuf[i*RTK_PU_DATA+m],
							readbuf[i*RTK_PU_DATA+m+1], readbuf[i*RTK_PU_DATA+m+2],
							readbuf[i*RTK_PU_DATA+m+3], readbuf[i*RTK_PU_DATA+m+4],
							readbuf[i*RTK_PU_DATA+m+5], readbuf[i*RTK_PU_DATA+m+6],
							readbuf[i*RTK_PU_DATA+m+7], readbuf[i*RTK_PU_DATA+m+8],
							readbuf[i*RTK_PU_DATA+m+9], readbuf[i*RTK_PU_DATA+m+10],
							readbuf[i*RTK_PU_DATA+m+11], readbuf[i*RTK_PU_DATA+m+12],
							readbuf[i*RTK_PU_DATA+m+13], readbuf[i*RTK_PU_DATA+m+14],
							readbuf[i*RTK_PU_DATA+m+15]);
							write(ofd, pretty_buf, 60);
					}
				}else
					write(ofd, readbuf + i*RTK_PU_DATA, RTK_PU_DATA);
				
				/* Write out OOB data */
				if (pretty_print) {
					for (m = 0; m < RTK_PU_OOB; m += 16) {
						sprintf(pretty_buf, "  OOB Data: %02x %02x %02x %02x %02x %02x "
						"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
							oobbuf[i*RTK_PU_OOB+m],
							oobbuf[i*RTK_PU_OOB+m+1], oobbuf[i*RTK_PU_OOB+m+2],
							oobbuf[i*RTK_PU_OOB+m+3], oobbuf[i*RTK_PU_OOB+m+4],
							oobbuf[i*RTK_PU_OOB+m+5], oobbuf[i*RTK_PU_OOB+m+6],
							oobbuf[i*RTK_PU_OOB+m+7], oobbuf[i*RTK_PU_OOB+m+8],
							oobbuf[i*RTK_PU_OOB+m+9], oobbuf[i*RTK_PU_OOB+m+10],
							oobbuf[i*RTK_PU_OOB+m+11], oobbuf[i*RTK_PU_OOB+m+12],
							oobbuf[i*RTK_PU_OOB+m+13], oobbuf[i*RTK_PU_OOB+m+14],
							oobbuf[i*RTK_PU_OOB+m+15]);
							write(ofd, pretty_buf, 60);
					}
				}else
					write(ofd, oobbuf + i*(RTK_PU_OOB), RTK_PU_OOB);
			}
		}else{
			/* Write out page data */
			if (pretty_print) {
				for (i = 0; i < bs; i += 16) {
					sprintf(pretty_buf,
						"0x%08x: %02x %02x %02x %02x %02x %02x %02x "
						"%02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
						(unsigned int) (ofs + i),  readbuf[i],
						readbuf[i+1], readbuf[i+2],
						readbuf[i+3], readbuf[i+4],
						readbuf[i+5], readbuf[i+6],
						readbuf[i+7], readbuf[i+8],
						readbuf[i+9], readbuf[i+10],
						readbuf[i+11], readbuf[i+12],
						readbuf[i+13], readbuf[i+14],
						readbuf[i+15]);
					write(ofd, pretty_buf, 60);
				}
			}else{
				if ( !tune_buffer_data)
					write(ofd, readbuf, bs);
			}

			if (omitoob)
				continue;
			
			if (badblock) {
				memset (readbuf, 0xff, meminfo.oobsize);
			} else {
				/* Read OOB data and exit on failure */
				/*
				oob.start = ofs;
				if (ioctl(fd, MEMREADOOB, &oob) != 0) {
					perror("ioctl(MEMREADOOB)");
					goto closeall;
				}
				*/
			}

			/* Write out OOB data */
			if (pretty_print) {
				if (meminfo.oobsize < 16) {
					sprintf(pretty_buf, "  OOB Data: %02x %02x %02x %02x %02x %02x "
						"%02x %02x\n",
						oobbuf[0], oobbuf[1], oobbuf[2],
						oobbuf[3], oobbuf[4], oobbuf[5],
						oobbuf[6], oobbuf[7]);
					write(ofd, pretty_buf, 48);
					continue;
				}

				for (i = 0; i < meminfo.oobsize; i += 16) {
					sprintf(pretty_buf, "  OOB Data: %02x %02x %02x %02x %02x %02x "
						"%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
						oobbuf[i], oobbuf[i+1], oobbuf[i+2],
						oobbuf[i+3], oobbuf[i+4], oobbuf[i+5],
						oobbuf[i+6], oobbuf[i+7], oobbuf[i+8],
						oobbuf[i+9], oobbuf[i+10], oobbuf[i+11],
						oobbuf[i+12], oobbuf[i+13], oobbuf[i+14],
						oobbuf[i+15]);
					write(ofd, pretty_buf, 60);
				}
			} else {
				if ( !tune_buffer_data ){
					if(packed) {
						memset(oobpackedbuf, 0xff, oobsize);
						for (i = 0, j = 0; j < meminfo.oobsize; i += 4, j += 16) {
							if(i < 4) {
								oobpackedbuf[i] = oobbuf[j+1];
								oobpackedbuf[i+1] = oobbuf[j+2];
								oobpackedbuf[i+2] = oobbuf[j+3];
								oobpackedbuf[i+3] = oobbuf[j+4];
							} else {
								oobpackedbuf[i] = oobbuf[j];
								oobpackedbuf[i+1] = oobbuf[j+1];
								oobpackedbuf[i+2] = oobbuf[j+2];
								oobpackedbuf[i+3] = oobbuf[j+3];
							}
						}
						write(ofd, oobpackedbuf, meminfo.oobsize);
					} else {
						write(ofd, oobbuf, meminfo.oobsize);
					}
				}
			}
		
			process_cnt++;
				
			if ( tune_buffer_data ){
				if ( !(process_cnt%process_cnt_mode) ){
					write(ofd, t_totalbuf, tune_buffer_data+tune_buffer_oob);
					sync();
				}
			}
		}
	}

	//CMYu, 20090910
	if (ioctl (fd, MEMSETOOBSEL, &none_oobinfo) != 0) {
		perror ("MEMSETOOBSEL");
		goto closeall;
	} 
	
	//printf("[%s] none_oobinfo.useecc=%d\n", __FUNCTION__, none_oobinfo.useecc);
	
	//Ken, 20081222, free buffer
	free(readbuf);
	free(oobbuf);	
	free(oobpackedbuf);	
	if ( !realtek && tune_buffer_data)
		free(t_totalbuf);
			
	/* Close the output file and MTD device */
	close(fd);
	close(ofd);

	/* Exit happy */
	return 0;

 closeall:
	free(readbuf);
	free(oobbuf);	
	free(oobpackedbuf);
	if ( !realtek && tune_buffer_data)
		free(t_totalbuf);
	close(fd);
	close(ofd);
	exit(1);
}
