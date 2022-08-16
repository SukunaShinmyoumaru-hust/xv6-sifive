#ifndef __FILE_H
#define __FILE_H

#include "types.h"
#include "dev.h"
#include "fcntl.h"

struct iovec {
  void  *iov_base;    /* Starting address */
  uint iov_len;     /* Number of bytes to transfer */
};

struct poll_table;

struct file {
  enum { FD_NONE, FD_PIPE, FD_ENTRY, FD_DEVICE } type;
  int ref; // reference count
  char readable;
  char writable;
  struct pipe *pipe; // FD_PIPE
  struct dirent *ep;
  struct spinlock lk;
  uint64 off;          // FD_ENTRY
  short major;       // FD_DEVICE
  uint32 (*poll)(struct file *, struct poll_table *);
  uint64 t0_sec;
  uint64 t0_nsec;
  uint64 t1_sec;
  uint64 t1_nsec;

};

// #define major(dev)  ((dev) >> 16 & 0xFFFF)
// #define minor(dev)  ((dev) & 0xFFFF)
// #define	mkdev(m,n)  ((uint)((m)<<16| (n)))

// map major device number to device functions.


#define CONSOLE 1

#define SEEK_SET  (int)0
#define SEEK_CUR  (int)1
#define SEEK_END  (int)2

#define R_OK	4		/* Test for read permission.  */
#define W_OK	2		/* Test for write permission.  */
#define X_OK	1		/* Test for execute permission.  */
#define F_OK	0		/* Test for existence.  */  


struct file*    filealloc(void);
void            fileclose(struct file*);
struct file*    filedup(struct file*);
void            fileinit(void);
void            print_f_info(struct file* f);
int             fileread(struct file*, uint64, int);
int             filestat(struct file*, uint64 addr);
int		          filekstat(struct file *f, uint64 addr);
int             filewrite(struct file*, uint64, int n);
uint64          filesend(struct file* fin,struct file* fout,uint64 addr,uint64 n);
int             dirnext(struct file *f, uint64 addr);
int             dirent_next(struct file *f, uint64 addr, int n);
uint64			filelseek(struct file *f, uint64 offset, int whence);
struct file*    findfile(struct dirent* ep);
#endif
