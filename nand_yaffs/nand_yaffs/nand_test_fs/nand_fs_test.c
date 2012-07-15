/******************************************************************************
 * system/ap/develop/nand_yaffs/nand_test_fs/nand_fs_test.c
 * Overview: test yaffs filesystem with nand driver
 * Copyright (c) 2008 Realtek Semiconductor Corp. All Rights Reserved.
 * Modification History:
 *    #000 2008-10-29 Ken-Yu   modify file
 *
 *******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <sys/vfs.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mount.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <ctype.h>

#define RTK_DEBUG 0
#if RTK_DEBUG
   #define DBG(MSG, ...)  printf (MSG, ##__VA_ARGS__)
#else
   #define DBG(MSG, ...)
#endif

#define test_performance 0

#if test_performance
   #define PERFORMANCE
#else
   #define ACCESS_TEST
#endif

#define TEST_DIR 1

#define KB	1024
#define MB	1048576
#define NAND_SIZE	2*1024*1024*1024

#define NAMESIZE 32
static char test_file[NAMESIZE];
static char test_file_r[NAMESIZE];

static char *fstype, *smdev, *dir;
static unsigned int filesize, filenum, cnt;

static char wbuf[KB], rbuf[KB];
static unsigned long wtime = 0, rtime = 0; // unit: msec

static int inconsistent_data = 0;

void gen_pattern()
{
	int i;
	srand((int) time(0));	
	for (i = 0; i < KB; i++)
		wbuf[i] = toascii( 65+(int)(rand() % 10) );
}


int test_write(int fno)
{
	int i,fd;
	struct timeval start_t,end_t;
	long long diff_t; //unit : ms
	
	/* open test_file  */	
	if ((fd = open (test_file, O_CREAT | O_WRONLY | O_TRUNC, 0660)) < 0) {
                printf ("Error: cannot open %s\n",test_file);
		return -1;
        }
	
	/* write test_file */
	gettimeofday(&start_t,NULL);
	for (i = 0; i < (filesize * KB); i++) {
	        if ( write (fd, wbuf, KB) < 0) {
                	printf ("Error: cannot write %s\n", test_file);
			return -1;
       	        }
	}
	sync();	
	sync();	
	sync();	
	gettimeofday(&end_t,NULL);
	
	diff_t = (end_t.tv_sec - start_t.tv_sec) * 1000000  + 
		 (end_t.tv_usec - start_t.tv_usec);
	diff_t = diff_t / 1000; // msec

	wtime += diff_t;
	
#ifdef PERFORMANCE
        printf("[file%d] WR :  %lld msec\n", fno, diff_t);
#endif

	/* close test_file */
	if(fd > 0)
		close(fd);
	
	return 0;
}

void verify_data()
{
	int i;

	for (i=0; i<sizeof(rbuf); i++ ){
		if (wbuf[i] != rbuf[i]){
			inconsistent_data++;
		}
	}
	
#if 0           
	if ( memcmp(wbuf, rbuf, sizeof(rbuf)) )
		data_err++;
#endif  
}


int test_read(int fno)
{
	int i,fd;
        struct timeval start_t,end_t;
        long long diff_t; //unit : ms

	/* open test_file  */	
	if((fd = open(test_file,O_RDONLY)) < 0){
            printf("Error: cannot open %s\n", test_file);
	    return -1;
        }
  
        /* read test_file */
	gettimeofday(&start_t,NULL);
	for (i = 0 ; i < (filesize * KB); i++){
	        if (read (fd, rbuf, KB) < 0) {
                	printf ("Error: cannot read %s\n", test_file);
			return -1;
       	        }
#ifdef ACCESS_TEST
                /* compare read data with write data */
                verify_data();
#endif
	}
	sync();	
	sync();	
	sync();	
	gettimeofday(&end_t,NULL);
	
        diff_t = (end_t.tv_sec - start_t.tv_sec) * 1000000  + 
		 (end_t.tv_usec - start_t.tv_usec);
        diff_t = diff_t / 1000 ; //msec
	
        rtime += diff_t;
	
#ifdef PERFORMANCE
	printf("[file%d] RE :  %lld msec\n", fno, diff_t);
#endif

	/* close test_file */
	if(fd > 0)
		close(fd);
	
	return 0;
}

