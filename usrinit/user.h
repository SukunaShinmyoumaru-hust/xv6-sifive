#ifndef __USER_H
#define __USER_H

#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_APPEND  0x004
#define O_CREATE  0x40
#define O_TRUNC   0x400
#define O_DIRECTORY	0x0200000

#define PROT_NONE		0
#define PROT_READ		1
#define PROT_WRITE		2
#define PROT_EXEC		4

#define AT_FDCWD -100
typedef unsigned long uint64;

typedef struct{
  int valid;
  char* name;
}mytest;

typedef struct{
  int valid;
  char* name[20];
}longtest;


extern int openat(int dirfd, char* path, int flags, int mode);
extern int write(int fd, const void *buf, int len);
extern int read(int fd, void *buf, int len);
extern int wait4(int pid,int* status);
extern int execve(char* path,char** argv,char** env);
extern int clone(uint64 flag, uint64 stack, uint64 ptid, uint64 tls, uint64 ctid);
extern int exit(int n);
extern int getpid();
extern int getppid();
extern void* mmap(void* start, uint64 len, int prot, int flags, int fd, uint64 offset);
extern int munmap(void* start, uint64 len);

void test_lua();
void test_lmbench();
void test_busybox();

void printf(const char *s, ...);
static inline int wait(int pid){
  return wait4(pid,0);
}

static inline int fork(){
  return clone(0, 0, 0, 0, 0);
}

static inline int exec(char* path,char** argv){
  return execve(path,argv,0);
}

#endif
