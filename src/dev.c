#include"include/fat32.h"
#include"include/param.h"
#include"include/string.h"
#include"include/printf.h"
#include"include/copy.h"
#include"include/dev.h"
#include"include/sbi.h"
#include"include/riscv.h"

struct dirent* dev;
int devnum;


#define BACKSPACE 0x100
#define C(x)  ((x)-'@')  // Control-x

struct devsw devsw[NDEV];
struct dirent* selfexe;

extern char sacrifice_start[];
extern uint64 sacrifice_size;
extern char localtime[];
extern uint64 localtime_size;
extern char mounts[];
extern uint64 mounts_size;
extern char meminfo[];
extern uint64 meminfo_size;
extern char lat_sig[];
extern uint64 lat_sig_size;
extern char hello[];
extern uint64 hello_size;
extern char sh[];
extern uint64 sh_size;
struct dirent* loadfile(char* name, char* start, uint64 size, int need);

int devinit()
{
  int err = 0;
  devnum = 0;
  dev = create(NULL,"/dev",T_DIR,0, &err);
  eunlock(dev);
  loadfile("/etc/passwd", 0, 0, 0);
  loadfile("/etc/adjtime", 0, 0, 0);
  loadfile("/etc/group", 0, 0, 0);
  loadfile("/bin/ls", 0, 0, 0);
  loadfile("/etc/localtime", localtime, localtime_size, 0);
  loadfile("/proc/mounts", mounts, mounts_size, 0);
  loadfile("/proc/meminfo", meminfo, meminfo_size, 0);
  loadfile("/sacrifice", sacrifice_start, sacrifice_size, 0);
  loadfile("/lat_sig", lat_sig, lat_sig_size, 0);
  loadfile("/tmp/hello", hello, hello_size, 0);
  loadfile("/bin/sh", sh, sh_size, 0);
  selfexe = loadfile("/proc/self/exe", 0, 0, 1);
  __debug_info("devinit\n");
  memset(devsw,0,NDEV*sizeof(struct devsw));
  allocdev("console",consoleread,consolewrite);
  allocdev("tty",consoleread,consolewrite);
  allocdev("null",nullread,nullwrite);
  allocdev("zero",zeroread,zerowrite);
  allocdev("rtc",rtcread,rtcwrite);
  return 0;
}

struct dirent*
loadfile(char* name, char* start, uint64 size, int need)
{
  int err;
  struct dirent* ep = ename(NULL, name, NULL);
  if(ep){
    goto ret;
  }
  ep = create(NULL, name, T_FILE, 0, &err);
  if(size)ewrite(ep, 0, (uint64)start, 0, size);
  eunlock(ep);
ret:
  if(!need)eput(ep);
  return ep;
  
}

int getdevnum(){
  return devnum;
}

int
allocdev(char* name,int (*devread)(int, uint64, int),int (*devwrite)(int, uint64, int)){
  if(devnum == NDEV){
    __debug_warn("[dev]no more space for device\n");
    return -1;
  }
  strncpy(devsw[devnum].name,name,DEV_NAME_MAX+1);
  initlock(&devsw[devnum].lk,name);
  devsw[devnum].read = devread;
  devsw[devnum].write = devwrite;
  devnum++;
  return 0;
}

int 
devlookup(char *name)
{
  for(int i = 0;i < NDEV;i++){
    if(strncmp(name,devsw[i].name,DEV_NAME_MAX+1)==0){
      return i;
    }
  }
  return -1;
}

int
rtcread(int user_dst, uint64 addr, int n)
{
  return 0;
}

int
rtcwrite(int user_dst, uint64 addr, int n)
{
  return 0;
}

int
nullread(int user_dst,uint64 addr,int n){
  return 0;
}

int
nullwrite(int user_dst,uint64 addr,int n){
  return n;
}

int
zeroread(int user_dst,uint64 addr,int n){
  if(user_dst){
    return zero_out(addr,n);
  }
  else{
    memset((void*)addr,0,n);
    return n;
  }
}
  

int
zerowrite(int user_dst,uint64 addr,int n){
  return n;
}

int
consoleread(int user_dst,uint64 addr,int n){
  char readbuf[CONSOLE_BUF_LEN];
  int ret = 0;
  int interp = 0;
  while(n&&!interp){
    int len = MIN(n,CONSOLE_BUF_LEN);
    int i;
    for(i=0;i<len;i++){
      char c = 0;
      while((c=sbi_console_getchar())==255);
      c = c==13?10:c;
      readbuf[i] = c;
      consputc(c);
      if(c == 10){
        interp = 1;
        break;
      }
    }
    if(either_copyout(user_dst,addr,readbuf,i)<0){
      return ret;
    }
    n -= i;
    ret += i;
    addr += i;
  }
  return ret;
}

int
consolewrite(int user_dst,uint64 addr,int n){
  char writebuf[CONSOLE_BUF_LEN];
  int ret = 0;
  while(n){
    int len = MIN(n,CONSOLE_BUF_LEN);
    if(either_copyin(user_dst,writebuf,addr,len)<0){
      return ret;
    }
    for(int i=0;i<len;i++){
      consputc(writebuf[i]);
    }
    n -= len;
    ret += len;
    addr += len;
  }
  return ret;
}

int 
devkstat(struct devsw* mydev, struct kstat* st){
    st->st_dev = mydev-devsw;
    st->st_size = 0;
    st->st_blksize = 128;
    st->st_blocks = 0;
    st->st_atime_nsec = 0;
    st->st_atime_sec = 0;
    st->st_ctime_nsec = 0;
    st->st_ctime_sec = 0;
    st->st_mtime_nsec = 0;
    st->st_mtime_sec = 0;
    st->st_uid = 0;
    st->st_gid = 0;
    st->st_rdev = 0;
    st->st_nlink = 1;
    st->st_ino = hashpath(mydev->name);
    st->st_mode = S_IFCHR;
  return 0;
}

