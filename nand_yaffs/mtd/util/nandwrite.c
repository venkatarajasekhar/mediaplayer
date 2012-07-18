/*
 *  nandwrite.c
 *
 *  Copyright (C) 2000 Steven J. Hill (sjhill@realitydiluted.com)
 *   		  2003 Thomas Gleixner (tglx@linutronix.de)
 *
 * $Id: nandwrite.c,v 1.30 2005/04/07 14:17:46 dbrown Exp $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Overview:
 *   This utility writes a binary image directly to a NAND flash
 *   chip or NAND chips contained in DoC devices. This is the
 *   "inverse operation" of nanddump.
 *
 * tglx: Major rewrite to handle bad blocks, write data with or without ECC
 *	 write oob data only on request
 *
 * Bug/ToDo:
 *
 *
 *
 * Modification History:
 *    #000 2008-10-03 CMYu   add ioctl( , MEMWRITEDATAOOB, ) to do page write one time
 *    #001 2008-10-08 CMYu   add erase before writing in option
 *    #002 2008-12-22 CMYu   support 4K page size
 *    #003 2009-02-12 CMYu   support writing normal file
 */


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


#define PROGRAM "nandwrite"
#define VERSION "$Revision: 1.30 $"

#define MAX_PAGE_SIZE	2048
#define MAX_OOB_SIZE	64
//Ken, 20081222
unsigned int pagesize, oobsize;

// oob layouts to pass into the kernel as default
struct nand_oobinfo none_oobinfo = { 
	.useecc = MTD_NANDECC_OFF,
};

struct nand_oobinfo jffs2_oobinfo = { 
	.useecc = MTD_NANDECC_PLACE,
	.eccbytes = 6,
	.eccpos = { 0, 1, 2, 3, 6, 7 }
};

struct nand_oobinfo yaffs_oobinfo = { 
	.useecc = MTD_NANDECC_PLACE,
	.eccbytes = 6,
	.eccpos = { 8, 9, 10, 13, 14, 15}
};

struct nand_oobinfo autoplace_oobinfo = {
	.useecc = MTD_NANDECC_AUTOPLACE
};

void display_help (void)
{
	printf("Usage: nandwrite [OPTION] MTD_DEVICE INPUTFILE\n"
	       "Writes to the specified MTD device.\n"
	       "\n"
	       "  -a, --autoplace  	Use auto oob layout\n"
	       "  -j, --jffs2  	 	force jffs2 oob layout (legacy support)\n"
	       "  -y, --yaffs  	 	force yaffs oob layout (legacy support)\n"
	       "  -f, --forcelegacy     force legacy support on autoplacement enabled mtd device\n"
	       "  -n, --noecc		write without ecc\n"
	       "  -o, --oob    	 	image contains oob data\n"
	       "  -s addr, --start=addr set start address (default is 0)\n"
	       "  -p, --pad             pad to page size\n"
	       "  -b, --blockalign=1|2|4 set multiple of eraseblocks to align to\n"
	       "  -q, --quiet    	don't display progress messages\n"
	       "  -e, --erase    	add erase before writing\n"
	       "  -i, --Ignore0xff    	ignore the pagethe that data and oob are all 0xff\n"
	       "  -c, --compare   read back page data and oob after write page data and oob to verify\n"
	       "  -l imglen, --len=imglen set image length to imglen when image is from STDIN. (default is 0)\n"
	       "      --help     	display this help and exit\n"
	       "      --version  	output version information and exit\n");
	exit(0);
}

void display_version (void)
{
	printf(PROGRAM " " VERSION "\n"
	       "\n"
	       "Copyright (C) 2003 Thomas Gleixner \n"
	       "\n"
	       PROGRAM " comes with NO WARRANTY\n"
	       "to the extent permitted by law.\n"
	       "\n"
	       "You may redistribute copies of " PROGRAM "\n"
	       "under the terms of the GNU General Public Licence.\n"
	       "See the file `COPYING' for more information.\n");
	exit(0);
}

char 	*mtd_device, *img;
int 	mtdoffset = 0;
int 	quiet = 0;
int	writeoob = 0;
int	autoplace = 0;
int	forcejffs2 = 0;
int	forceyaffs = 0;
int	forcelegacy = 0;
int	noecc = 0;
int	pad = 0;
int	blockalign = 1; /*default to using 16K block size */
int	erase_first = 0;
int	img_len = 0;
int 	NoOob = 0;
int 	Ignore0xff = 0;
int 	compare = 0;

