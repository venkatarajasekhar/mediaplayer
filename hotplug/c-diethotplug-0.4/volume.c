//#define _GNU_SOURCE

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <mntent.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>

/*Ken added  these*/
#include <unistd.h> 
#include <errno.h>
#include <stdio.h> 

#include "volume.h"

#ifndef RESCUE_LINUX

char filter_buffer[10];

int part_filter(const struct dirent *param)
{
	int filter_len = strlen(filter_buffer);

	if ((strlen(param->d_name) == filter_len+1) && !strncmp(filter_buffer, param->d_name, filter_len))
		return 1;
	else
		return 0;
}

static int find_sequence(char *name, char *extended)
{
	struct dirent	**namelist;
	int		n, i, j = -1, k = 0;
	int		counter = 0;
	int		flag = 0;

	while (counter < 10) {
		strncpy(filter_buffer, name, 3);
		if (counter == 0) {
			filter_buffer[3] = '\0'; 
		} else {
			filter_buffer[3] = '0'+counter;
			filter_buffer[4] = '\0'; 
		}
//		MY_PRINTF("filter buffer: %s \n", filter_buffer);
	
	        n = scandir("/dev/", &namelist, part_filter, 0);
	        if (n < 0)
	                perror("scandir");
	        else {
			i = 0;
			while (i < n) {
//				MY_PRINTF("  str: %s \n", namelist[i]->d_name);
				if ((extended != 0) && (j == -1) && (!strcmp(namelist[i]->d_name, extended)))
					flag = 1;
				if (!strcmp(namelist[i]->d_name, name))
					j = i+k;
				free(namelist[i]);
				i++;
			}
			k = n;
			free(namelist);
	        }

		if (j != -1)
			break;
		counter++;
	}

	if (flag)
		return j-1;
	else
		return j;
}

static int reserv_volume(volume_struct volume[], char *name, int num, int ext, char *alias)
{
	int seq = 0, i, j, k;
	char temp[NAME_LENGTH] = {0};
/*
	if (num > 10) {
		MY_PRINTF("Error in partition number...\n");
		return -1;
	}
*/
	if (ext != 0) {
		strncpy(temp, name, 3);
		sprintf(temp, "%s%d", temp, ext);
		if (!strcmp(name, temp)) {
			printf("This is extended partition\n");
			return -1;
		}
	}

	// check if this partition has been reserved...
	for (i = 2; i < VOLUME_NUMBER; i++) {
		if (!strcmp(volume[i].name, name)) {
			if (alias != NULL)
				strncpy(volume[i].alias, alias, ALIAS_LENGTH);
			return i;
		}
	}

	if (num != 0) {
		seq = find_sequence(name, temp);
		if (seq < 0) {
			MY_PRINTF("Error in getting sequence number...\n");
			return -1;
		}
		MY_PRINTF("sequence value: %d \n", seq);

		for (i = 2, j = 0; i < VOLUME_NUMBER; i++) {
			if ((strlen(volume[i].name) > 3) && (!strncmp(volume[i].name, name, strlen(volume[i].name)-1))) {
				if (j == seq) {
					if (strchr(volume[i].name, '*') == NULL) {
						MY_PRINTF("Error in getting reserved slot...\n");
						return -1;
					}
					strncpy(volume[i].name, name, NAME_LENGTH);
					if (alias != NULL)
						strncpy(volume[i].alias, alias, ALIAS_LENGTH);
					return i;
				}
				j++;
			}
		}

		strncpy(temp, name, 3);
		temp[3] = '*';
		temp[4] = 0x0;

		if (ext != 0)
			num--;
	}

	// reserve this partition...
	if (alias == NULL) {
		for (i = 2, j = 0, k = -1; i < VOLUME_NUMBER; i++) {
			if (volume[i].available == 0) {
				volume[i].available = 1;
				if (num == 0) {
					// special case... (no partition)
					strncpy(volume[i].name, name, NAME_LENGTH);
	
					MY_PRINTF("reserve %d, name: %s \n", i, name);
	
					k = i;
					break;
				} else {
					// normal case...
					if (j == seq) {
						strncpy(volume[i].name, name, NAME_LENGTH);
						k = i;
					} else {
						strncpy(volume[i].name, temp, NAME_LENGTH);
					}

					MY_PRINTF("reserve %d, name: %s \n", i, volume[i].name);

					if (++j >= num)
						break;
				}
			}
		}
	} else {
		for (i = VOLUME_NUMBER-1, j = num-1, k = -1; i >= 2; i--) {
			if (volume[i].available == 0) {
				volume[i].available = 1;
				if (num == 0) {
					// special case... (no partition)
					strncpy(volume[i].name, name, NAME_LENGTH);
					if (alias != NULL)
						strncpy(volume[i].alias, alias, ALIAS_LENGTH);

					MY_PRINTF("reserve %d, name: %s \n", i, name);

					k = i;
					break;
				} else {
					// normal case...
					if (j == seq) {
						strncpy(volume[i].name, name, NAME_LENGTH);
						k = i;
						if (alias != NULL)
							strncpy(volume[i].alias, alias, ALIAS_LENGTH);
					} else {
						strncpy(volume[i].name, temp, NAME_LENGTH);
					}

					MY_PRINTF("reserve %d, name: %s \n", i, volume[i].name);

					if (--j < 0)
						break;
				}
			}
		}
	}

	if (k >= 0)
		return k;
	else
		return -1;
}

