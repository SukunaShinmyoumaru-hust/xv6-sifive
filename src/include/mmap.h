#ifndef _MMAP_H_
#define _MMAP_H_

#include "types.h"
#include "vma.h"

#define PROT_NONE		0
#define PROT_READ		1
#define PROT_WRITE		2
#define PROT_EXEC		4
#define PROT_GROWSDOWN	0x01000000
#define PROT_GROWSUP	0x02000000
#define PROT_ALL (PROT_READ|PROT_WRITE|PROT_EXEC|PROT_GROWSDOWN|PROT_GROWSUP)

#define MAP_FILE		0
#define MAP_SHARED		0x01
#define MAP_PRIVATE		0x02
#define MAP_FIXED		0x10
#define MAP_ANONYMOUS		0x20
#define MAP_FAILED ((void *) -1)

#define MS_ASYNC	1
#define MS_INVALIDATE	2
#define MS_SYNC	4

#define MMAP_SHARE_FLAG	0x1L
#define MMAP_ANONY_FLAG	0x2L

#define MMAP_FILE(x)	((void *)((uint64)(x) & ~(MMAP_SHARE_FLAG|MMAP_ANONY_FLAG)))
#define MMAP_SHARE(x)	((uint64)(x) & MMAP_SHARE_FLAG)
#define MMAP_ANONY(x)	((uint64)(x) & MMAP_ANONY_FLAG)

typedef struct vma map_fix;

uint64 do_mmap(uint64 start, uint64 len,int prot,int flags,int fd, off_t offset);
uint64 do_munmap(struct proc* np,uint64 start, uint64 len);
void  free_map_fix(struct proc* p);
map_fix * find_map_fix(struct proc *p, uint64 start, uint64 len);
int do_msync(uint64 addr, uint64 len, int flags);

#endif