void process_options (int argc, char *argv[])
{
	int error = 0;

	for (;;) {
		int option_index = 0;
		static const char *short_options = "os:ajynqpel:xic";
		static const struct option long_options[] = {
			{"help", no_argument, 0, 0},
			{"version", no_argument, 0, 0},
			{"oob", no_argument, 0, 'o'},
			{"start", required_argument, 0, 's'},
			{"autoplace", no_argument, 0, 'a'},
			{"jffs2", no_argument, 0, 'j'},
			{"yaffs", no_argument, 0, 'y'},
			{"forcelegacy", no_argument, 0, 'f'},
			{"noecc", no_argument, 0, 'n'},
			{"quiet", no_argument, 0, 'q'},
			{"pad", no_argument, 0, 'p'},
		   	{"NoOob", no_argument, 0, 'x'},
		   	{"Ignore0xff", no_argument, 0, 'i'},
		   	{"compare", no_argument, 0, 'c'},
		   	{"blockalign", required_argument, 0, 'b'},
		   	{"len", required_argument, 0, 'l'},
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
		case 'q':
			quiet = 1;
			break;
		case 'a':
			autoplace = 1;
			break;
		case 'j':
			forcejffs2 = 1;
			break;
		case 'y':
			forceyaffs = 1;
			break;
		case 'f':
			forcelegacy = 1;
			break;
		case 'n':
			noecc = 1;
			break;
		case 'o':
			writeoob = 1;
			break;
		case 'p':
			pad = 1;
			break;
		case 'e':
			erase_first = 1;
			break;			
		case 'x':
			NoOob = 1;
			break;
		case 'i':
			Ignore0xff = 1;
			break;
		case 'c':
			compare = 1;
			break;			
		case 's':
			mtdoffset = atoi (optarg);
			break;
		case 'b':
			blockalign = atoi (optarg);
			break;
		case 'l':
			img_len = (int)strtol (optarg, NULL, 0);
			break;
		case '?':
			error = 1;
			break;
		}
	}
	
	if ((argc - optind) != 2 || error) 
		display_help ();
	
	mtd_device = argv[optind++];
	img = argv[optind];
}

ssize_t safe_read(int fd, void *buf, size_t count)
{
	ssize_t n;

	do {
		n = read(fd, buf, count);
	} while (n < 0 && errno == EINTR);

	return n;
}

ssize_t full_read(int fd, void *buf, size_t len)
{
	ssize_t cc;
	ssize_t total;

	total = 0;

	while (len > 0) {
		cc = safe_read(fd, buf, len);

		if (cc < 0)
			return cc;	/* read() returns -1 on failure. */

		if (cc == 0)
			break;

		buf = ((char *)buf) + cc;
		total += cc;
		len -= cc;
	}

	return total;
}


static void rtk_dump_nand_data (char *name, unsigned char *buf, int start, int size)
{
	int j;
	int print_col = 16;
	printf("print %s buffer content:\n", name);
	for ( j=start; j < start+size; j++){
		if ( !(j % print_col) )
			printf("[%p]:", &buf[j]);
		printf(" %02x", buf[j]);
		if ( (j % print_col) == print_col-1 )
			printf("\n");
	}
	printf("\n");
}


static void reverse_to_Yaffs2Tags(unsigned char *r_oobbuf)
{
	int k;
	for ( k=0; k<16; k++ ){
		r_oobbuf[k]  = r_oobbuf[1+k];
	}
}


int verify_data(unsigned char *w_data, unsigned char *r_data, int size)
{
	int i = 0;
	for( i=0; i<size; i++){
		//if ( mtdoffset ==16*1024*1024 )
			//printf("w_data[%d]=%c, r_data[%d]=%c\n", i, w_data[i], i, r_data[i]);
		if ( w_data[i] != r_data[i] ){
			return i;
		}
	}
	return 0;
}