static int remove_volume(volume_struct volume[], int num)
{
	volume[num].available = 0;
	memset(volume[num].name, 0, NAME_LENGTH);
	memset(volume[num].alias, 0, ALIAS_LENGTH);

	return 0;
}

static int search_volume(volume_struct volume[], char *name)
{
	int i;

	// check if this partition has been reserved...
	for (i = 2; i < VOLUME_NUMBER; i++) {
		if (!strcmp(volume[i].name, name)) {
			return i;
		}
	}
	return -1;
}

static void make_symlink(volume_struct volume[], int num, char *mnt_point)
{
	char volume_label[50] = {0};
     
	if (volume[num].alias[0] != 0) {
		strcpy(volume_label, "/tmp/ramfs/volumes/");
		strcat(volume_label, volume[num].alias);
	} else {
		strcpy(volume_label, "/tmp/ramfs/volumes/A:");
		volume_label[strlen(volume_label)-2] += num;
	}

	if ( (symlink(mnt_point, volume_label)) == -1)
            printf("make_symlink(): Symbolic Error!! errno= %s\n", strerror(errno));
}

static void make_file(volume_struct volume[], int num, char *path)
{
	char volume_label[50] = {0};
	int fd, path_len;
     path_len = strlen(path);

	if (volume[num].alias[0] != 0) {
		strcpy(volume_label, "/tmp/ramfs/volumes/");
		strcat(volume_label, volume[num].alias);
	} else {
		strcpy(volume_label, "/tmp/ramfs/volumes/A:");
		volume_label[strlen(volume_label)-2] += num;
	}

    //printf("path=%s, volume_label=%s\n", path, volume_label);
    
    //fd = creat(volume_label, 0644);
    fd = open(volume_label, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    if(fd != -1){
         if ( write(fd, path, path_len) != path_len )
    	        printf ("make_file(): WRITE Error, errno=%s\n", strerror(errno));
    }else{
            printf ("make_file(): CREAT FILE Error, errno=%s\n", strerror(errno));
    }
   close(fd);
}

static void del_symlink(volume_struct volume[], int num)
{

	char volume_label[50] = {0};

	if (volume[num].alias[0] != 0) {
		strcpy(volume_label, "/tmp/ramfs/volumes/");
		strcat(volume_label, volume[num].alias);
	} else {
		strcpy(volume_label, "/tmp/ramfs/volumes/A:");
		volume_label[strlen(volume_label)-2] += num;
	}

     if ( remove(volume_label)==-1 )
     	printf ("del_symlink(): remove Error, errno=%s\n", strerror(errno));
}

static void get_fat_name(char *dev_name, char volumename[], int len)
{
	int fd;
	short *f16_size;
	const int bsize = 4096;
	char buffer[bsize];

	fd = open(dev_name, O_RDONLY);
	if (fd < 0)
		perror("open fat");
	if (read(fd, buffer, bsize) != bsize)
		perror("read fat");
	f16_size = (short *)&buffer[0x16];
	if (*f16_size == 0) {
		short *reserved, *sec_size;
		char *clus, *fats, *attr;
		int *f32_size, *root_cluster, i, j;

		MY_PRINTF("fat32...\n");
		memcpy(volumename, buffer+0x47, len);

		f32_size = (int *)&buffer[0x24];
		reserved = (short *)&buffer[0xe];
		sec_size = (short *)&buffer[0xb];
		clus = (char *)&buffer[0xd];
		fats = (char *)&buffer[0x10];
		root_cluster = (int *)&buffer[0x2c];

		MY_PRINTF("reserved: %d \n", *reserved);
		MY_PRINTF("fat_size: %d \n", *f32_size);
		MY_PRINTF("sec_size: %d \n", *sec_size);
		MY_PRINTF("clus: %d \n", *clus);
		MY_PRINTF("fats: %d \n", *fats);
		MY_PRINTF("root: %d \n", *root_cluster);

		lseek(fd, (*reserved+((*fats)*(*f32_size)))*(*sec_size)+(*root_cluster-2)*(*sec_size)*(*clus), SEEK_SET);
		if (read(fd, buffer, bsize) != bsize)
			perror("read fat");

		for (i = 0, j = -1; i < 128; i++) {
			attr = buffer+i*32+11;
			if ((*attr & 0x8) && ((*attr & 0xf) != 0xf) && ((*attr & 0xf0) == 0)) {
				char *size;

				size = buffer+i*32+31;
				if (*size != 0)
					continue;
//				printf(" %d volumename: %s \n", i, volumename);
				j = i;
			}
		}
		if ((j != -1) && ((int)*(buffer+j*32) != -27))
			memcpy(volumename, buffer+j*32, len);
		else
			strcpy(volumename, "FAT32");
	} else {
		short *reserved, *sec_size;
		char *fats, *attr;
		int i, j;

		MY_PRINTF("fat16...\n");
		memcpy(volumename, buffer+0x2b, len);
		
		reserved = (short *)&buffer[0xe];
		sec_size = (short *)&buffer[0xb];
		fats = (char *)&buffer[0x10];
		
		MY_PRINTF("reserved: %d \n", *reserved);
		MY_PRINTF("fat_size: %d \n", *f16_size);
		MY_PRINTF("sec_size: %d \n", *sec_size);
		MY_PRINTF("fats: %d \n", *fats);

		lseek(fd, (*reserved+((*fats)*(*f16_size)))*(*sec_size), SEEK_SET);
		if (read(fd, buffer, bsize) != bsize)
			perror("read fat");

		for (i = 0, j = -1; i < 128; i++) {
			attr = buffer+i*32+11;
			if ((*attr & 0x8) && ((*attr & 0xf) != 0xf) && ((*attr & 0xf0) == 0)) {
				char *size;

				size = buffer+i*32+31;
				if (*size != 0)
					continue;
//				printf(" %d volumename: %s \n", i, volumename);
				j = i;
			}
		}
		if ((j != -1) && ((int)*(buffer+j*32) != -27))
			memcpy(volumename, buffer+j*32, len);
		else
			strcpy(volumename, "FAT16");
	}

	close(fd);
}

static int get_ntfs_name(char *dev_name, char volumename[], int len)
{
	int fd;
	short *BytePerSec;
	char *SecPerClus;
	long long *mft, *bak;
	int *record_size;
	const int bsize = 8192; 
	char buffer[bsize];
	MFT_RECORD *mft_record;
	ATTR_RECORD *attr_record;
	int mft_offset;
	int attr_offset;
	int ret = 0;

	fd = open64(dev_name, O_RDONLY);
	if (fd < 0)
		perror("open ntfs");
	if (read(fd, buffer, bsize) != bsize)
		perror("read ntfs");
	BytePerSec = (short *)&buffer[0xb];
	SecPerClus = (char *)&buffer[0xd];
	mft = (long long *)&buffer[0x30];
	bak = (long long *)&buffer[0x38];
	record_size = (int *)&buffer[0x40];
	printf("BytePerSec: %d \n", *BytePerSec);
	printf("SecPerClus: %d \n", *SecPerClus);
	printf("MFT: %lld \n", *mft);
	printf("BAK: %lld \n", *bak);
	printf("record: %d \n", *record_size);

//	printf("seek: %lld \n", (*BytePerSec)*(*SecPerClus)*(*mft));
	lseek64(fd, (*mft)*(*BytePerSec)*(*SecPerClus), SEEK_SET);
	if (read(fd, buffer, bsize) != bsize)
		perror("read mft");

	// $MFT
	mft_offset = 0;
	mft_record = (MFT_RECORD *)&buffer[mft_offset];
//	printf("attr_offset: %d \n", mft_record->attrs_offset);
	attr_offset = mft_offset+mft_record->attrs_offset;
	attr_record = (ATTR_RECORD *)&buffer[attr_offset];
	while (*(unsigned int *)attr_record != 0xffffffff) {
//		printf("attr_type: %x \n", attr_record->type);
		attr_offset += attr_record->length;
//		printf("\tattr_offset: %d \n", attr_offset);
		attr_record = (ATTR_RECORD *)&buffer[attr_offset];
	}

	// $MFTMirr
	mft_offset += mft_record->bytes_allocated;
	mft_record = (MFT_RECORD *)&buffer[mft_offset];
//	printf("attr_offset: %d \n", mft_record->attrs_offset);
	attr_offset = mft_offset+mft_record->attrs_offset;
	attr_record = (ATTR_RECORD *)&buffer[attr_offset];
	while (*(unsigned int *)attr_record != 0xffffffff) {
//		printf("attr_type: %x \n", attr_record->type);
		attr_offset += attr_record->length;
//		printf("\tattr_offset: %d \n", attr_offset);
		attr_record = (ATTR_RECORD *)&buffer[attr_offset];
	}

	// $LogFile
	mft_offset += mft_record->bytes_allocated;
	mft_record = (MFT_RECORD *)&buffer[mft_offset];
//	printf("attr_offset: %d \n", mft_record->attrs_offset);
	attr_offset = mft_offset+mft_record->attrs_offset;
	attr_record = (ATTR_RECORD *)&buffer[attr_offset];
	while (*(unsigned int *)attr_record != 0xffffffff) {
//		printf("attr_type: %x \n", attr_record->type);
		attr_offset += attr_record->length;
//		printf("\tattr_offset: %d \n", attr_offset);
		attr_record = (ATTR_RECORD *)&buffer[attr_offset];
	}

	// $Volume
	mft_offset += mft_record->bytes_allocated;
	mft_record = (MFT_RECORD *)&buffer[mft_offset];
//	printf("attr_offset: %d \n", mft_record->attrs_offset);
	attr_offset = mft_offset+mft_record->attrs_offset;
	attr_record = (ATTR_RECORD *)&buffer[attr_offset];
	while (*(unsigned int *)attr_record != 0xffffffff) {
//		printf("attr_type: %x \n", attr_record->type);
		if (attr_record->type == 0x60) {
//			printf("value_offset: %d \n", attr_record->data.resident.value_offset);
//			printf("value_length: %d \n", attr_record->data.resident.value_length);
			if (attr_record->data.resident.value_length <= len)
				len = attr_record->data.resident.value_length;
			memcpy(volumename, &buffer[attr_offset+attr_record->data.resident.value_offset], len);
			ret = len;
			break;
		}
		attr_offset += attr_record->length;
//		printf("\tattr_offset: %d \n", attr_offset);
		attr_record = (ATTR_RECORD *)&buffer[attr_offset];
	}

	close(fd);
	return ret;
}

static void make_volumefile(volume_struct volume[], int num, char *fstype, char *fsname)
{
	char volumefile[50] = {0};
	char volumename[50] = {0};
	char volumetype[10] = {0};
	int len = 0;
	char cstype = UTF8;

	if (volume[num].alias[0] != 0) {
		strcpy(volumefile, "/tmp/ramfs/labels/");
		strcat(volumefile, volume[num].alias);
	} else {
		strcpy(volumefile, "/tmp/ramfs/labels/A:");
		volumefile[strlen(volumefile)-2] += num;
	}

	if (!strcmp(fstype, "vfat")) {
		strcpy(volumetype, "fat");
		get_fat_name(fsname, volumename, 11);
		cstype = UNKNOWN;

		MY_PRINTF("volume name: %s, len: %d \n", volumename, strlen(volumename));
/*
		if ((volumename[0] != ' ') && (volumename[0] != 0x0)) {
			char *tmp = strchr(volumename, ' ');
			if (tmp != NULL)
				*tmp = 0x0;
			MY_PRINTF("volume name: %s, len: %d \n", volumename, strlen(volumename));
		} else {
			strcpy(volumename, "FAT");
		}
*/
	} else if (!strcmp(fstype, "ntfs")) {
		strcpy(volumetype, "ntfs");
		len = get_ntfs_name(fsname, volumename, 48);
		if (len > 0)
			cstype = UTF16_LE;
		else
			strcpy(volumename, "NTFS");

		MY_PRINTF("volume name: %s, len: %d \n", volumename, len);
//		strcpy(volumename, "NTFS");
	} else if (!strcmp(fstype, "ptp")) {
		int fd;

		strcpy(volumetype, "ptp");
		fd = open(fsname, O_RDONLY);
		if (fd < 0) {
			MY_PRINTF("Error in open %s \n", fsname);
			strcpy(volumename, "PTP Device");
		} else {
			if (read(fd, volumename, sizeof(volumename)-1) < 0) {
				MY_PRINTF("Error in read %s \n", fsname);
				strcpy(volumename, "PTP Device");
			}
			close(fd);
		}
	} else if (!strcmp(fstype, "ext2")) {
		strcpy(volumetype, "ext2");
		strcpy(volumename, "EXT2");
	} else if (!strcmp(fstype, "ext3")) {
		strcpy(volumetype, "ext3");
		strcpy(volumename, "EXT3");
	} else if (!strcmp(fstype, "udf")) {
		strcpy(volumetype, "udf");
		strcpy(volumename, "UDF");
	} else {
		// we have not supported it yet...
		return;
	}
	if (len == 0)
		len = strlen(volumename);
	if (volumename[0] != 0x0) {
		int fd = open(volumefile, O_CREAT | O_TRUNC | O_WRONLY);
		write(fd, &cstype, 1);
		write(fd, volumename, len);
		if (volumename[len-1] != '\n')
			write(fd, "\n", 1);
		write(fd, volumetype, strlen(volumetype));
		close(fd);
	}
}

static void del_volumefile(volume_struct volume[], int num)
{
	char volumefile[50] = {0};

	if (volume[num].alias[0] != 0) {
		strcpy(volumefile, "/tmp/ramfs/labels/");
		strcat(volumefile, volume[num].alias);
	} else {
		strcpy(volumefile, "/tmp/ramfs/labels/A:");
		volumefile[strlen(volumefile)-2] += num;
	}

     if ( remove(volumefile)==-1 )
     	printf ("del_volumefile(): remove %s Error, errno=%s\n", volumefile, strerror(errno));
}

int add_partition(char *name, char *path, char *alias)
{
	int fd_lock;
	int shmid;
	char *pMem = NULL;
//	char cmd_buffer[100];

//	static int x = 0;

	// get the lock...
	fd_lock = open("/var/lock/hotplug/volume_lock", O_RDONLY);
	if (fd_lock < 0) {
	        MY_PRINTF("Error in open lock file...\n");
	        return  -1;
	}
	flock(fd_lock, LOCK_EX);

        shmid = shmget(0x23792379, 0x1000, IPC_CREAT | IPC_EXCL | 0660);
        if (shmid == -1) {
                shmid = shmget(0x23792379, 0x1000, IPC_EXCL | 0660);
                if (shmid == -1) {
			MY_PRINTF("Error in shmget...\n");
			goto out1;
                }
		pMem = shmat(shmid, 0, 0);
		if (pMem == (void *)-1) {
			MY_PRINTF("Error in shmat...\n");
			goto out1;
		}
/*
		if (!x) {
			memset(pMem, 0, 0x1000);
			x = 1;
		}
*/
        } else {
		// initialize this area...
		pMem = shmat(shmid, 0, 0);
		if (pMem == (void *)-1) {
			MY_PRINTF("Error in shmat...\n");
			goto out1;
		}
		memset(pMem, 0, 0x1000);
/*
		if (!x) {
			memset(pMem, 0, 0x1000);
			x = 1;
		}
*/
	}

	if ((path != NULL) && (!strncmp(path, "ptp:", 4))) {
		int num;

		num = reserv_volume((volume_struct *)pMem, name, 0, 0, alias);
		if (num < 0) {
			MY_PRINTF("Error in get volume number...\n");
			goto out2;
		}

		{
			char		mnt_point[60] = "/tmp/usbmounts/";
			char		sys_product[] = "/sys";

			strcat(mnt_point, name);
			make_symlink((volume_struct *)pMem, num, mnt_point);
			strcat(sys_product, path+4);
			*(strrchr(sys_product, '/')) = 0;
			strcat(sys_product, "/product");
			make_volumefile((volume_struct *)pMem, num, "ptp", sys_product);
		}
	} else if (strlen(name) >= 4) {
		char dev_name[30];
		int fd, num, ext;

		// read partition number
		strcpy(dev_name, "/sys/block/");
		strncat(dev_name, name, 3);
		strcat(dev_name, "/part_num");
		MY_PRINTF("file name: %s \n", dev_name);
		fd = open(dev_name, O_RDONLY);
		if (fd < 0) {
			MY_PRINTF("Error in open part_num...\n");
			goto out2;
		}
		if (read(fd, dev_name, sizeof(dev_name)) < 0) {
			MY_PRINTF("Error in read part_num...\n");
			close(fd);
			goto out2;
		}
		sscanf(dev_name, "%d", &num);
		MY_PRINTF("part number: %d \n", num);
		close(fd);

		// read extended partition
		strcpy(dev_name, "/sys/block/");
		strncat(dev_name, name, 3);
		strcat(dev_name, "/part_extended");
		MY_PRINTF("file name: %s \n", dev_name);
		fd = open(dev_name, O_RDONLY);
		if (fd < 0) {
			MY_PRINTF("Error in open part_extended...\n");
			goto out2;
		}
		if (read(fd, dev_name, sizeof(dev_name)) < 0) {
			MY_PRINTF("Error in read part_extended...\n");
			close(fd);
			goto out2;
		}
		sscanf(dev_name, "%d", &ext);
		MY_PRINTF("extended number: %d \n", ext);
		close(fd);

		num = reserv_volume((volume_struct *)pMem, name, num, ext, alias);
		if (num < 0) {
			MY_PRINTF("Error in get volume number...\n");
			goto out2;
		}
		// check if we have mounted this partition...
		if (path == NULL) {
			FILE		*mtab;
			struct mntent	*mbuf;
			char		mnt_point[30] = "/tmp/usbmounts/";

			strcat(mnt_point, name);
			make_symlink((volume_struct *)pMem, num, mnt_point);

			// get the filesystem type...
			mtab = setmntent("/etc/mtab", "r");
			if (mtab) {
				while ((mbuf = getmntent(mtab))) {
					if (strcmp(mbuf->mnt_dir, mnt_point) == 0) {
						MY_PRINTF("file system: %s \n", mbuf->mnt_type);
						MY_PRINTF("device: %s \n", mbuf->mnt_fsname);
						make_volumefile((volume_struct *)pMem, num, mbuf->mnt_type, mbuf->mnt_fsname);
						break;
					}
				}
				endmntent(mtab);
			}
		} else {
			make_file((volume_struct *)pMem, num, path);
		}
	} else if (strlen(name) == 3) {
//		char dev_name[30] = "/sys/block/";
		int num;

		num = reserv_volume((volume_struct *)pMem, name, 0, 0, alias);
		if (num < 0) {
			MY_PRINTF("Error in get volume number...\n");
			goto out2;
		}
		// check if we have mounted this partition...
		if (path == NULL) {
			FILE		*mtab;
			struct mntent	*mbuf;
			char		mnt_point[30] = "/tmp/usbmounts/";

			strcat(mnt_point, name);
			make_symlink((volume_struct *)pMem, num, mnt_point);

			// get the filesystem type...
			mtab = setmntent("/etc/mtab", "r");
			if (mtab) {
				while ((mbuf = getmntent(mtab))) {
					if (strcmp(mbuf->mnt_dir, mnt_point) == 0) {
						MY_PRINTF("file system: %s \n", mbuf->mnt_type);
						MY_PRINTF("device: %s \n", mbuf->mnt_fsname);
						make_volumefile((volume_struct *)pMem, num, mbuf->mnt_type, mbuf->mnt_fsname);
						break;
					}
				}
				endmntent(mtab);
			}
		} else {
			make_file((volume_struct *)pMem, num, path);
		}
	} else {
		MY_PRINTF("Error in device name: %s \n", name);
		goto out2;
	}

out2:
	shmdt(pMem);
out1:
	// put the lock...
	flock(fd_lock, LOCK_UN);
	close(fd_lock);

	return 0;
}

int del_partition(char *name)
{
	int fd_lock;
	int shmid;
	char *pMem = NULL;
	int num;

	// get the lock...
	fd_lock = open("/var/lock/hotplug/volume_lock", O_RDONLY);
	if (fd_lock < 0) {
	        MY_PRINTF("Error in open lock file...\n");
		return -1;
	}
	flock(fd_lock, LOCK_EX);

	shmid = shmget(0x23792379, 0x1000, IPC_EXCL | 0660);
	if (shmid == -1) {
		MY_PRINTF("Error in shmget...\n");
		goto out;
	}
	pMem = shmat(shmid, 0, 0);
	if (pMem == (void *)-1) {
		MY_PRINTF("Error in shmat...\n");
		goto out;
	}

	num = search_volume((volume_struct *)pMem, name);
	if (num >= 0) {
		del_symlink((volume_struct *)pMem, num);
		del_volumefile((volume_struct *)pMem, num);
		remove_volume((volume_struct *)pMem, num);
	}
	
out:
	// put the lock...
	flock(fd_lock, LOCK_UN);
	close(fd_lock);
	
	return 0;
}

#endif
