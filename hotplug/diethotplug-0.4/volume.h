#ifndef _VOLUME_H
#define _VOLUME_H

#ifndef RESCUE_LINUX

#define VOLUME_NUMBER	26
#define NAME_LENGTH	32
#define ALIAS_LENGTH	16

#define MYDEBUG

#ifdef MYDEBUG
	#define MY_PRINTF(...) printf(__VA_ARGS__)
#else
	#define MY_PRINTF(...)
#endif

#define UTF16_LE	0x01
#define UTF16_BE	0x02
#define	UTF8		0x03
#define	UNKNOWN		0xff

typedef __signed char		s8;
typedef unsigned char		u8;
typedef __signed short		s16;
typedef unsigned short		u16;
typedef __signed int		s32;
typedef unsigned int		u32;
typedef __signed long long	s64;
typedef unsigned long long	u64;

typedef u16	le16;
typedef u32	le32;
typedef u64	le64;
typedef u64	sle64;

typedef u8 RESIDENT_ATTR_FLAGS;
typedef le16 MFT_RECORD_FLAGS;
typedef le16 ATTR_FLAGS;
typedef le32 NTFS_RECORD_TYPE;
typedef le32 ATTR_TYPE;
typedef le64 leMFT_REF;
typedef sle64 leVCN;

typedef struct {
        NTFS_RECORD_TYPE magic; /* A four-byte magic identifying the record
                                   type and/or status. */
        le16 usa_ofs;           /* Offset to the Update Sequence Array (usa)
                                   from the start of the ntfs record. */
        le16 usa_count;         /* Number of le16 sized entries in the usa
                                   including the Update Sequence Number (usn),
                                   thus the number of fixups is the usa_count
                                   minus 1. */
} __attribute__ ((__packed__)) NTFS_RECORD;

typedef struct {
/*Ofs*/
/*  0   NTFS_RECORD; -- Unfolded here as gcc doesn't like unnamed structs. */
        NTFS_RECORD_TYPE magic; /* Usually the magic is "FILE". */
        le16 usa_ofs;           /* See NTFS_RECORD definition above. */
        le16 usa_count;         /* See NTFS_RECORD definition above. */

/*  8*/ le64 lsn;               /* $LogFile sequence number for this record.
                                   Changed every time the record is modified. */
/* 16*/ le16 sequence_number;   /* Number of times this mft record has been
                                   reused. (See description for MFT_REF
                                   above.) NOTE: The increment (skipping zero)
                                   is done when the file is deleted. NOTE: If
                                   this is zero it is left zero. */
/* 18*/ le16 link_count;        /* Number of hard links, i.e. the number of
                                   directory entries referencing this record.
                                   NOTE: Only used in mft base records.
                                   NOTE: When deleting a directory entry we
                                   check the link_count and if it is 1 we
                                   delete the file. Otherwise we delete the
                                   FILE_NAME_ATTR being referenced by the
                                   directory entry from the mft record and
                                   decrement the link_count.
                                   FIXME: Careful with Win32 + DOS names! */
/* 20*/ le16 attrs_offset;      /* Byte offset to the first attribute in this
                                   mft record from the start of the mft record.
                                   NOTE: Must be aligned to 8-byte boundary. */
/* 22*/ MFT_RECORD_FLAGS flags; /* Bit array of MFT_RECORD_FLAGS. When a file
                                   is deleted, the MFT_RECORD_IN_USE flag is
                                   set to zero. */
/* 24*/ le32 bytes_in_use;      /* Number of bytes used in this mft record.
                                   NOTE: Must be aligned to 8-byte boundary. */
/* 28*/ le32 bytes_allocated;   /* Number of bytes allocated for this mft
                                   record. This should be equal to the mft
                                   record size. */
/* 32*/ leMFT_REF base_mft_record;/* This is zero for base mft records.
                                   When it is not zero it is a mft reference
                                   pointing to the base mft record to which
                                   this record belongs (this is then used to
                                   locate the attribute list attribute present
                                   in the base record which describes this
                                   extension record and hence might need
                                   modification when the extension record
                                   itself is modified, also locating the
                                   attribute list also means finding the other
                                   potential extents, belonging to the non-base
                                   mft record). */
/* 40*/ le16 next_attr_instance;/* The instance number that will be assigned to
                                   the next attribute added to this mft record.
                                   NOTE: Incremented each time after it is used.
                                   NOTE: Every time the mft record is reused
                                   this number is set to zero.  NOTE: The first
                                   instance number is always 0. */
/* The below fields are specific to NTFS 3.1+ (Windows XP and above): */
/* 42*/ le16 reserved;          /* Reserved/alignment. */
/* 44*/ le32 mft_record_number; /* Number of this mft record. */
/* sizeof() = 48 bytes */
/*
 * When (re)using the mft record, we place the update sequence array at this
 * offset, i.e. before we start with the attributes.  This also makes sense,
 * otherwise we could run into problems with the update sequence array
 * containing in itself the last two bytes of a sector which would mean that
 * multi sector transfer protection wouldn't work.  As you can't protect data
 * by overwriting it since you then can't get it back...
 * When reading we obviously use the data from the ntfs record header.
 */
} __attribute__ ((__packed__)) MFT_RECORD;

