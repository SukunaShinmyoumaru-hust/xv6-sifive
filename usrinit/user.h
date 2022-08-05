#define O_RDONLY  0x000
#define O_WRONLY  0x001
#define O_RDWR    0x002
#define O_APPEND  0x004
#define O_CREATE  0x40
#define O_TRUNC   0x400
#define O_DIRECTORY	0x0200000

#define AT_FDCWD -100
typedef unsigned long uint64;

extern int openat(int dirfd, char* path, int flags, int mode);
extern int write(int fd, const void *buf, int len);
extern int read(int fd, void *buf, int len);
extern int wait4(int pid,int* status);
extern int clone(uint64 flag, uint64 stack, uint64 ptid, uint64 tls, uint64 ctid);
extern int exit(int n);
extern int getpid();
extern int getppid();

void printf(const char *s, ...);
static inline int wait(int pid){
  return wait4(pid,0);
}

static inline int fork(){
  return clone(0, 0, 0, 0, 0);
}
