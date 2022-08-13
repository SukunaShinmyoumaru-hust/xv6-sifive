#ifndef __DEV_H
#define __DEV_H

#include "fat32.h"

#define DEV_NAME_MAX 12
#define CONSOLE_BUF_LEN 0x100

extern struct dirent* dev;

// #define major(dev)  ((dev) >> 16 & 0xFFFF)
// #define minor(dev)  ((dev) & 0xFFFF)
// #define	mkdev(m,n)  ((uint)((m)<<16| (n)))

// map major device number to device functions.
struct devsw {
  char name[DEV_NAME_MAX+1];
  struct spinlock lk;
  int (*read)(int, uint64, int);
  int (*write)(int, uint64, int);
};

extern struct devsw devsw[];

int devinit();
int devlookup(char* name);
int getdevnum();
int allocdev(char* name,int (*devread)(int, uint64, int),int (*devwrite)(int, uint64, int));
int rtcread(int user_dst, uint64 addr, int n);
int rtcwrite(int user_dst, uint64 addr, int n);
int nullread(int user_dst,uint64 addr,int n);
int nullwrite(int user_dst,uint64 addr,int n);
int zeroread(int user_dst,uint64 addr,int n);
int zerowrite(int user_dst,uint64 addr,int n);
int consoleread(int user_dst,uint64 addr,int n);
int consolewrite(int user_dst,uint64 addr,int n);
int devkstat(struct devsw* mydev, struct kstat* kst);

#endif