int compare_data(struct mtd_info_user *meminfo, int fd, unsigned char *writebuf, 
	unsigned char *oobreadbuf, unsigned char *v_readbuf, unsigned char *v_oobbuf)
{
	int rc;
	struct mtd_data_oob v_DataOobLocal;
	v_DataOobLocal.rtk_data.start = v_DataOobLocal.rtk_oob.start = mtdoffset;
	v_DataOobLocal.rtk_data.length = meminfo->oobblock;
	v_DataOobLocal.rtk_oob.length = meminfo->oobsize;
	v_DataOobLocal.rtk_data.ptr = v_readbuf;
	v_DataOobLocal.rtk_oob.ptr = v_oobbuf;
	
	if (ioctl(fd, MEMREADDATAOOB, &v_DataOobLocal) != 0) {
		perror("ioctl(MEMREADDATAOOB)");
		return -1;
	}

#if 0
	if ( mtdoffset ==16*1024*1024 ){
		rtk_dump_nand_data("oobreadbuf", oobreadbuf, 0, meminfo->oobsize);
		rtk_dump_nand_data("v_oobbuf", v_oobbuf, 0, meminfo->oobsize);
	}
#endif
	
	reverse_to_Yaffs2Tags(v_oobbuf);

#if 0
	if ( mtdoffset ==16*1024*1024 ){
		rtk_dump_nand_data("after reverse Yaffs v_oobbuf", v_oobbuf, 0, meminfo->oobsize);
	}
#endif
		
	//compare oob
	if ( !NoOob ){
		rc = verify_data(oobreadbuf, v_oobbuf, 16);
		if (rc){
			printf("Verification page oob fails at mtdoffset %d byte %d\n", mtdoffset, rc);
			return -1;
		}
	}
	
	//compare data
	rc = verify_data(writebuf, v_readbuf, meminfo->oobblock);
	if (rc){
		printf("Verification page data fails at mtdoffset %d byte %d\n", mtdoffset, rc);
		return -1;
	}
	
	return 0;
}


/*
 * Main program
 */
int main(int argc, char **argv)
{
	int cnt, fd, ifd, imglen = 0, pagelen, baderaseblock, blockstart = -1;
	struct mtd_info_user meminfo;
	//struct mtd_oob_buf oob;
	loff_t offs;
	int ret, readlen;
	int oobinfochanged = 0;
	struct nand_oobinfo old_oobinfo;
	
	//Ken: 20081003
	struct mtd_data_oob DataOobLocal;
	//Ken: add erase before write data
	erase_info_t erase;
	int rc = 0;
	//int counter = 0;
	int data_all_0xff = 1, oob_all_0xff = 1;
	int i;

	/*
 	* Buffer array used for writing data
 	*/
	unsigned char *writebuf;
	unsigned char *oobbuf;
	unsigned char *oobreadbuf;
	/* CMYu, 20090420, buffer for verification */
	unsigned char *v_readbuf;
	unsigned char *v_oobbuf;
	
	pagesize =  MAX_PAGE_SIZE;
	oobsize = MAX_OOB_SIZE;
	
	process_options(argc, argv);

	/* Open the device */
	if ((fd = open(mtd_device, O_RDWR)) == -1) {
		perror("open flash");
		exit(1);
	}

	//printf("fd=%d\n", fd);
	
	/* Fill in MTD device capability structure */  
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		perror("MEMGETINFO");
		close(fd);
		exit(1);
	}

	//printf("meminfo.size=%d\n", meminfo.size);
	
	/* Set erasesize to specified number of blocks - to match jffs2 (virtual) block size */
	meminfo.erasesize *= blockalign;

	/* Ken, 20081222: Make sure device page sizes are valid */
	if ( meminfo.oobblock&(512-1) || meminfo.oobsize&(16-1) ){
		fprintf(stderr, "Unknown flash (not normal NAND)\n");
		close(fd);
		exit(1);	
	}

#if 0	     
	/* Make sure device page sizes are valid */
	if (!(meminfo.oobsize == 16 && meminfo.oobblock == 512) &&
	    !(meminfo.oobsize == 8 && meminfo.oobblock == 256) && 
	    !(meminfo.oobsize == 64 && meminfo.oobblock == 2048) && 
	    !(meminfo.oobsize == 128 && meminfo.oobblock == 4096)){
		fprintf(stderr, "Unknown flash (not normal NAND)\n");
		close(fd);
		exit(1);
	}