typedef struct {
/*Ofs*/
/*  0*/ ATTR_TYPE type;         /* The (32-bit) type of the attribute. */
/*  4*/ le32 length;            /* Byte size of the resident part of the
                                   attribute (aligned to 8-byte boundary).
                                   Used to get to the next attribute. */
/*  8*/ u8 non_resident;        /* If 0, attribute is resident.
                                   If 1, attribute is non-resident. */
/*  9*/ u8 name_length;         /* Unicode character size of name of attribute.
                                   0 if unnamed. */
/* 10*/ le16 name_offset;       /* If name_length != 0, the byte offset to the
                                   beginning of the name from the attribute
                                   record. Note that the name is stored as a
                                   Unicode string. When creating, place offset
                                   just at the end of the record header. Then,
                                   follow with attribute value or mapping pairs
                                   array, resident and non-resident attributes
                                   respectively, aligning to an 8-byte
                                   boundary. */
/* 12*/ ATTR_FLAGS flags;       /* Flags describing the attribute. */
/* 14*/ le16 instance;          /* The instance of this attribute record. This
                                   number is unique within this mft record (see
                                   MFT_RECORD/next_attribute_instance notes in
                                   in mft.h for more details). */
/* 16*/ union {
                /* Resident attributes. */
                struct {
/* 16 */                le32 value_length;/* Byte size of attribute value. */
/* 20 */                le16 value_offset;/* Byte offset of the attribute
                                             value from the start of the
                                             attribute record. When creating,
                                             align to 8-byte boundary if we
                                             have a name present as this might
                                             not have a length of a multiple
                                             of 8-bytes. */
/* 22 */                RESIDENT_ATTR_FLAGS flags; /* See above. */
/* 23 */                s8 reserved;      /* Reserved/alignment to 8-byte
                                             boundary. */
                } __attribute__ ((__packed__)) resident;
                /* Non-resident attributes. */
                struct {
/* 16*/                 leVCN lowest_vcn;/* Lowest valid virtual cluster number
                                for this portion of the attribute value or
                                0 if this is the only extent (usually the
                                case). - Only when an attribute list is used
                                does lowest_vcn != 0 ever occur. */
/* 24*/                 leVCN highest_vcn;/* Highest valid vcn of this extent of
                                the attribute value. - Usually there is only one
                                portion, so this usually equals the attribute
                                value size in clusters minus 1. Can be -1 for
                                zero length files. Can be 0 for "single extent"
                                attributes. */
/* 32*/                 le16 mapping_pairs_offset; /* Byte offset from the
                                beginning of the structure to the mapping pairs
                                array which contains the mappings between the
                                vcns and the logical cluster numbers (lcns).
                                When creating, place this at the end of this
                                record header aligned to 8-byte boundary. */
/* 34*/                 u8 compression_unit; /* The compression unit expressed
                                as the log to the base 2 of the number of
                                clusters in a compression unit. 0 means not
                                compressed. (This effectively limits the
                                compression unit size to be a power of two
                                clusters.) WinNT4 only uses a value of 4. */
/* 35*/                 u8 reserved[5];         /* Align to 8-byte boundary. */
/* The sizes below are only used when lowest_vcn is zero, as otherwise it would
   be difficult to keep them up-to-date.*/
/* 40*/                 sle64 allocated_size;   /* Byte size of disk space
                                allocated to hold the attribute value. Always
                                is a multiple of the cluster size. When a file
                                is compressed, this field is a multiple of the
                                compression block size (2^compression_unit) and
                                it represents the logically allocated space
                                rather than the actual on disk usage. For this
                                use the compressed_size (see below). */
/* 48*/                 sle64 data_size;        /* Byte size of the attribute
                                value. Can be larger than allocated_size if
                                attribute value is compressed or sparse. */
/* 56*/                 sle64 initialized_size; /* Byte size of initialized
                                portion of the attribute value. Usually equals
                                data_size. */
/* sizeof(uncompressed attr) = 64*/
/* 64*/                 sle64 compressed_size;  /* Byte size of the attribute
                                value after compression. Only present when
                                compressed. Always is a multiple of the
                                cluster size. Represents the actual amount of
                                disk space being used on the disk. */
/* sizeof(compressed attr) = 72*/
                } __attribute__ ((__packed__)) non_resident;
        } __attribute__ ((__packed__)) data;
} __attribute__ ((__packed__)) ATTR_RECORD;

typedef struct {
	int	occupied;
	char	name[NAME_LENGTH];
	char	alias[ALIAS_LENGTH];
} volume_struct;

void get_mntpath(int num, char *path);
int add_partition(char *name, char *path, char *alias, int *is_all_mounted);
int del_partition(char *name, int *is_all_umounted);

#else

#warning **********Using RESCUE version**********
#define add_partition(...)	(__VA_ARGS__, 0)
#define del_partition(...)	(__VA_ARGS__, 0)
#define is_all_mounted(...)	(__VA_ARGS__, 0)
#define is_all_umounted(...)	(__VA_ARGS__, 0)

#endif 

#endif

