#ifndef __FAT32_H
#define __FAT32_H

#include "sleeplock.h"
#include "stat.h"
#include "buf.h"
#include "param.h"

#define ATTR_READ_ONLY      0x01
#define ATTR_HIDDEN         0x02
#define ATTR_SYSTEM         0x04
#define ATTR_VOLUME_ID      0x08
#define ATTR_DIRECTORY      0x10
#define ATTR_ARCHIVE        0x20
#define ATTR_LONG_NAME      0x0F

#define LAST_LONG_ENTRY     0x40
#define FAT32_EOC           0x0ffffff8
#define EMPTY_ENTRY         0xe5
#define END_OF_ENTRY        0x00
#define CHAR_LONG_NAME      13
#define CHAR_SHORT_NAME     11

#define FAT32_MAX_FILENAME  255
#define FAT32_MAX_PATH      260
#define ENTRY_CACHE_NUM     50

#define S_IFMT				0170000
#define S_IFSOCK			0140000		// socket
#define S_IFLNK				0120000		// symbolic link
#define S_IFREG				0100000		// regular file
#define S_IFBLK				0060000		// block device
#define S_IFDIR				0040000		// directory
#define S_IFCHR				0020000		// character device
#define S_IFIFO				0010000		// FIFO
#define ST_MODE_DIR			S_IFDIR


#define NAME_HASH	     1000000

struct dirent {
    char  filename[FAT32_MAX_FILENAME + 1];
    uint8   attribute;
    // uint8   create_time_tenth;
    // uint16  create_time;
    // uint16  create_date;
    // uint16  last_access_date;
    uint32  first_clus;
    // uint16  last_write_time;
    // uint16  last_write_date;
    uint32  file_size;

    uint32  cur_clus;
    uint    clus_cnt;

    /* for OS */
    uint8   dev;
    uint8   dirty;
    short   valid;
    int     ref;
    int     mnt;
    uint32  off;            // offset in the parent dir entry, for writing convenience
    struct dirent *parent;  // because FAT32 doesn't have such thing like inum, use this for cache trick
    struct dirent *next;
    struct dirent *prev;
    struct sleeplock    lock;
};

struct linux_dirent64 {
        uint64        d_ino;
        uint64         d_off;
        unsigned short  d_reclen;
        unsigned char   d_type;
        char            d_name[FAT32_MAX_FILENAME + 1];
};

struct Fat{
    uint32  first_data_sec;
    uint32  data_sec_cnt;
    uint32  data_clus_cnt;
    uint32  byts_per_clus;

    struct {
        uint16  byts_per_sec;
        uint8   sec_per_clus;
        uint16  rsvd_sec_cnt;
        uint8   fat_cnt;            /* count of FAT regions */
        uint32  hidd_sec;           /* count of hidden sectors */
        uint32  tot_sec;            /* total count of sectors including all regions */
        uint32  fat_sz;             /* count of sectors for a FAT region */
        uint32  root_clus;
    } bpb;
};

struct entry_cache {
    struct spinlock lock;
    struct dirent entries[ENTRY_CACHE_NUM];
};


struct fs{
    uint devno;
    int  valid;
    struct dirent* image;
    struct Fat fat;
    struct entry_cache ecache;
    struct dirent root;
    void (*disk_init)(struct dirent*image);
    void (*disk_read)(struct buf* b,struct dirent* image);
    void (*disk_write)(struct buf* b,struct dirent* image);
};

typedef struct __fsid_t {
	int __val[2];
} fsid_t;
typedef uint64 fsblkcnt_t;
typedef uint64 fsfilcnt_t;

struct statfs {
	unsigned long f_type, f_bsize;
	fsblkcnt_t f_blocks, f_bfree, f_bavail;
	fsfilcnt_t f_files, f_ffree;
	fsid_t f_fsid;
	unsigned long f_namelen, f_frsize, f_flags, f_spare[4];
};

extern struct fs FatFs[FSNUM];
extern struct fs* rootfs;


int                 fs_init();
int                 fat32_init(struct fs* self_fs);
struct fs*          fat32_img(struct dirent* img);
struct dirent *     dirlookup(struct dirent *dp, char *filename, uint *poff);
char*               formatname(char *name);
void                emake(struct dirent *dp, struct dirent *ep, uint off);
struct dirent *     ealloc(struct dirent *dp, char *name, int attr);
struct dirent *     edup(struct dirent *entry);
void                eupdate(struct dirent *entry);
void                etrunc(struct dirent *entry);
void                eremove(struct dirent *entry);
void                eput(struct dirent *entry);
void                estat(struct dirent *de, struct stat *st);
void                ekstat(struct dirent *de, struct kstat *st);
void                estatfs(struct dirent *de, struct statfs *st);
void                elock(struct dirent *entry);
void                eunlock(struct dirent *entry);
int                 enext(struct dirent *dp, struct dirent *ep, uint off, int *count);
struct dirent *     ename(struct dirent* env,char* path,int *devno);
struct dirent *     enameparent(struct dirent* env, char* path, char* name,int *devno);
int                 eread(struct dirent *entry, int user_dst, uint64 dst, uint off, uint n);
int                 ewrite(struct dirent *entry, int user_src, uint64 src, uint off, uint n);
int                 emount(struct fs* fatfs,char* mnt);
int                 eumount(char* mnt);
int                 hashpath(char* name);
int                 isdirempty(struct dirent *dp);
struct dirent*      create(struct dirent* env, char *path, short type, int mode, int *err);
#endif