#endif

	//Ken, 20081222	
	pagesize = meminfo.oobblock;
	oobsize = meminfo.oobsize;
	//printf("pagesize=%d, oobsize=%d\n", pagesize, oobsize);
	
	//Ken, 20081219
	if (( writebuf = malloc(pagesize)) == NULL ){
		printf("allocate writebuf fails\n");
		goto EXIT;
	}
	memset(writebuf, 0xff, sizeof(writebuf));

	if (( oobbuf = malloc(oobsize)) == NULL ){
		printf("allocate oobbuf fails\n");
		goto EXIT;
	}
	memset(oobbuf, 0xff, sizeof(oobbuf));
	
	if (( oobreadbuf = malloc(oobsize)) == NULL ){
		printf("allocate oobreadbuf fails\n");
		goto EXIT;
	}
	memset(oobreadbuf, 0xff, sizeof(oobreadbuf));

	//CMYu, 20090420
	if (( v_readbuf = malloc(pagesize)) == NULL ){
		printf("allocate v_readbuf fails\n");
		goto EXIT;
	}
	memset(v_readbuf, 0xff, sizeof(v_readbuf));

	if (( v_oobbuf = malloc(oobsize)) == NULL ){
		printf("allocate v_oobbuf fails\n");
		goto EXIT;
	}
	memset(v_oobbuf, 0xff, sizeof(v_oobbuf));
	
	if (pad && writeoob) {
		fprintf(stderr, "Can't pad when oob data is present.\n");
		rc = -1;
		goto EXIT;
	}

	/* Read the current oob info */
	if (ioctl (fd, MEMGETOOBSEL, &old_oobinfo) != 0) {
		perror ("MEMGETOOBSEL");
		rc = -1;
		goto EXIT;
	} 
	
	// write without ecc ?
	if (noecc) {
		if (ioctl (fd, MEMSETOOBSEL, &none_oobinfo) != 0) {
			perror ("MEMSETOOBSEL");
			rc = -1;
			goto EXIT;
		}
		oobinfochanged = 1;
	}

	// autoplace ECC ?
	if (autoplace && (old_oobinfo.useecc != MTD_NANDECC_AUTOPLACE)) {
		if (ioctl (fd, MEMSETOOBSEL, &autoplace_oobinfo) != 0) {
			perror ("MEMSETOOBSEL");
			rc = -1;
			goto EXIT;
		} 
		oobinfochanged = 1;
	} 

	/* 
	 * force oob layout for jffs2 or yaffs ?
	 * Legacy support 
	 */  
	if (forcejffs2 || forceyaffs) {
		struct nand_oobinfo *oobsel = forcejffs2 ? &jffs2_oobinfo : &yaffs_oobinfo;

		if (autoplace) {
			fprintf(stderr, "Autoplacement is not possible for legacy -j/-y options\n");
			goto restoreoob;
		}
		if ((old_oobinfo.useecc == MTD_NANDECC_AUTOPLACE) && !forcelegacy) {
			fprintf(stderr, "Use -f option to enforce legacy placement on autoplacement enabled mtd device\n");
			goto restoreoob;
		}
		if (meminfo.oobsize == 8) {
    			if (forceyaffs) {
				fprintf (stderr, "YAFSS cannot operate on 256 Byte page size");
				goto restoreoob;
			}	
			/* Adjust number of ecc bytes */	
			jffs2_oobinfo.eccbytes = 3;	
		}
		
		if (ioctl (fd, MEMSETOOBSEL, oobsel) != 0) {
			perror ("MEMSETOOBSEL");
			goto restoreoob;
		} 
	}

	DataOobLocal.rtk_oob.length = meminfo.oobsize;
	DataOobLocal.rtk_oob.ptr = noecc ? oobreadbuf : oobbuf;
	//oob.length = meminfo.oobsize;
	//oob.ptr = noecc ? oobreadbuf : oobbuf;

	/* Open the input file */
	if(strcmp(img, "-") == 0) {
		if ((ifd = fileno(stdin)) == -1 || img_len == 0) {
			perror("open input file");
			goto restoreoob;
		}
		imglen = img_len;
	} else {
		if ((ifd = open(img, O_RDONLY)) == -1) {
			perror("open input file");
			goto restoreoob;
		}
		// get image length
		imglen = lseek(ifd, 0, SEEK_END);
		lseek (ifd, 0, SEEK_SET);
	}

	//printf("imglen=%d\n", imglen);
	
	pagelen = meminfo.oobblock + ((writeoob == 1) ? meminfo.oobsize : 0);
	
#if 0	
	// Check, if file is pagealigned
	if ((!pad) && ((imglen % pagelen) != 0)) {
		fprintf (stderr, "Input file is not page aligned\n");
		goto closeall;
	}