void test_result()
{
	unsigned long w_throughput,r_throughput; //unit: byte/s
	
	w_throughput = cnt * filesize * filenum * MB / wtime ;
	r_throughput = cnt * filesize * filenum * MB / rtime ;

	printf("\nIO  size(MB) \t filenum \t count \t  time(ms) \t bytes/ms \t      KB/s\n");
	printf("W %5d \t %5d \t %12d \t %9ld \t %9ld \t %9ld\n",
		filesize, filenum, cnt, wtime, w_throughput, w_throughput*1000/1024);
	printf("R %5d \t %5d \t %12d \t %9ld \t %9ld \t %9ld\n",
		filesize, filenum, cnt, rtime, r_throughput, r_throughput*1000/1024);

}

void usage()
{
#ifdef PERFORMANCE
	printf("Usage: nand_ptest <fstype> <nand_dev> <mount_point> "
	       "<filesize (MB)> <filenum> <loop_count>\n");
#endif
#ifdef ACCESS_TEST
	printf("Usage: nand_ptest <fstype> <nand_dev> <mount_point> "
	       "<filesize (MB)> <filenum> <loop_count>\n");
#endif
	
	exit(1);
}

int main(int argc, char **argv)
{
	int i, j;
	char mount_cmd[128],umount_cmd[128];
	int err = 0;

	if (argc != 7)
		usage();

	fstype 		= argv[1];
	smdev		= argv[2];
	dir		= argv[3];
	filesize	= strtoul(argv[4], NULL, 0);
	filenum		= strtoul(argv[5], NULL, 0);
	cnt		= strtoul(argv[6], NULL, 0);

	if ( (filesize * filenum) > (unsigned long int)NAND_SIZE ){
		printf("%s: totoal testing size exceed nand free size\n", argv[0]);
		err = -1;
		goto END;
	}

	sprintf(umount_cmd,"umount %s", smdev);
	sprintf(mount_cmd,"mount -t %s %s %s", fstype, smdev, dir);

	/* start to test */
#ifdef PERFORMANCE
	printf("\n\t\t*** NAND Performance ***\n");
#endif
#ifdef ACCESS_TEST
	printf("\n\t\t*** NAND Access Test ***\n");
#endif
	gen_pattern();

	for ( i = 0 ; i < cnt; i++)
	{
		printf("\n========== Testing %d ==========\n",i+1);

#ifdef ACCESS_TEST
                printf ("Begin write random data to "
                        "<%s> device ...\n", smdev);
#endif
		
                DBG("%s\n", umount_cmd);
                umount(dir);
                DBG("%s\n", mount_cmd);
                mount(smdev, dir, fstype, 0, '\0');

		for (j=0; j < filenum; j++) 
		{
			memset (test_file, 0, NAMESIZE);
			snprintf (test_file, NAMESIZE - 1, "%s/file_%d", dir, j+1);
			
			DBG ("test_file = %s\n", test_file);
			if ((err = test_write(j+1)) < 0 ){
				printf ("write data failed\n");
				goto END;
			}

#if TEST_DIR
			/* test mkdir and rmdir */
			snprintf (test_file_r, NAMESIZE - 1, "%s/dir_%d", dir, j+1);
			DBG ("test_file_r = %s\n", test_file_r);
			
			if ( mkdir(test_file_r, O_RDWR) )
				DBG ("mkdir <%s> failed\n", test_file_r);
			
			if ( rmdir(test_file_r) )
				DBG ("rmdir <%s> failed\n", test_file_r);
#endif					
		}

#ifdef ACCESS_TEST
		printf ("write data successfully\n");
#endif

#if TEST_DIR
		printf ("mkdir and rmdir successfully\n");
#endif

#ifdef ACCESS_TEST
                printf ("Begin read data and verify from "
			"<%s> device ...\n", smdev);
#endif
		
		for (j=0 ; j < filenum; j++)
		{
			memset (test_file, 0, NAMESIZE);
			snprintf (test_file, NAMESIZE - 1, "%s/file_%d", dir, j+1);

			DBG ("test_file = %s\n", test_file);
			if ((err = test_read(j+1)) < 0){
				printf ("read data failed\n");
				goto END;
			}

			/* remove test_file */
			if (remove(test_file))
				DBG ("remove <%s> failed\n", test_file);
			/* for ext3 */
			sync();
			sync();
			sync();
		}
#ifdef ACCESS_TEST
		printf ("read data successfully\n");
#endif

	}
#ifdef ACCESS_TEST
	printf ("\nThere are %d bytes as inconsistent write/read data\n",
		inconsistent_data);
#endif
	
#ifdef PERFORMANCE
	test_result();
#endif

END:
	if (err < 0)
		printf("\n\t\t*** Test Failed ***\n\n");
	else
		printf("\n\t\t*** Test Complete ***\n\n");

	return 0;

}