#endif	
	// Check, if length fits into device
	if ( ((imglen / pagelen) * meminfo.oobblock) > (meminfo.size - mtdoffset)) {
		fprintf (stderr, "Image %d bytes, NAND page %d bytes, OOB area %u bytes, device size %u bytes\n",
				imglen, pagelen, meminfo.oobblock, meminfo.size);
		perror ("Input file does not fit into device");
		goto closeall;
	}
	
	//Ken: add erase before write data
	if ( erase_first )
		erase.length = meminfo.erasesize;	
	
	//printf("imglen=%d, mtdoffset=%d, meminfo.size=%d\n", imglen, mtdoffset, meminfo.size);
	
	
	/* Get data from input and write to the device */
	while (imglen && (mtdoffset < meminfo.size)) {
		// new eraseblock , check for bad block(s)
		// Stay in the loop to be sure if the mtdoffset changes because
		// of a bad block, that the next block that will be written to
		// is also checked. Thus avoiding errors if the block(s) after the 
		// skipped block(s) is also bad (number of blocks depending on 
		// the blockalign
#if 0		
		while (blockstart != (mtdoffset & (~meminfo.erasesize + 1))) {
			blockstart = mtdoffset & (~meminfo.erasesize + 1);
			offs = blockstart;
			baderaseblock = 0;
			//if (!quiet)
				//fprintf (stdout, "Writing data to block %x\n", blockstart);
			
			//Ken: add erase before write data		   
		 	if ( erase_first ){
				erase.start = blockstart;
		 		if(ioctl(fd, MEMERASE, &erase) != 0)	{
					perror("\nMTD Erase failure");
					goto closeall;
				}
			}
		
		        /* Check all the blocks in an erase block for bad blocks */
			do {
			   	if ((ret = ioctl(fd, MEMGETBADBLOCK, &offs)) < 0) {
					perror("ioctl(MEMGETBADBLOCK)");
					goto closeall;
				}
				if (ret == 1) {
					baderaseblock = 1;
				   	if (!quiet)
						fprintf (stderr, "Bad block at %x, %u block(s) from %x will be skipped\n", (int) offs, blockalign, blockstart);
					}
			   
				if (baderaseblock) {		   
					mtdoffset = blockstart + meminfo.erasesize;
				}
			        offs +=  meminfo.erasesize / blockalign ;
			} while ( offs < blockstart + meminfo.erasesize );
		}
#endif

		//CMYu, 20090409, do not check bad blocks
		if ( erase_first ){
			if ( mtdoffset&(meminfo.erasesize-1) ){
				//printf("===mtdoffset %d is not block alignment !!\n", mtdoffset);
			}else{	
				//printf("@@@mtdoffset %d is block alignment !!\n", mtdoffset);
				erase.start = mtdoffset & (~meminfo.erasesize + 1);
				if(ioctl(fd, RTKMEMERASE, &erase) != 0)	{
					perror("\nMTD Erase failure");
					goto closeall;
				}
			}
		}
			
		readlen = meminfo.oobblock;
		if (pad && (imglen < readlen))
		{
			readlen = imglen;
			memset(writebuf + readlen, 0xff, meminfo.oobblock - readlen);
		}

		/* Ken: 20090212  */
		if ( NoOob && (imglen < meminfo.oobblock) ){
			readlen = imglen;
			//printf("counter=%d, readlen=%d\n", counter, readlen);
		}
		
		//cnt = full_read(ifd, writebuf, readlen);
		//counter++;
		//printf("counter=%d, cnt=%d, readlen=%d, imglen=%d\n", counter, cnt, readlen, imglen);
		/* Read Page Data from input file */
		//if ((cnt = read(ifd, writebuf, readlen)) != readlen) {
		if ((cnt = full_read(ifd, writebuf, readlen)) != readlen) {
		//if (cnt != readlen) {
			if (cnt == 0)	// EOF
				break;
			perror ("File I/O error on input file");
			goto closeall;
		}

		if (writeoob) {
			/* Read OOB data from input file, exit on failure */
			//if ((cnt = read(ifd, oobreadbuf, meminfo.oobsize)) != meminfo.oobsize) {
			if ((cnt = full_read(ifd, oobreadbuf, meminfo.oobsize)) != meminfo.oobsize) {
				perror ("File I/O error on input file");
				goto closeall;
			}
			if (!noecc) {
				int i, start, len;
				/* 
				 *  We use autoplacement and have the oobinfo with the autoplacement
				 * information from the kernel available 
				 *
				 * Modified to support out of order oobfree segments,
				 * such as the layout used by diskonchip.c
				 */  
				if (!oobinfochanged && (old_oobinfo.useecc == MTD_NANDECC_AUTOPLACE)) {
					for (i = 0;old_oobinfo.oobfree[i][1]; i++) {
						/* Set the reserved bytes to 0xff */
						start = old_oobinfo.oobfree[i][0];
						len = old_oobinfo.oobfree[i][1];
						memcpy(oobbuf + start,
							oobreadbuf + start,
							len);
					}
				} else {
					/* Set at least the ecc byte positions to 0xff */
					start = old_oobinfo.eccbytes;
					len = meminfo.oobsize - start;
					memcpy(oobbuf + start,
						oobreadbuf + start,
						len);
				}
			}
			/* Write OOB data first, as ecc will be placed in there*/
			/*
			oob.start = mtdoffset;
			if (ioctl(fd, MEMWRITEOOB, &oob) != 0) {
				perror ("ioctl(MEMWRITEOOB)");
				goto closeall;
			}
			imglen -= meminfo.oobsize;
			*/
		}
		
		/* Write out the Page data */
		/*
		if (pwrite(fd, writebuf, meminfo.oobblock, mtdoffset) != meminfo.oobblock) {
			perror ("pwrite");
			goto closeall;
		}
		*/
		
		//Ken, 20081003
		DataOobLocal.rtk_data.start = DataOobLocal.rtk_oob.start = mtdoffset;
		DataOobLocal.rtk_data.length = meminfo.oobblock;
		DataOobLocal.rtk_data.ptr = writebuf;

		if ( !Ignore0xff ){
			if (ioctl(fd, MEMWRITEDATAOOB, &DataOobLocal) != 0) {
				perror ("ioctl(MEMWRITEDATAOOB)");
				goto closeall;
			}
		}else{
		/* CMYu, 20090409
		If the content of data area and oob area are all 0xff, then I do not write to 
		nand device .
		*/
			if ( !NoOob ){
				for (i=0; i<oobsize; i++){
					if ( oobreadbuf[i] !=0xff ){
						//printf("oobreadbuf[%d]=%x\n", i, oobreadbuf[i]);
						oob_all_0xff = 0;
						break;
					}
				}
			}
		
			if ( oob_all_0xff ){
				for (i=0; i<pagesize; i++){
					if ( writebuf[i] !=0xff ){
						//printf("writebuf[%d]=%x\n", i, writebuf[i]);
						data_all_0xff = 0;
						break;
					}
				}
			}

			if ( !oob_all_0xff || !data_all_0xff ){
				//printf("=== mtdoffset=%d, data_all_0xff=%d, oob_all_0xff=%d\n", mtdoffset, data_all_0xff, oob_all_0xff);
				if (ioctl(fd, MEMWRITEDATAOOB, &DataOobLocal) != 0) {
					perror ("ioctl(MEMWRITEDATAOOB)");
					goto closeall;
				}
			}else{
				//printf("@@@ mtdoffset=%d, data_all_0xff=%d, oob_all_0xff=%d\n", mtdoffset, data_all_0xff, oob_all_0xff);
			}
		}	

		if ( compare ){
			rc = compare_data(&meminfo, fd, writebuf, oobreadbuf, v_readbuf, v_oobbuf);
			if ( rc ){
				printf("verification data fails\n");
				goto closeall;
			}
		}
			
		if ( !NoOob )
			imglen -= meminfo.oobsize;
		imglen -= readlen;
		mtdoffset += meminfo.oobblock;
		//printf("imglen=%d, mtdoffset=%d, readlen=%d, oobsize=%d, data_all_0xff=%d, oob_all_0xff=%d\n", 
			//imglen, mtdoffset, readlen, meminfo.oobsize, data_all_0xff, oob_all_0xff);
		if ( Ignore0xff ){
			oob_all_0xff = data_all_0xff = 1;
		}
	}
	
	fprintf (stdout, "Writing data finishes !!\n");
	
 closeall:
	close(ifd);

 restoreoob:
	if (oobinfochanged) {
		if (ioctl (fd, MEMSETOOBSEL, &old_oobinfo) != 0) {
			perror ("MEMSETOOBSEL");
			rc = -1;
			goto EXIT;
		} 
	}

	if (imglen > 0) {
		printf("imglen=%d\n", imglen);
		perror ("Data did not fit into device\n");
		rc = -1;
		goto EXIT;
	}

EXIT:
	if (writebuf)
		free(writebuf);
	if (oobbuf)
		free(oobbuf);
	if (oobreadbuf)
		free(oobreadbuf);
	if (v_readbuf)
		free(v_readbuf);
	if (v_oobbuf)
		free(v_oobbuf);
		
	if ( fd > 0 )
		close(fd);
	
	if ( rc < 0 )
		exit(1);
	else	/* Return happy */
		return 0;
}
